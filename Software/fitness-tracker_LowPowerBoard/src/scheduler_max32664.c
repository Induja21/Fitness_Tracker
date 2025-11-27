
/*
 * scheduler_max32664.c
 *
 *  Created on: 08-Nov-2025
 *      Author: nindu
 */
#include "gatt_db.h"
#include "timer.h"
#include "scheduler_max32664.h"
#include "max32664.h"
#include "ble.h"
#include "gpio.h"



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

typedef enum max32664InitOperationState
{
    MAX32664_START_APP_MODE_OPERATION,
    MAX32664_STATE_CLEAR_RESET_PIN,
    MAX32664_STATE_SET_RESET_PIN,
    MAX32664_SELECT_DEVICE_MODE,
    MAX32664_WAIT_FOR_DEVICE_MODE_SELECTION,
    MAX32664_SELECT_ALGO_MODE,
    MAX32664_WAIT_FOR_ALGO_MODE_SELECTION,
    MAX32664_STATE_APP_READ,
    MAX32664_STATE_APP_COMPLETE,
    MAX32664_SET_THRESHOLD_VALUE,
    MAX32664_WAIT_TO_SET_THRESHOLD_DATA,
    MAX32664_SET_REPORT_PERIOD_VALUE,
    MAX32664_ENABLE_SENSOR,
    MAX32664_WAIT_FOR_SENSOR_TO_ENABLE,
    MAX32664_ENABLE_AGC_ALGORITHM,
    MAX32664_READ_FIRST_TIME,
    MAX32664_WAIT_FOR_AGC_ENABLE,
    MAX32664_ENABLE_BPT_ALGO,
    MAX32664_WAIT_FOR_WEARABLE_ALGO_SUITE_TO_ENABLE,
    MAX32664_WAIT_FOR_INTERRUPT,
    MAX32664_START_GET_NO_OF_FIFO_BYTES,
    MAX32664_WAIT_GET_NO_OF_FIFO_BYTES,
    MAX32664_START_PERFORM_FIFO_READ,
    MAX32664_WAIT_FOR_FIFO_READ,
    MAX32664_PERFORM_FIFO_READ,
    MAX32664_READ_STATUS_BYTE,
    MAX32664_GET_NO_OF_FIFO_BYTES,
    MAX32664_PERFORM_READ,
    MAX32664_STATE_HUB_VERSION_READ,
    MAX32664_STATE_HUB_VERSION_READ_COMPLETE,
    MAX32664_WAIT_FOR_INIT_TO_COMPLETE,
    MAX32664_STATE_CONVERSION_COMPLETE,
    MAX32664_PERFORM_DUMMY_READ,
    MAX32664_STATE_DISPLAY_TEMP_DATA,
}max32664InitOperationState_e;



#define LETIMER0_UF    (1U << UF_EVENT)
#define LETIMER0_COMP0 (1U << COMP0_EVENT)
#define LETIMER0_COMP1 (1U << COMP1_EVENT)
#define I2C_TRANSFER_DONE (1U << I2C_TRANSFER_EVENT)
#define MAX_MFIO_INTERRUPT (1U << MAX_MFIO_EVENT)
#define BMI_INTERRUPT1 (1U << BMI_INT1_EVENT)



#define IS_BIT_SET(byte,pos) ((byte>>pos)&1)
static max32664InitOperationState_e currentInitStateMachineState = MAX32664_START_APP_MODE_OPERATION;
static max32664InitState_e max32664CurrentInitState = MAX32664_INIT_IDLE;
static void sendIndicationsOfMax32664version(uint8_t version);

static uint8_t noOfFiFoBytes=0;
void max32664StateMachine(sl_bt_msg_t *bleEvent)
{
  allEvents_t event = INVALID_EVENT;
  static uint8_t count=0;
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
         if ((status_flags == sl_bt_gatt_server_client_config)&&((characteristic == gattdb_heart_rate_measurement)|| (characteristic== gattdb_Steps)))
         {
             if (client_config_flags & sl_bt_gatt_indication)
             {
                 max32664CurrentInitState = MAX32664_INIT_IN_PROGRESS;
                 //Initiate App mode
                 max32664StartInitAppmode();

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
      }else if(bleEvent->data.evt_system_external_signal.extsignals & MAX_MFIO_INTERRUPT)
        {
          event = MAX_MFIO_EVENT;
        }
      break;
    default:
      break;
  }
    if(SL_BT_MSG_ID(bleEvent->header) == sl_bt_evt_system_external_signal_id)
      {
        switch (currentInitStateMachineState) {
          case MAX32664_START_APP_MODE_OPERATION:
            {
              if (event == COMP1_EVENT) {
                configureBioSensorHubResetPin();
                setBioSensorHubMfioPin();
                clearBioSensorHubResetPin();
                //Wait for 20ms
                timerWaitUs_interrupt(20000);
                currentInitStateMachineState = MAX32664_STATE_CLEAR_RESET_PIN;

              }
            }
            break;
          case MAX32664_STATE_CLEAR_RESET_PIN:
            if (event == COMP1_EVENT) {
                currentInitStateMachineState = MAX32664_STATE_SET_RESET_PIN;
                setBioSensorHubResetPin();
                //Wait for 1000ms
                 timerWaitUs_interrupt(1000000);
            }
            break;
          case MAX32664_STATE_SET_RESET_PIN:
            {
              if(event == COMP1_EVENT)
                {
                  configureBioSensorHubMfioPin();
                  currentInitStateMachineState = MAX32664_SELECT_DEVICE_MODE;
                  selectDeviceMode(MAX32664_MODE_APP);
                }

            }
            break;
          case MAX32664_SELECT_DEVICE_MODE:
            {
              if(event==I2C_TRANSFER_EVENT)
                {
                  currentInitStateMachineState = MAX32664_WAIT_FOR_DEVICE_MODE_SELECTION;
                  waitForDeviceModeSelection();
                }
            }
            break;

          case MAX32664_WAIT_FOR_DEVICE_MODE_SELECTION:
            {
              if (event == COMP1_EVENT) {
                  currentInitStateMachineState=MAX32664_STATE_APP_READ;
                  readDeviceMode();

              }
            }
            break;


          case MAX32664_STATE_APP_READ:
            {
              if(event==I2C_TRANSFER_EVENT)
              {
                  //Read the result
                  const uint8_t* dataRead = NULL;
                  uint8_t buffsize= getLastReadBuffer(&dataRead);
                  if((buffsize==2) && (dataRead[0]==0x00) && (dataRead[1]==0x00))
                  {
                    currentInitStateMachineState = MAX32664_STATE_HUB_VERSION_READ;
                    readSensorHubVersion();

                  }
              }
                  else
                  {
                      max32664CurrentInitState = MAX32664_INIT_FAILED;
                    // Failed, reset state machine
                      currentInitStateMachineState=MAX32664_START_APP_MODE_OPERATION;

                  }

              }
          break;
          case MAX32664_STATE_HUB_VERSION_READ:
          {
            if(event==I2C_TRANSFER_EVENT)
            {
            const uint8_t* dataRead = NULL;
            uint8_t buffsize= getLastReadBuffer(&dataRead);
            if(buffsize==4)
            {
                 //Check If valid version
               if(!isAValidHubVersion())
               {
                   max32664CurrentInitState = MAX32664_INIT_FAILED;
                 // Failed, reset state machine
                 currentInitStateMachineState=MAX32664_START_APP_MODE_OPERATION;
               }
               else
               {
                   currentInitStateMachineState=MAX32664_SELECT_ALGO_MODE;

                   float version = getHubVersion();
                   selectAlgoMode();
               }


             }
           }
          }
        break;

          case  MAX32664_SELECT_ALGO_MODE:
            {
              if(event==I2C_TRANSFER_EVENT)
                {
                  currentInitStateMachineState=MAX32664_WAIT_FOR_ALGO_MODE_SELECTION;
                  waitForAlgoModeSelection();
                }
            }
            break;
          case MAX32664_WAIT_FOR_ALGO_MODE_SELECTION:
            {
              if(event==COMP1_EVENT)
                {

                  currentInitStateMachineState=MAX32664_SET_THRESHOLD_VALUE;
                  setThresholdData(0x0F);


                }
            }
            break;

          case MAX32664_SET_THRESHOLD_VALUE:
            {
              if(event==I2C_TRANSFER_EVENT)
                {
                  currentInitStateMachineState=MAX32664_WAIT_TO_SET_THRESHOLD_DATA;
                  waitToSetThresholdData();
                }
            }
            break;
          case MAX32664_WAIT_TO_SET_THRESHOLD_DATA:
            if(event==COMP1_EVENT)
              {
                currentInitStateMachineState=MAX32664_ENABLE_AGC_ALGORITHM;
                enableAGCAlgorithm();
              }
            break;
          case MAX32664_ENABLE_AGC_ALGORITHM:
            if(event==I2C_TRANSFER_EVENT)
              {
                currentInitStateMachineState=MAX32664_WAIT_FOR_AGC_ENABLE;
                waitForAGCAlgoToEnable();
              }
            break;

//          case MAX32664_WAIT_FOR_AGC_ENABLE:
//            if(event==COMP1_EVENT)
//              {
//                currentInitStateMachineState=MAX32664_ENABLE_WEARABLE_SUITE;
//                enableWearableAlgoSuite();
//              }
//            break;
//          case MAX32664_ENABLE_WEARABLE_SUITE:
//            {
//            if(event == I2C_TRANSFER_EVENT)
//              {
//                currentInitStateMachineState=MAX32664_WAIT_FOR_WEARABLE_ALGO_SUITE_TO_ENABLE;
//                waitForWearableAlgoSuiteToEnable();
//              }
//            }
//            break;
          case MAX32664_WAIT_FOR_AGC_ENABLE:
            {
            if(event == COMP1_EVENT)
              {
                currentInitStateMachineState=MAX32664_ENABLE_SENSOR;
                max32664ConfigInterrupts();
                enableSensor();
              }
            }
            break;
          case MAX32664_ENABLE_SENSOR:
            {
              if(event == I2C_TRANSFER_EVENT)
                {
                  currentInitStateMachineState=MAX32664_WAIT_FOR_SENSOR_TO_ENABLE;
                  waitForSensorToEnable();
                }
            }
            break;
          case MAX32664_WAIT_FOR_SENSOR_TO_ENABLE:
            {
              if(event == COMP1_EVENT)
                {
                  currentInitStateMachineState=MAX32664_ENABLE_BPT_ALGO;
                  enableBPTAlgoSuite();
                }
            }
            break;
          case MAX32664_ENABLE_BPT_ALGO:
            if(event==I2C_TRANSFER_EVENT)
              {
                currentInitStateMachineState=MAX32664_WAIT_FOR_WEARABLE_ALGO_SUITE_TO_ENABLE;
                waitForBPTAlgoSuiteToEnable();

              }
            break;
          case MAX32664_WAIT_FOR_WEARABLE_ALGO_SUITE_TO_ENABLE:
            {
              if(event == COMP1_EVENT)
                {
                  currentInitStateMachineState=MAX32664_WAIT_FOR_INIT_TO_COMPLETE;
                  waitForInitComplete();

                }
            }
            break;

          case MAX32664_WAIT_FOR_INIT_TO_COMPLETE:
            {
              if(event == COMP1_EVENT)
                {
                  currentInitStateMachineState=MAX32664_READ_STATUS_BYTE;
                  readStatusByte();


                }

            }
            break;
          case MAX32664_READ_STATUS_BYTE:
            {
              if(event == I2C_TRANSFER_EVENT)
                {
                  //Get the status byte
                  uint8_t statusByte = getStatusByte();
                  if(IS_BIT_SET(statusByte,3)==1 || count<=1)
                    {
                      currentInitStateMachineState=MAX32664_START_GET_NO_OF_FIFO_BYTES;
                      startreadNoOfSamplesinFiFo();
                      count++;
                    }
                  else
                    {
                      currentInitStateMachineState=MAX32664_WAIT_FOR_INTERRUPT;

                    }

                }
            }
            break;
          case MAX32664_START_GET_NO_OF_FIFO_BYTES:
            {
              if(event==I2C_TRANSFER_EVENT)
                {
                  currentInitStateMachineState=MAX32664_WAIT_GET_NO_OF_FIFO_BYTES;
                  i2cDelayForReadOperation();
                }

            }
            break;
          case MAX32664_WAIT_GET_NO_OF_FIFO_BYTES:
            {
              if(event==COMP1_EVENT)
                {
                  currentInitStateMachineState=MAX32664_GET_NO_OF_FIFO_BYTES;
                  performReadOfNofSamplesInFiFo();
                }
            }
            break;
          case MAX32664_GET_NO_OF_FIFO_BYTES:
            {
              if(event == I2C_TRANSFER_EVENT)
                {
                  noOfFiFoBytes = getNoOfSamplescurrentlyAvailableInFifo();
                  currentInitStateMachineState=MAX32664_START_PERFORM_FIFO_READ;
                  startperformSensorRead();
                }

            }
            break;
          case MAX32664_START_PERFORM_FIFO_READ:
            {
              if(event==I2C_TRANSFER_EVENT)
                {
                  currentInitStateMachineState=MAX32664_WAIT_FOR_FIFO_READ;
                  i2cDelayForReadOperation();
                }
            }
            break;
          case MAX32664_WAIT_FOR_FIFO_READ:
            {
              if(event==COMP1_EVENT)
                {
                  currentInitStateMachineState=MAX32664_PERFORM_FIFO_READ;
                  performSensorReadOperation(noOfFiFoBytes*23);
                }

            }

            break;

          case MAX32664_PERFORM_FIFO_READ:
            {
              if(event==I2C_TRANSFER_EVENT)
                {
                  parseAlgoData();
                  checkIfDataIsValid();
                  currentInitStateMachineState=MAX32664_WAIT_FOR_INTERRUPT;
                }

            }
            break;

          case MAX32664_WAIT_FOR_INTERRUPT:
            {
              if(event==MAX_MFIO_EVENT)
                {
                  currentInitStateMachineState=MAX32664_READ_STATUS_BYTE;
                  readStatusByte();
                }
            }

            break;


          default:
            break;

        }

      }

}
max32664InitState_e getLatestInitState()
{
  return max32664CurrentInitState;

}


/* -------------------------------------------------------------------------------------
 * sendIndicationsOfTemperature
 * ------------------------------------------------------------------------------------
 * @Purpose : This function prepares and transmits temperature data via BLE indications.
 *            It converts the temperature value into the IEEE 11073 32-bit floating-point
 *            format, updates the GATT database with the new value, and sends an indication
 *            to the connected BLE client if the conditions are met. Additionally, it logs
 *            errors and displays the temperature on a display if the operation is successful.
 * @Param   : float temperature - The temperature value to be sent and displayed.
 * @Return  : void
 *-------------------------------------------------------------------------------------*/
static void sendIndicationsOfMax32664version(uint8_t version)
{
  uint8_t heart_rate_buffer[2] = {0};
  heart_rate_buffer[0] = 0x00; // Flags byte (0 for Celsius)
  // Convert version to IEEE-11073 format
  //uint32_t ieee11073_temp = convertToIEEE11073(version);
  memcpy(&heart_rate_buffer[1], &version, sizeof(version));
  // Update GATT database with new version value
   sl_bt_gatt_server_write_attribute_value(
       gattdb_heart_rate_measurement,  // Handle from gatt_db.h
       0,  // Offset (start of characteristic value)
       sizeof(heart_rate_buffer),
       heart_rate_buffer
   );

   //Get the connection handle
   ble_data_struct_t* bleDataPtr = getBleData();
   // Send indication if conditions are met
   if (bleDataPtr->connection_open && bleDataPtr->ok_to_send_heartrate_indications && !bleDataPtr->indication_in_flight_heartrate) {
       sl_bt_gatt_server_send_indication(
           bleDataPtr->connection_handle, // Connection handle
           gattdb_heart_rate_measurement, // Handle from gatt_db.h
           sizeof(heart_rate_buffer), // Length
           heart_rate_buffer // Data
       );


   }


}

