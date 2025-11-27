/*
 * scheduler_bmi270.c
 *
 *  Created on: 10-Nov-2025
 *      Author: nindu
 */
#include "scheduler_bmi270.h"
#include "bmi270.h"
#include "gatt_db.h"
#include "ble.h"


typedef enum leTimerEvents
{
  UF_EVENT,
  COMP0_EVENT,
  COMP1_EVENT,
  I2C_TRANSFER_EVENT,
  MAX_MFIO_EVENT,
  BMI_INT1_EVENT,
  INVALID_EVENT
}allEvents_t;

typedef enum bmi270InitOperationState {
    BMI270_START_INITIALIZATION,
    BMI270_SOFT_RESET,
    BMI270_WAIT_FOR_RESET,
    BMI270_READ_CHIP_ID,
    BMI270_DISABLE_POWER_CONFIG,
    BMI270_WAIT_FOR_POWER_DISABLE,
    BMI270_SET_INIT_CTRL_0,
    BMI270_WAIT_INIT_CTRL_0,
    BMI270_UPLOAD_CHUNK_ADDR_0,
    BMI270_UPLOAD_CHUNK_ADDR_1,
    BMI270_UPLOAD_CHUNK_DATA,
    BMI270_WAIT_BEFORE_NEXT_CHUNK,
    BMI270_WAIT_BEFORE_INIT_CTRL,
    BMI270_SET_INIT_CTRL_1,
    BMI270_WAIT_INIT_CTRL_1,
    BMI270_CHECK_INIT_STATUS,
    BMI270_ENABLE_ODR,
    BMI270_START_ACCEL_ENABLE,
    BMI270_WAIT_FOR_ACCEL_ENABLE,
    BMI270_START_STEPCOUNTER_ENABLE,
    BMI270_READ_SELECTED_FEATURE_RANGE,
    BMI270_ENABLE_STEP_OP,
    BMI270_RESTORE_FEAT_PAGE0,
    BMI270_MAP_STEP_COUNTER_TO_INTERRUPT,
    BMI270_CONFIG_INT1,
    BMI270_STATE_INIT_COMPLETE
} bmi270InitOperationState_e;

typedef enum bmi270DataOperationState{
  BMI270_WAIT_FOR_INTERRUPT,
  BMI270_READ_BMI_INT_STATUS_REG,
  BMI270_READ_STEP_COUNTER_DATA,
  BMI270_SEND_INDICATION
}bmi270DataOperationState_e;

#define LETIMER0_UF    (1U << UF_EVENT)
#define LETIMER0_COMP0 (1U << COMP0_EVENT)
#define LETIMER0_COMP1 (1U << COMP1_EVENT)
#define I2C_TRANSFER_DONE (1U << I2C_TRANSFER_EVENT)
#define MAX_MFIO_INTERRUPT (1U << MAX_MFIO_EVENT)
#define BMI_INTERRUPT1 (1U << BMI_INT1_EVENT)

static bmi270InitOperationState_e currentInitStateMachineState = BMI270_START_INITIALIZATION;
static bmi270DataOperationState_e currentDataHandlingStateMachineState = BMI270_WAIT_FOR_INTERRUPT;
static bmi270InitStatus_e bmi270InitStatus = BMI270_INIT_IDLE;


void bmi270StateMachine(sl_bt_msg_t *bleEvent)
{
  allEvents_t event = INVALID_EVENT;
   switch (SL_BT_MSG_ID(bleEvent->header)) {
     case sl_bt_evt_connection_opened_id:
         break;
     case sl_bt_evt_connection_closed_id:
       break;
     case sl_bt_evt_gatt_server_characteristic_status_id:
       {
         uint8_t status_flags = bleEvent->data.evt_gatt_server_characteristic_status.status_flags;
          uint16_t client_config_flags = bleEvent->data.evt_gatt_server_characteristic_status.client_config_flags;
          uint16_t characteristic = bleEvent->data.evt_gatt_server_characteristic_status.characteristic;
          if ((status_flags == sl_bt_gatt_server_client_config)&&(characteristic == gattdb_Steps))
          {
              if (client_config_flags & sl_bt_gatt_indication)
              {
                  //Initiate App mode

              }

          }
       }
       break;
     case sl_bt_evt_system_external_signal_id:
       if(bleEvent->data.evt_system_external_signal.extsignals & I2C_TRANSFER_DONE)
       {
           event = I2C_TRANSFER_EVENT;
       }else if (bleEvent->data.evt_system_external_signal.extsignals & LETIMER0_UF) {
           event = UF_EVENT;
       } else if (bleEvent->data.evt_system_external_signal.extsignals & LETIMER0_COMP0) {
           event = COMP0_EVENT;
       } else if (bleEvent->data.evt_system_external_signal.extsignals & LETIMER0_COMP1) {
           event = COMP1_EVENT;
       }
       break;
     default:
       break;
   }

   if(SL_BT_MSG_ID(bleEvent->header) == sl_bt_evt_system_external_signal_id) {
          switch (currentInitStateMachineState) {

              case BMI270_START_INITIALIZATION:
                  bmi270StartInitSequence();
                  currentInitStateMachineState = BMI270_SOFT_RESET;
                  bmi270SendSoftReset();
                  break;

              case BMI270_SOFT_RESET:
                if(event==I2C_TRANSFER_EVENT)
                  {
                    waitForSoftReset();
                    currentInitStateMachineState= BMI270_WAIT_FOR_RESET;
                  }
                break;
              case BMI270_WAIT_FOR_RESET:
               if(event==COMP1_EVENT)
               {
                   currentInitStateMachineState=BMI270_READ_CHIP_ID;
                   bmi270ReadChipID(); // sends I2C read
               }
               break;

              case BMI270_READ_CHIP_ID:
                if(event == I2C_TRANSFER_EVENT) {
                  if(isValidBMI270ChipID()) {
                      currentInitStateMachineState=BMI270_DISABLE_POWER_CONFIG;
                      bmi270UpdatePowerMode(0);
                  } else {
                      bmi270InitStatus = BMI270_INIT_FAILED;
                      currentInitStateMachineState = BMI270_START_INITIALIZATION;
                  }
                }
                  break;
              case BMI270_DISABLE_POWER_CONFIG:
                if(event==I2C_TRANSFER_EVENT)
                  {
                    //Wait for 450us minimum.
                    currentInitStateMachineState = BMI270_WAIT_FOR_POWER_DISABLE;
                    waitForDisablePowerMode();

                  }
                break;
              case BMI270_WAIT_FOR_POWER_DISABLE:
                if(event==COMP1_EVENT)
                  {
                    currentInitStateMachineState = BMI270_SET_INIT_CTRL_0;
                    bmi270SetInitCtrl(0x00); // sends I2C write
                  }
                break;
              case BMI270_SET_INIT_CTRL_0:
                  if(event == I2C_TRANSFER_EVENT) {
                      currentInitStateMachineState = BMI270_WAIT_INIT_CTRL_0;
                      waitForInitCtrlToDisable();
                  }
                  break;

              case BMI270_WAIT_INIT_CTRL_0:
                if(event == COMP1_EVENT)
                  {
                  configUploadOffset = 0;
                  currentInitStateMachineState = BMI270_UPLOAD_CHUNK_ADDR_0;
                  uint16_t index = configUploadOffset;
                  uint8_t addr0 = (uint8_t)((index / 2) & 0x0F);
                  setInitAddr0(addr0);
                  }
                  break;

              case BMI270_UPLOAD_CHUNK_ADDR_0:
                if(event == I2C_TRANSFER_EVENT) {
                  uint16_t index = configUploadOffset;
                  uint8_t addr1 = (uint8_t)((index / 2) >> 4);
                  currentInitStateMachineState = BMI270_UPLOAD_CHUNK_ADDR_1;
                  setInitAddr1(addr1);
                }
                break;

              case BMI270_UPLOAD_CHUNK_ADDR_1:
                if(event == I2C_TRANSFER_EVENT) {
                  currentInitStateMachineState = BMI270_UPLOAD_CHUNK_DATA;
                  sendNextConfigChunk();
                }
                break;

              case BMI270_UPLOAD_CHUNK_DATA:
                if(event == I2C_TRANSFER_EVENT) {
                  if(hasMoreConfigData()) {
                    currentInitStateMachineState = BMI270_WAIT_BEFORE_NEXT_CHUNK;
                    waitBeforeNextTransfer();
                  } else {
                    currentInitStateMachineState = BMI270_WAIT_BEFORE_INIT_CTRL;
                    waitBeforeNextTransfer();
                  }
                }
                break;

              case BMI270_WAIT_BEFORE_NEXT_CHUNK:
                if(event == COMP1_EVENT) {
                  currentInitStateMachineState = BMI270_UPLOAD_CHUNK_ADDR_0;
                  uint16_t index = configUploadOffset;
                  uint8_t addr0 = (uint8_t)((index / 2) & 0x0F);
                  setInitAddr0(addr0);
                }
                break;

              case BMI270_WAIT_BEFORE_INIT_CTRL:
                if(event == COMP1_EVENT)
                  {
                  currentInitStateMachineState = BMI270_SET_INIT_CTRL_1;
                  bmi270SetInitCtrl(0x01); // sends I2C write
                  }
                  break;

              case BMI270_SET_INIT_CTRL_1:
                  if(event == I2C_TRANSFER_EVENT) {
                      currentInitStateMachineState = BMI270_WAIT_INIT_CTRL_1;
                      waitForInternalStatusToUpdate();
                  }
                  break;

              case BMI270_WAIT_INIT_CTRL_1:
                if(event==COMP1_EVENT)
                  {
                    currentInitStateMachineState = BMI270_CHECK_INIT_STATUS;
                    bmi270ReadInitStatus(); // sends I2C read
                  }
                  break;

              case BMI270_CHECK_INIT_STATUS:
                if(event == I2C_TRANSFER_EVENT) {
                  if(isBMI270InitSuccessful()) {
                      currentInitStateMachineState=BMI270_ENABLE_ODR;
                      bmi270EnableODR();

                  } else {
                      bmi270InitStatus = BMI270_INIT_FAILED;
                      currentInitStateMachineState = BMI270_START_INITIALIZATION;
                  }
                }
                  break;
              case BMI270_ENABLE_ODR:
                if(event == I2C_TRANSFER_EVENT) {
                    currentInitStateMachineState=BMI270_START_ACCEL_ENABLE;
                    bmi270EnableAccel();
                }
                break;
              case BMI270_START_ACCEL_ENABLE:
                if(event == I2C_TRANSFER_EVENT) {
                    currentInitStateMachineState=BMI270_WAIT_FOR_ACCEL_ENABLE;
                    waitForAccelEnable();
                }
                break;
              case BMI270_WAIT_FOR_ACCEL_ENABLE:
                if(event == COMP1_EVENT) {
                    currentInitStateMachineState=BMI270_START_STEPCOUNTER_ENABLE;
                    startStepCounterEnable();
                }
                break;
              case BMI270_START_STEPCOUNTER_ENABLE:
                if(event == I2C_TRANSFER_EVENT) {
                    currentInitStateMachineState=BMI270_READ_SELECTED_FEATURE_RANGE;
                    readSelectedFeaturePage();
                }
                break;

              case BMI270_READ_SELECTED_FEATURE_RANGE:
                if(event == I2C_TRANSFER_EVENT) {
                    currentInitStateMachineState=BMI270_ENABLE_STEP_OP;
                    enableStepOpAndWriteToFeaturePage();
                }
                break;

              case BMI270_ENABLE_STEP_OP:
                if (event == I2C_TRANSFER_EVENT) {
                    // Step feature configuration has been written on page 6.
                    // Now restore FEAT_PAGE back to 0 to avoid side effects on later accesses.
                    currentInitStateMachineState = BMI270_RESTORE_FEAT_PAGE0;
                    bmi270RestoreFeaturePage();
                }
                break;

              case BMI270_RESTORE_FEAT_PAGE0:
                if (event == I2C_TRANSFER_EVENT) {
                    // Now that FEAT_PAGE is back to 0, map the step counter interrupt.
                    currentInitStateMachineState = BMI270_MAP_STEP_COUNTER_TO_INTERRUPT;
                    mapStepCounterToInterrupt1();
                }
                break;
              case BMI270_MAP_STEP_COUNTER_TO_INTERRUPT:
                if(event == I2C_TRANSFER_EVENT) {
                    currentInitStateMachineState=BMI270_CONFIG_INT1;
                    configureInt1ToOutputEnable();
                }
                break;
              case BMI270_CONFIG_INT1:
                if(event == I2C_TRANSFER_EVENT) {
                    currentInitStateMachineState=BMI270_STATE_INIT_COMPLETE;
                    bmi270InitStatus = BMI270_INIT_SUCCESSFUL;
                }
                break;

              default:
                  break;
          }
      }
}

bmi270InitStatus_e getLatestBmi270InitState()
{
  return bmi270InitStatus;

}

static void sendIndicationsOfStepCount(uint32_t stepCount)
{
  // Buffer for 32-bit integer (4 bytes)
  // Note: Unlike HTM, we don't strictly need a 'Flags' byte unless your
  // custom characteristic specification requires it.
  // Here we send just the raw 32-bit value.
  uint8_t step_buffer[4];

  // Copy the uint32_t stepCount into the byte buffer (Little Endian)
  // SiLabs/ARM Cortex-M is Little Endian, so memcpy works directly.
  memcpy(step_buffer, &stepCount, sizeof(stepCount));

  // 1. Update GATT database with new step value so read requests get the latest data
  sl_status_t sc = sl_bt_gatt_server_write_attribute_value(
      gattdb_Steps,   // You must define this in your GATT Configurator
      0,                   // Offset
      sizeof(step_buffer), // Length (4 bytes)
      step_buffer          // Data
  );

//  if (sc != SL_STATUS_OK) {
//      LOG_ERROR("Failed to update GATT database with step count %d", (int)sc);
//  }

  // Get the BLE context
  ble_data_struct_t* bleDataPtr = getBleData();

  // 2. Send Indication if connection is active and notifications/indications are enabled
  // Note: You need to add 'ok_to_send_step_indications' to your ble_data_struct_t
  // and update it when the CCCD for the step characteristic is changed.
  if (bleDataPtr->connection_open &&
      bleDataPtr->ok_to_send_stepCounter_indications &&
      !bleDataPtr->indication_in_flight_stepCounter)
  {
      sc = sl_bt_gatt_server_send_indication(
          bleDataPtr->connection_handle,
          gattdb_Steps,    // Handle from gatt_db.h
          sizeof(step_buffer),  // Length
          step_buffer           // Data
      );

      if (sc != SL_STATUS_OK) {
          //LOG_ERROR("Failed to send step indication %d", (int)sc);
      } else {
          // Mark indication as in flight to prevent overlapping sends
          // You typically clear this flag in the sl_bt_evt_gatt_server_characteristic_status_id event
          bleDataPtr->indication_in_flight_stepCounter = true;
      }
  }
}

void bmi270DataHandlingStateMachine(sl_bt_msg_t *bleEvent)
{
  allEvents_t event = INVALID_EVENT;
   switch (SL_BT_MSG_ID(bleEvent->header)) {
     case sl_bt_evt_connection_opened_id:
         break;
     case sl_bt_evt_connection_closed_id:
       break;
     case sl_bt_evt_gatt_server_characteristic_status_id:
       break;
     case sl_bt_evt_system_external_signal_id:
       if(bleEvent->data.evt_system_external_signal.extsignals & I2C_TRANSFER_DONE)
       {
           event = I2C_TRANSFER_EVENT;
       }else if (bleEvent->data.evt_system_external_signal.extsignals & LETIMER0_UF) {
           event = UF_EVENT;
       } else if (bleEvent->data.evt_system_external_signal.extsignals & LETIMER0_COMP0) {
           event = COMP0_EVENT;
       } else if (bleEvent->data.evt_system_external_signal.extsignals & LETIMER0_COMP1) {
           event = COMP1_EVENT;
       }else if (bleEvent->data.evt_system_external_signal.extsignals & MAX_MFIO_INTERRUPT) {
           event = MAX_MFIO_EVENT;
       }else if (bleEvent->data.evt_system_external_signal.extsignals & BMI_INTERRUPT1) {
           event = BMI_INT1_EVENT;
       }
       break;
     default:
       break;
   }

   if(SL_BT_MSG_ID(bleEvent->header) == sl_bt_evt_system_external_signal_id) {
       switch (currentDataHandlingStateMachineState) {
         case BMI270_WAIT_FOR_INTERRUPT:
           {
             if(event==BMI_INT1_EVENT)
               {
                 currentDataHandlingStateMachineState=BMI270_READ_BMI_INT_STATUS_REG;
                 bmi270StartReadingIntStatus0Reg();
                 //Call Read Step COunter data
               }

           }
           break;
         case BMI270_READ_BMI_INT_STATUS_REG:
           if(event==I2C_TRANSFER_EVENT)
           {
               getIntStatus0Value();
               currentDataHandlingStateMachineState=BMI270_READ_STEP_COUNTER_DATA;
               bmi270StartReadingStepCounterData();
           }
           break;
         case BMI270_READ_STEP_COUNTER_DATA:
           {
             if(event==I2C_TRANSFER_EVENT)
             {
                 uint32_t stepcounter = getStepCounterData();
                 currentDataHandlingStateMachineState=BMI270_SEND_INDICATION;
                 sendIndicationsOfStepCount(stepcounter);

             }

           }
           break;
         case BMI270_SEND_INDICATION:
           {
             currentDataHandlingStateMachineState=BMI270_WAIT_FOR_INTERRUPT;
           }
           break;
         default:
           break;
       }
   }
}

