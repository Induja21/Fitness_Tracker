/*
 * scheduler_bmi270.c
 *
 *  Created on: 10-Nov-2025
 *      Author: nindu
 */
#include "scheduler_bmi270.h"
#include "bmi270.h"
#include "gatt_db.h"


typedef enum leTimerEvents
{
  UF_EVENT,
  COMP0_EVENT,
  COMP1_EVENT,
  I2C_TRANSFER_EVENT,
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
    BMI270_STATE_INIT_COMPLETE
} bmi270InitOperationState_e;


#define LETIMER0_UF    (1U << UF_EVENT)
#define LETIMER0_COMP0 (1U << COMP0_EVENT)
#define LETIMER0_COMP1 (1U << COMP1_EVENT)
#define I2C_TRANSFER_DONE (1U << I2C_TRANSFER_EVENT)

static bmi270InitOperationState_e currentInitStateMachineState = BMI270_START_INITIALIZATION;

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
          if ((status_flags == sl_bt_gatt_server_client_config)&&(characteristic == gattdb_temperature_measurement))
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
                      bmi270InitStatus = BMI270_INIT_SUCCESSFUL;
                  } else {
                      bmi270InitStatus = BMI270_INIT_FAILED;
                      currentInitStateMachineState = BMI270_START_INITIALIZATION;
                  }
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
