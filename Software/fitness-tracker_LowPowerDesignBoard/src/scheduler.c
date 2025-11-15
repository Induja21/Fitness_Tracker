/***********************************************************************
* @file scheduler.c
* @version 0.0.1
* @brief This file has implementations related to schedule processing of events
*
* @author Induja Narayanan, Induja.Narayanan@Colorado.edu
* @date Feb 03,2025
*
* @institution University of Colorado Boulder (UCB)
* @course ECEN 5823-001: IoT Embedded Firmware (Fall 2025)
* @instructor Chris Choi
*
* @assignment ecen5823-assignment3-IndujaNarayanan
* @due Feb 6,2025
*
* @resources Utilized Silicon Labs' EMLIB peripheral libraries to
* implement functionality.
* -
*
* @copyright All rights reserved. Distribution allowed only for the
* use of assignment grading. Use of code excerpts allowed at the
* discretion of author. Contact for permission.
*/

#include "scheduler.h"
#include <stdint.h>
#include <stdbool.h>
#include "em_core.h"
#include "i2c.h"
#include "timer.h"
#include "sl_power_manager.h"
#include "gatt_db.h"
#include "lcd.h"
#include "ble.h"
#define INCLUDE_LOG_DEBUG 1
#include "log.h"
// Include logging specifically for this .c file

typedef enum tempOperationState
{
    TEMP_STATE_IDLE,
    TEMP_STATE_WRITE_CMD,
    TEMP_STATE_WRITE_COMPLETE,
    TEMP_STATE_CONVERSION_COMPLETE,
    TEMP_STATE_DISPLAY_TEMP_DATA,
}tempOperationState_t;


typedef struct stateMachine{
  tempOperationState_t currentState;
  float temperature;
}stateMachine_t;


static stateMachine_t stateMachine = {TEMP_STATE_IDLE, 0.0};

/* -------------------------------------------------------------------------------------
 * Global Variables
 * -------------------------------------------------------------------------------------
 * @Purpose : Holds event flags for the scheduler.
 * @Notes   : This variable is accessed and modified inside critical sections
 *            to ensure atomicity and prevent race conditions.
 * -------------------------------------------------------------------------------------*/
static volatile uint32_t eventFlags = 0;

/* -------------------------------------------------------------------------------------
 * schedulerInit()
 * -------------------------------------------------------------------------------------
 * @Purpose : Initializes the scheduler by clearing all event flags.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void schedulerInit()
{
  eventFlags = 0;
}

/* -------------------------------------------------------------------------------------
 * schedulerSetEventUF()
 * -------------------------------------------------------------------------------------
 * @Purpose : Sets the underflow event for LETIMER0 in the scheduler.
 * @Param   : None
 * @Return  : None
 * @Notes   : This function is called inside an ISR when a LETIMER0 underflow occurs.
 *            It ensures thread safety by using a critical section.
 * -------------------------------------------------------------------------------------*/
void schedulerSetEventUF(void)
{
  // Enter critical section
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  sl_bt_external_signal(LETIMER0_UF);
  // Exit critical section
  CORE_EXIT_CRITICAL();
}

/* -------------------------------------------------------------------------------------
 * schedulerSetEventComp0()
 * -------------------------------------------------------------------------------------
 * @Purpose : Sets the compare match 0 event for LETIMER0 in the scheduler.
 * @Param   : None
 * @Return  : None
 * @Notes   : Called when LETIMER0 Compare Match 0 interrupt occurs.
 * -------------------------------------------------------------------------------------*/
void schedulerSetEventComp0(void)
{
  // Enter critical section
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  sl_bt_external_signal(LETIMER0_COMP0);
  // Exit critical section
  CORE_EXIT_CRITICAL();
}

/* -------------------------------------------------------------------------------------
 * schedulerSetEventComp1()
 * -------------------------------------------------------------------------------------
 * @Purpose : Sets the compare match 1 event for LETIMER0 in the scheduler.
 * @Param   : None
 * @Return  : None
 * @Notes   : Called when LETIMER0 Compare Match 1 interrupt occurs.
 * -------------------------------------------------------------------------------------*/
void schedulerSetEventComp1(void)
{
  // Enter critical section
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  sl_bt_external_signal(LETIMER0_COMP1);
  // Exit critical section
  CORE_EXIT_CRITICAL();
}


/* -------------------------------------------------------------------------------------
 * schedulerSetEventComp1()
 * -------------------------------------------------------------------------------------
 * @Purpose : Sets the compare match 1 event for LETIMER0 in the scheduler.
 * @Param   : None
 * @Return  : None
 * @Notes   : Called when LETIMER0 Compare Match 1 interrupt occurs.
 * -------------------------------------------------------------------------------------*/
void schedulerSetEventI2c(void)
{
  // Enter critical section
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  sl_bt_external_signal(I2C_TRANSFER_DONE);
  // Exit critical section
  CORE_EXIT_CRITICAL();
}

/* -------------------------------------------------------------------------------------
 * resetStateMachine
 * ------------------------------------------------------------------------------------
 * @Purpose : This function resets the state machine by setting the current state
 *            to TEMP_STATE_IDLE. This ensures that the system starts from a known
 *            idle state before processing further state transitions.
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
static void resetStateMachine(void)
{
  stateMachine.currentState = TEMP_STATE_IDLE;
}

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
static void sendIndicationsOfTemperature(float temperature)
{
  uint8_t htm_temperature_buffer[5] = {0};
  htm_temperature_buffer[0] = 0x00; // Flags byte (0 for Celsius)
  // Convert temperature to IEEE-11073 format
  uint32_t ieee11073_temp = convertToIEEE11073(temperature);
  memcpy(&htm_temperature_buffer[1], &ieee11073_temp, sizeof(ieee11073_temp));
  // Update GATT database with new temperature value
   sl_status_t sc = sl_bt_gatt_server_write_attribute_value(
       gattdb_temperature_measurement,  // Handle from gatt_db.h
       0,  // Offset (start of characteristic value)
       sizeof(htm_temperature_buffer),
       htm_temperature_buffer
   );

   if (sc != SL_STATUS_OK) {
       LOG_ERROR("Failed to update GATT database %d", (int)sc);
   }
   //Get the connection handle
   ble_data_struct_t* bleDataPtr = getBleData();
   // Send indication if conditions are met
   if (bleDataPtr->connection_open && bleDataPtr->ok_to_send_htm_indications && !bleDataPtr->indication_in_flight) {
       sc = sl_bt_gatt_server_send_indication(
           bleDataPtr->connection_handle, // Connection handle
           gattdb_temperature_measurement, // Handle from gatt_db.h
           sizeof(htm_temperature_buffer), // Length
           htm_temperature_buffer // Data
       );

       if (sc != SL_STATUS_OK) {
           LOG_ERROR("Failed to send indication %d", (int)sc);
       }
       displayPrintf(DISPLAY_ROW_TEMPVALUE,"Temp=%.2f",temperature);
   }


}


/* -------------------------------------------------------------------------------------
 * temperatureStateMachine
 * ------------------------------------------------------------------------------------
 * @Purpose : This function implements a state machine to manage the temperature measurement
 *            process using the Si7021 sensor. It handles powering on the sensor, initiating
 *            temperature measurement, waiting for conversion, reading the temperature data,
 *            and displaying the result. The state machine transitions between states based
 *            on events and performs the necessary actions for each state.
 * @Param   : sl_bt_msg_t event - The event that triggers state transitions. Events can include
 *            UF_EVENT, COMP1_EVENT, I2C_TRANSFER_EVENT, etc.
 * @Return  : None
 *-------------------------------------------------------------------------------------*/

void temperatureStateMachine(sl_bt_msg_t *bleEvent)
{
  allEvents_t event = INVALID_EVENT;
  // Map BLE stack events to state machine events
  switch (SL_BT_MSG_ID(bleEvent->header)) {
      case sl_bt_evt_connection_opened_id:
          break;

      case sl_bt_evt_connection_closed_id:
          turnOff_UF_Event(); // Stop the 3-second timer
         // powerOffSi7021(); // Power Off the temp sensor to reduce current consumption
          resetStateMachine();//Reset the state machine current state
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
                   // Indications are enabled for temperature measurement
                     turnOn_UF_Event();
                   //sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
               }
               else
               {
                   // Indications are disabled for temperature measurement
                   turnOff_UF_Event(); // Stop the 3-second timer
                  // powerOffSi7021(); // Power Off the temp sensor to reduce current consumption
                   resetStateMachine();//Reset the state machine current state
               }

           }


        }
          break;

      case sl_bt_evt_system_external_signal_id:
          if (bleEvent->data.evt_system_external_signal.extsignals & I2C_TRANSFER_DONE) {
              event = I2C_TRANSFER_EVENT;
          } else if (bleEvent->data.evt_system_external_signal.extsignals & LETIMER0_UF) {
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
  switch (stateMachine.currentState) {
      case TEMP_STATE_IDLE:
          if (event == UF_EVENT) {
              // Action: Power On
             // powerOnSi7021();
              waitForPowerOn();
              stateMachine.currentState = TEMP_STATE_WRITE_CMD;
          }
          break;

      case TEMP_STATE_WRITE_CMD:
          if (event == COMP1_EVENT) {
              // Action: Send Write Command
             // sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1); // Restrict to EM1
              initiateTemperatureMeasurement();
              stateMachine.currentState = TEMP_STATE_WRITE_COMPLETE;
          }
          else
            {
              stateMachine.currentState = TEMP_STATE_IDLE;
            }
          break;

      case TEMP_STATE_WRITE_COMPLETE:
          if (event == I2C_TRANSFER_EVENT) {
              // Action: Wait for Conversion
             // sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
              waitForConversion();
              stateMachine.currentState = TEMP_STATE_CONVERSION_COMPLETE;
          }
          else
            {
              stateMachine.currentState = TEMP_STATE_IDLE;
            }
          break;

      case TEMP_STATE_CONVERSION_COMPLETE:
          if (event == COMP1_EVENT) {
              // Action: Send Read Command
             // sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1); // Restrict to EM1
              sendReadCmdForTempData();
              stateMachine.currentState = TEMP_STATE_DISPLAY_TEMP_DATA;
          }
          else
            {
              stateMachine.currentState = TEMP_STATE_IDLE;
            }
          break;

      case TEMP_STATE_DISPLAY_TEMP_DATA:
          if (event == I2C_TRANSFER_EVENT) {
              // Action: Get Temperature
             // sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
              stateMachine.temperature = getConvertedTempData();
              //LOG_INFO("Temperature: %.2f C", stateMachine.temperature);
              stateMachine.currentState = TEMP_STATE_IDLE;
             // powerOffSi7021();

              sendIndicationsOfTemperature(stateMachine.temperature);



          }
          else
            {
              stateMachine.currentState = TEMP_STATE_IDLE;
            }
          break;

      default:
          stateMachine.currentState = TEMP_STATE_IDLE;
          LOG_ERROR("Invalid State!");
          break;
    }
  }
}

/* -------------------------------------------------------------------------------------
 * schedulerGetEvent()
 * -------------------------------------------------------------------------------------
 * @Purpose : Retrieves the next pending event from the scheduler.
 * @Param   : None
 * @Return  : The next event (if any); returns 0 if no events are pending.
 * @Notes   : - Ensures only one event is retrieved at a time.
 *            - Uses a loop to find the lowest set event bit.
 *            - Clears the event after retrieval to avoid re-processing.
 * -------------------------------------------------------------------------------------*/
//uint32_t schedulerGetEvent(void) {
//    // Enter critical section
//    CORE_DECLARE_IRQ_STATE;
//    CORE_ENTER_CRITICAL();
//
//    uint32_t event = 0;
//
//    // Find the lowest set event bit
//    for (uint32_t i = 0; i < 32; i++) {
//        uint32_t mask = (1 << i);
//        if (eventFlags & mask) {
//            event = mask;
//            eventFlags &= ~mask;  // Clear the retrieved event
//            break; // Exit loop after retrieving one event
//        }
//    }
//
//    // Exit critical section
//    CORE_EXIT_CRITICAL();
//
//    return event;  // Return the retrieved event (or 0 if no event is set)
//}
