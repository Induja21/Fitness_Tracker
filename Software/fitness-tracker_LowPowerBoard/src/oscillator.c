/***********************************************************************
* @file cmu.c
* @version 0.0.1
* @brief This file has implementations related to all cmu clock
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
* - em_cmu.h used for cmu clock functionality
* -
*
* @copyright All rights reserved. Distribution allowed only for the
* use of assignment grading. Use of code excerpts allowed at the
* discretion of author. Contact for permission.
*/
#include "em_cmu.h"
#include "timer.h"
#include "../app.h"
/* -------------------------------------------------------------------------------------
 * cmuClockInit()
 * ------------------------------------------------------------------------------------
 * @Purpose : This function initializes CMU Clock based on the Power mode set
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/

void cmuClockInit(void)
{
    // Enable necessary clocks
    #if LOWEST_ENERGY_MODE == EM3
        //Energy mode is EM3 and hence ULFRCO clock is selected
        CMU_ClockEnable(cmuOsc_ULFRCO, true);
        CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
    #elif (LOWEST_ENERGY_MODE == EM0 || LOWEST_ENERGY_MODE == EM1 || LOWEST_ENERGY_MODE == EM2)
        //For other Energy modes LFXO clock is selected
        CMU_ClockEnable(cmuOsc_LFXO, true);
        CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
    #else
        #error "Invalid energy mode selected. Please set ENERGY_MODE to EM0, EM1, EM2, or EM3."
    #endif

    CMU_ClockDivSet(cmuClock_LETIMER0, LETIMER_PRESCALAR);
    CMU_ClockEnable(cmuClock_LETIMER0, true);
}
