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
#include "em_gpio.h"
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

void schedulerSetEventPF5(void)
{
  // Clear the interrupt flag
  uint32_t flags = GPIO_IntGet();
  GPIO_IntClear(flags);

  // Enter critical section
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  // Check if PF5 triggered the interrupt (interrupt source 5)
  if (flags & (1 << 5)) {
    sl_bt_external_signal(MAX_MFIO_INTERRUPT);
    GPIO_IntClear(1 << 5);
    NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  }
  // Exit critical section
  CORE_EXIT_CRITICAL();
}

void schedulerSetEventPF6(void)
{
  // Clear the interrupt flag
  uint32_t flags = GPIO_IntGet();
  GPIO_IntClear(flags);
  // Enter critical section
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  // Check if PF6 triggered the interrupt (interrupt source 6)
  if (flags & (1 << 6)) {
    sl_bt_external_signal(BMI_INTERRUPT1);
  }
  // Exit critical section
  CORE_EXIT_CRITICAL();
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
