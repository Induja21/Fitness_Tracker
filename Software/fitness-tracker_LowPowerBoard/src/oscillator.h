/***********************************************************************
* @file cmu.h
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

#ifndef SRC_OSCILLATOR_H_
#define SRC_OSCILLATOR_H_


/* -------------------------------------------------------------------------------------
 * cmuClockInit()
 * ------------------------------------------------------------------------------------
 * @Purpose : This function initializes CMU Clock based on the Power mode set
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/

void cmuClockInit(void);

#endif /* SRC_OSCILLATOR_H_ */
