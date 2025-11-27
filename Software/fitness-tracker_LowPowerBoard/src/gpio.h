/*
   gpio.h
  
    Created on: Dec 12, 2018
        Author: Dan Walkes

    Updated by Dave Sluiter Sept 7, 2020. moved #defines from .c to .h file.
    Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

    Editor: Feb 26, 2022, Dave Sluiter
    Change: Added comment about use of .h files.

 *
 * Student edit: Add your name and email address here:
 * @student    Induja Narayanan,Induja.Narayanan@Colorado.edu
 *
 
 */


// Students: Remember, a header file (a .h file) generally defines an interface
//           for functions defined within an implementation file (a .c file).
//           The .h file defines what a caller (a user) of a .c file requires.
//           At a minimum, the .h file should define the publicly callable
//           functions, i.e. define the function prototypes. #define and type
//           definitions can be added if the caller requires theses.


#ifndef SRC_GPIO_H_
#define SRC_GPIO_H_

#include <stdbool.h>


// Function prototypes
/* -------------------------------------------------------------------------------------
 * gpioInit()
 * -------------------------------------------------------------------------------------
 * @Purpose : Initializes GPIO settings, including drive strengths and modes.
 * @Param   : None
 * @Return  : None
 * @Notes   : - Configures drive strength settings for LED and sensor ports.
 *            - Sets LED and sensor pins as push-pull outputs.
 * -------------------------------------------------------------------------------------*/
void gpioInit(void);
/* -------------------------------------------------------------------------------------
 * gpioLed0SetOn()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns LED0 ON.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioLed0SetOn(void);
/* -------------------------------------------------------------------------------------
 * gpioLed0SetOff()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns LED0 OFF.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioLed0SetOff(void);
/* -------------------------------------------------------------------------------------
 * gpioLed1SetOn()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns LED1 ON.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioLed1SetOn(void);
/* -------------------------------------------------------------------------------------
 * gpioLed1SetOff()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns LED1 OFF.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioLed1SetOff(void);
/* -------------------------------------------------------------------------------------
 * gpioTempSensorOff()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns the temperature sensor OFF.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioTempSensorOff(void);
/* -------------------------------------------------------------------------------------
 * gpioTempSensorOn()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns the temperature sensor ON.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioTempSensorOn(void);
/* -------------------------------------------------------------------------------------
 * gpioSetDisplayExtcomin
 * ------------------------------------------------------------------------------------
 * @Purpose : This function controls the state of the DISP_EXT_COMIN pin, which is used
 *            to drive the external COM signal for a display. It sets the pin high or low
 *            based on the input value, enabling or disabling the external COM signal.
 * @Param   : bool value - The desired state of the DISP_EXT_COMIN pin (true = high, false = low).
 * @Return  : void
 *-------------------------------------------------------------------------------------*/

void gpioSetDisplayExtcomin(bool value);

void gpioMaxMFIOInterruptConfigure(void);


#endif /* SRC_GPIO_H_ */
