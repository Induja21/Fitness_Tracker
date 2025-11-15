/***********************************************************************
* @file timer.h
* @version 0.0.1
* @brief This file has implementations related to all timer functions
*
* @author Induja Narayanan, Induja.Narayanan@Colorado.edu
* @date Jan 29,2025
*
* @institution University of Colorado Boulder (UCB)
* @course ECEN 5823-001: IoT Embedded Firmware (Fall 2025)
* @instructor Chris Choi
*
* @assignment ecen5823-assignment2-IndujaNarayanan
* @due Jan 30,2025
*
* @resources Utilized Silicon Labs' EMLIB peripheral libraries to
* implement functionality.
* - em_letimer.h used for timer functionality
* -
*
* @copyright All rights reserved. Distribution allowed only for the
* use of assignment grading. Use of code excerpts allowed at the
* discretion of author. Contact for permission.
*/

#ifndef SRC_TIMER_H_
#define SRC_TIMER_H_

#include "../app.h"
#include <stdint.h>
// LED On time (in milliseconds)
#define LETIMER_ON_TIME_MS    175

// Total blink period (in milliseconds)
#define LETIMER_PERIOD_MS     3000

//Macro defining the success case of the timer wait functionality
#define TIMER_WAIT_SUCCESS 0
//Macro defining the failure case of the timer wait functionality
#define TIMER_WAIT_FAILURE -1


// Oscillator selection based on energy mode
#if LOWEST_ENERGY_MODE == EM3
    #define LETIMER_CLOCK_FREQUENCY 1000  // ULFRCO (1 KHz)
    //Prescalar value set for LETIMER0
    #define LETIMER_PRESCALAR     1
#elif (LOWEST_ENERGY_MODE == EM0 || LOWEST_ENERGY_MODE == EM1 || LOWEST_ENERGY_MODE == EM2)
    #define LETIMER_CLOCK_FREQUENCY 32768 // LFXO (32.768 KHz)
    //Prescalar value set for LETIMER0
    #define LETIMER_PRESCALAR     4
#else
    #error "Invalid energy mode selected. Please set ENERGY_MODE to EM0, EM1, EM2, or EM3."
#endif

//Actual LETIMER0 Clock Frequency
#define ACTUAL_LETIMER_CLOCK_FREQUENCY  LETIMER_CLOCK_FREQUENCY/LETIMER_PRESCALAR


//Value to load for COMP0
#define VALUE_TO_LOAD_COMP0   (LETIMER_PERIOD_MS * ACTUAL_LETIMER_CLOCK_FREQUENCY)/1000

//Minimum measurement provided by the timer
#define TIMER_MIN_MEASUREMENT (1.0e6 / (double)ACTUAL_LETIMER_CLOCK_FREQUENCY)

//Maximum measurement provided by the timer
//Since it is a 16 bit down counter max value is 65535, So max time period that function can wait is 65535*(1/ACTUAL_LETIMER_CLOCK_FREQUENCY)*1000000 rounded off
#define TIMER_MAX_MEASUREMENT TIMER_MIN_MEASUREMENT*65535



/* -------------------------------------------------------------------------------------
 * timerWaitUs()
 * ------------------------------------------------------------------------------------
 * @Purpose : This function waits for the specified amount of time
 * @Param   : Time period to wait in micro seconds
 * @Return  : -1 if the provided time delay cannot be done.
 *          : 0 on success
 *-------------------------------------------------------------------------------------*/
int8_t timerWaitUs_polled(uint32_t timeToWaitInUs);
/* -------------------------------------------------------------------------------------
 * timerWaitUs_interrupt()
 * ------------------------------------------------------------------------------------
 * @Purpose : This function calculates and initiate the COMP1 event with required ticks to wait
 * @Param   : Time period to wait in micro seconds
 * @Return  : -1 if the provided time delay cannot be done.
 *          : 0 on success
 *-------------------------------------------------------------------------------------*/

int8_t timerWaitUs_interrupt(uint32_t timeToWaitInUs);
/* -------------------------------------------------------------------------------------
 * leTimerInit()
 * ------------------------------------------------------------------------------------
 * @Purpose : This function initializes LETIME0
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/

void leTimerInit(void);

void turnOff_UF_Event(void);

void turnOn_UF_Event(void);

#endif /* SRC_TIMER_H_ */
