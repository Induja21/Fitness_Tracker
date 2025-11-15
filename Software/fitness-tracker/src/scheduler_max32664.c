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



typedef enum leTimerEvents
{
  UF_EVENT,
  COMP0_EVENT,
  COMP1_EVENT,
  I2C_TRANSFER_EVENT,
  INVALID_EVENT
}allEvents_t;

typedef enum max32664InitOperationState
{
    MAX32664_START_APP_MODE_OPERATION,
    MAX32664_STATE_SET_RESET_PIN,
    MAX32664_STATE_APP_READ,
    MAX32664_STATE_APP_COMPLETE,
    MAX32664_STATE_HUB_VERSION_READ,
    MAX32664_STATE_HUB_VERSION_READ_COMPLETE,
    MAX32664_STATE_CONVERSION_COMPLETE,
    MAX32664_STATE_DISPLAY_TEMP_DATA,
}max32664InitOperationState_e;



#define LETIMER0_UF    (1U << UF_EVENT)
#define LETIMER0_COMP0 (1U << COMP0_EVENT)
#define LETIMER0_COMP1 (1U << COMP1_EVENT)
#define I2C_TRANSFER_DONE (1U << I2C_TRANSFER_EVENT)




static max32664InitOperationState_e currentInitStateMachineState = MAX32664_START_APP_MODE_OPERATION;
static max32664InitState_e max32664CurrentInitState = MAX32664_INIT_IDLE;
void max32664StateMachine(sl_bt_msg_t *bleEvent)
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
                setBioSensorHubResetPin();
                //Wait for 1 second
                timerWaitUs_interrupt(10000);
                currentInitStateMachineState = MAX32664_STATE_SET_RESET_PIN;

              }
            }
            break;
          case MAX32664_STATE_SET_RESET_PIN:
            {
              if (event == COMP1_EVENT) {
                  //TODO: Un-comment below line
                 // setBioSensorHubMfioPin();
                  readDeviceMode();
                  currentInitStateMachineState=MAX32664_STATE_APP_READ;
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
                  if(buffsize==1)
                  {
                    currentInitStateMachineState=MAX32664_STATE_HUB_VERSION_READ;
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
                     max32664CurrentInitState = MAX32664_INIT_SUCCESSFUL;
                 }


               }
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


