/***********************************************************************
* @file irq.h
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
#ifndef SRC_IRQ_H_
#define SRC_IRQ_H_


/* -------------------------------------------------------------------------------------
 * LETIMER0_IRQHandler
 * ------------------------------------------------------------------------------------
 * @Purpose : This is LETIMER0 IRQHandler
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/

void LETIMER0_IRQHandler (void);
/* -------------------------------------------------------------------------------------
 * I2C0_IRQHandler
 * ------------------------------------------------------------------------------------
 * @Purpose : This is I2C0_IRQHandler
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void I2C0_IRQHandler(void);
/* -------------------------------------------------------------------------------------
 * letimerMilliseconds
 * ------------------------------------------------------------------------------------
 * @Purpose : Returns the elapsed time in milliseconds since bootup
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/

uint64_t letimerMilliseconds(void);
#endif /* SRC_IRQ_H_ */
