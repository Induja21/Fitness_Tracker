/***********************************************************************
* @file timer.c
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
#include "timer.h"
#include "gpio.h"
#include "scheduler.h"
#include "em_letimer.h"
#include "sl_power_manager.h"
#define INCLUDE_LOG_DEBUG 1
#include "log.h"



/* -------------------------------------------------------------------------------------
 * timerWaitUs_interrupt()
 * ------------------------------------------------------------------------------------
 * @Purpose : This function calculates and initiate the COMP1 event with required ticks to wait
 * @Param   : Time period to wait in micro seconds
 * @Return  : -1 if the provided time delay cannot be done.
 *          : 0 on success
 *-------------------------------------------------------------------------------------*/
int8_t timerWaitUs_interrupt(uint32_t timeToWaitInUs)
{
  uint32_t clkfrequency = ACTUAL_LETIMER_CLOCK_FREQUENCY;
  uint64_t minmeasurement = 1000000 / clkfrequency;
  uint64_t maxmeasurement = minmeasurement*65535;
  //Check the minimum and max range supported by this function and also
  // check if the timer is capable of providing accurate delay the input timeToWaitInUs should always be a multiple of TIMER_MIN_MEASUREMENT
  if(timeToWaitInUs < minmeasurement || timeToWaitInUs > maxmeasurement)
  {
      //Return Error code
     return TIMER_WAIT_FAILURE;
  }
  uint32_t ticksToWait =  timeToWaitInUs/minmeasurement;

   uint32_t comp1_value = (LETIMER_CounterGet(LETIMER0) - ticksToWait) & 0xFFFF;
   LETIMER_CompareSet(LETIMER0, 1,comp1_value); //Set the comparator 1 the time to wait

  // Enable COMP1 interrupt
  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_COMP1);
  return TIMER_WAIT_SUCCESS;
}

/* -------------------------------------------------------------------------------------
 * timerWaitUs_polled()
 * ------------------------------------------------------------------------------------
 * @Purpose : This function waits for the specified amount of time
 * @Param   : Time period to wait in micro seconds
 * @Return  : -1 if the provided time delay cannot be done.
 *          : 0 on success
 *-------------------------------------------------------------------------------------*/
int8_t timerWaitUs_polled(uint32_t timeToWaitInUs)
{
    uint32_t clkfrequency = ACTUAL_LETIMER_CLOCK_FREQUENCY;
    uint64_t minmeasurement = 1000000 / clkfrequency;
    uint64_t maxmeasurement = minmeasurement*65535;
    //Check the minimum and max range supported by this function and also
    // check if the timer is capable of providing accurate delay the input timeToWaitInUs should always be a multiple of TIMER_MIN_MEASUREMENT
    if(timeToWaitInUs < minmeasurement || timeToWaitInUs > maxmeasurement)
    {
        //Return Error code
       return TIMER_WAIT_FAILURE;
    }
    uint32_t startCount = LETIMER_CounterGet(LETIMER0);
    uint32_t ticksToWait = timeToWaitInUs/TIMER_MIN_MEASUREMENT;
    uint32_t currentCount = LETIMER_CounterGet(LETIMER0);
    uint32_t elapsedTicks;
    while (true) {
        currentCount = LETIMER_CounterGet(LETIMER0);
        if (currentCount <= startCount) {
            elapsedTicks = startCount - currentCount;
        } else {
            elapsedTicks = (0xFFFF - currentCount) + startCount + 1;
        }

        if (elapsedTicks >= ticksToWait) {
            break;
        }
    }
    return TIMER_WAIT_SUCCESS;
}
/* -------------------------------------------------------------------------------------
 * leTimerInit()
 * ------------------------------------------------------------------------------------
 * @Purpose : This function initializes LETIME0
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void leTimerInit(void)
{

    // Configure LETIMER0
    LETIMER_Init_TypeDef letimer_init = LETIMER_INIT_DEFAULT;
    letimer_init.enable = false;
    letimer_init.comp0Top = true;  // COMP0 sets the top value

    LETIMER_Init(LETIMER0, &letimer_init);


    LETIMER_CompareSet(LETIMER0, 0, 0xFFFFFFFF); // Set the comparator to 3 seconds

    //Clear all IRQ flags in LETIMER0 IF Status register
    LETIMER_IntClear(LETIMER0, 0xFFFFFFFF);
    // Enable COMP0 and COMP1 interrupts
    NVIC_EnableIRQ(LETIMER0_IRQn);

    // Enable LETIMER0
    LETIMER_Enable(LETIMER0, true);

}

// Function to turn off the Underflow (UF) event
void turnOff_UF_Event(void) {
    // Disable the UF interrupt
    LETIMER_IntDisable(LETIMER0, LETIMER_IEN_UF);
}

// Function to turn on the Underflow (UF) event
void turnOn_UF_Event(void) {
    // Enable the UF interrupt
    LETIMER_IntEnable(LETIMER0, LETIMER_IEN_UF);
}
