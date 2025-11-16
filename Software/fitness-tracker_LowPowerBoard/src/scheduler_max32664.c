
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
static void sendIndicationsOfMax32664version(float version);

/* -------------------------------------------------------------------------------------
 * convertToIEEE11073
 * ------------------------------------------------------------------------------------
 * @Purpose : This function converts a floating-point temperature value into the
 *            IEEE 11073 32-bit floating-point format. It encodes the temperature
 *            with an exponent of -2 (scaling factor of 100) to preserve two decimal
 *            places while maintaining a compact representation.
 * @Param   : float temperature - The temperature value to be converted.
 * @Return  : uint32_t - The IEEE 11073 formatted 32-bit representation of the temperature.
 *-------------------------------------------------------------------------------------*/
static uint32_t convertToIEEE11073(float temperature) {
    uint8_t exponent = 0xFE; // Exponent of -2 (i.e., divide by 100)
    int32_t mantissa = (int32_t)(temperature * 100); // Scale to 2 decimal places

    uint32_t ieee11073_value = ((uint32_t)exponent << 24) | (mantissa & 0x00FFFFFF);

    return ieee11073_value;
}
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
                  setBioSensorHubMfioPin();
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
                      sendIndicationsOfMax32664version(23.67);
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
                     sendIndicationsOfMax32664version(28.67);
                     max32664CurrentInitState = MAX32664_INIT_FAILED;
                   // Failed, reset state machine
                   currentInitStateMachineState=MAX32664_START_APP_MODE_OPERATION;
                 }
                 else
                 {
                     max32664CurrentInitState = MAX32664_INIT_SUCCESSFUL;
                     float version = getHubVersion();
                     sendIndicationsOfMax32664version(23.67);
                     //sendIndicationsOfMax32664version(version);
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
static void sendIndicationsOfMax32664version(float version)
{
  uint8_t htm_version_buffer[5] = {0};
  htm_version_buffer[0] = 0x00; // Flags byte (0 for Celsius)
  // Convert version to IEEE-11073 format
  uint32_t ieee11073_temp = convertToIEEE11073(version);
  memcpy(&htm_version_buffer[1], &ieee11073_temp, sizeof(ieee11073_temp));
  // Update GATT database with new version value
   sl_status_t sc = sl_bt_gatt_server_write_attribute_value(
       gattdb_temperature_measurement,  // Handle from gatt_db.h
       0,  // Offset (start of characteristic value)
       sizeof(htm_version_buffer),
       htm_version_buffer
   );

   //Get the connection handle
   ble_data_struct_t* bleDataPtr = getBleData();
   // Send indication if conditions are met
   if (bleDataPtr->connection_open && bleDataPtr->ok_to_send_htm_indications && !bleDataPtr->indication_in_flight) {
       sc = sl_bt_gatt_server_send_indication(
           bleDataPtr->connection_handle, // Connection handle
           gattdb_temperature_measurement, // Handle from gatt_db.h
           sizeof(htm_version_buffer), // Length
           htm_version_buffer // Data
       );


   }


}

