/***********************************************************************
* @file irq.c
* @version 0.0.1
* @brief This file has implementations related to all irq handlers
*
* @author Induja Narayanan, Induja.Narayanan@Colorado.edu
* @date Feb 13,2025
*
* @institution University of Colorado Boulder (UCB)
* @course ECEN 5823-001: IoT Embedded Firmware (Fall 2025)
* @instructor Chris Choi
*
* @assignment ecen5823-assignment2-IndujaNarayanan
* @due Feb 13,2025
*
* @resources Utilized Silicon Labs' EMLIB peripheral libraries to
* implement functionality.
* -
* -
*
* @copyright All rights reserved. Distribution allowed only for the
* use of assignment grading. Use of code excerpts allowed at the
* discretion of author. Contact for permission.
*/
#include "em_letimer.h"
#include "em_i2c.h"
#include "sl_power_manager.h"
#include "log.h"
#include "scheduler.h"

//Time value elapsed since bootup
static uint64_t timeElapsedInMilliSeconds = 0;

// Max time for Underflow interrupt
#define TIME_FOR_UF_INTERRUPT 3000
/* -------------------------------------------------------------------------------------
 * LETIMER0_IRQHandler
 * ------------------------------------------------------------------------------------
 * @Purpose : This is LETIMER0 IRQHandler
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void LETIMER0_IRQHandler(void)
{
    uint32_t flags = LETIMER_IntGetEnabled(LETIMER0);
    LETIMER_IntClear(LETIMER0, flags);

    if (flags & LETIMER_IF_COMP0) {
        schedulerSetEventComp0();
    }

    if (flags & LETIMER_IF_COMP1) {
        schedulerSetEventComp1();
    }

    if(flags & LETIMER_IF_UF)
    {
        timeElapsedInMilliSeconds+=TIME_FOR_UF_INTERRUPT;
        schedulerSetEventUF();
    }
}

/* -------------------------------------------------------------------------------------
 * I2C0_IRQHandler
 * ------------------------------------------------------------------------------------
 * @Purpose : This is I2C0_IRQHandler
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void I2C0_IRQHandler(void) {
    I2C_TransferReturn_TypeDef i2cTransferStatus;
    i2cTransferStatus = I2C_Transfer(I2C0);
    if (i2cTransferStatus == i2cTransferInProgress) {
           return;
      }
    if (i2cTransferStatus == i2cTransferDone) {
           schedulerSetEventI2c();
    } else if (i2cTransferStatus < 0) {
            LOG_ERROR("I2C Transfer Failed! Error Code: %d", i2cTransferStatus);
      }
}
/* -------------------------------------------------------------------------------------
 * letimerMilliseconds
 * ------------------------------------------------------------------------------------
 * @Purpose : Returns the elapsed time in milliseconds since bootup
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/

uint64_t letimerMilliseconds(void)
{
  return timeElapsedInMilliSeconds;

}
