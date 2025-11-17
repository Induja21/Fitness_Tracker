/***********************************************************************
* @file scheduler.h
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

#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_
#include <stdint.h>
#include "sl_bt_api.h"

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


#define LETIMER0_UF    (1U << UF_EVENT)
#define LETIMER0_COMP0 (1U << COMP0_EVENT)
#define LETIMER0_COMP1 (1U << COMP1_EVENT)
#define I2C_TRANSFER_DONE (1U << I2C_TRANSFER_EVENT)
#define MAX_MFIO_INTERRUPT (1U << MAX_MFIO_EVENT)
#define BMI_INTERRUPT1 (1U << BMI_INT1_EVENT)


/* -------------------------------------------------------------------------------------
 * schedulerSetEventComp0()
 * -------------------------------------------------------------------------------------
 * @Purpose : Sets the compare match 0 event for LETIMER0 in the scheduler.
 * @Param   : None
 * @Return  : None
 * @Notes   : Called when LETIMER0 Compare Match 0 interrupt occurs.
 * -------------------------------------------------------------------------------------*/
void schedulerSetEventComp0(void);
/* -------------------------------------------------------------------------------------
 * schedulerSetEventComp1()
 * -------------------------------------------------------------------------------------
 * @Purpose : Sets the compare match 1 event for LETIMER0 in the scheduler.
 * @Param   : None
 * @Return  : None
 * @Notes   : Called when LETIMER0 Compare Match 1 interrupt occurs.
 * -------------------------------------------------------------------------------------*/
void schedulerSetEventComp1(void);

/* -------------------------------------------------------------------------------------
 * schedulerSetEventUF()
 * -------------------------------------------------------------------------------------
 * @Purpose : Sets the underflow event for LETIMER0 in the scheduler.
 * @Param   : None
 * @Return  : None
 * @Notes   : This function is called inside an ISR when a LETIMER0 underflow occurs.
 *            It ensures thread safety by using a critical section.
 * -------------------------------------------------------------------------------------*/
void schedulerSetEventUF(void);

void schedulerSetEventI2c(void);

void schedulerSetEventPF5(void);
void schedulerSetEventPF6(void);
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
uint32_t schedulerGetEvent(void);
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
void temperatureStateMachine(sl_bt_msg_t *bleEvent);

#endif /* SRC_SCHEDULER_H_ */
