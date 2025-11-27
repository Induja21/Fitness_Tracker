/*
  gpio.c
 
   Created on: Dec 12, 2018
       Author: Dan Walkes
   Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

   March 17
   Dave Sluiter: Use this file to define functions that set up or control GPIOs.
   
   Jan 24, 2023
   Dave Sluiter: Cleaned up gpioInit() to make it less confusing for students regarding
                 drive strength setting. 

 *
 * Student edit: Add your name and email address here:
 * @student    Induja Narayanan,Induja.Narayanan@Colorado.edu
 *
 
 */


// *****************************************************************************
// Students:
// We will be creating additional functions that configure and manipulate GPIOs.
// For any new GPIO function you create, place that function in this file.
// *****************************************************************************

#include <stdbool.h>
#include "em_gpio.h"
#include <string.h>

#include "gpio.h"


// Student Edit: Define these, 0's are placeholder values.
//
// See the radio board user guide at https://www.silabs.com/documents/login/user-guides/ug279-brd4104a-user-guide.pdf
// and GPIO documentation at https://siliconlabs.github.io/Gecko_SDK_Doc/efm32g/html/group__GPIO.html
// to determine the correct values for these.
// If these links have gone bad, consult the reference manual and/or the datasheet for the MCU.
// Change to correct port and pins:
/* -------------------------------------------------------------------------------------
 * GPIO Pin Definitions
 * -------------------------------------------------------------------------------------
 * @Purpose : Defines ports and pins for LEDs and the temperature sensor.
 * @Notes   : - LED0 and LED1 are connected to LED_port.
 *            - Temperature sensor is connected to SENSOR_port.
 * -------------------------------------------------------------------------------------*/
#define LED_port        (5)   // Port for LEDs
#define SENSOR_port     (3)   // Port for temperature sensor
#define LED0_pin        (4)   // Pin for LED0
#define LED1_pin        (5)   // Pin for LED1
#define TEMP_SENSOR_pin (15)  // Pin for temperature sensor
#define DISP_EXT_COMIN_pin (13)

#define BMI_INT1_PIN (6)
#define MAX_MFIO_PIN (5)

void gpioInterruptConfigure(void);
/* -------------------------------------------------------------------------------------
 * gpioInit()
 * -------------------------------------------------------------------------------------
 * @Purpose : Initializes GPIO settings, including drive strengths and modes.
 * @Param   : None
 * @Return  : None
 * @Notes   : - Configures drive strength settings for LED and sensor ports.
 *            - Sets LED and sensor pins as push-pull outputs.
 * -------------------------------------------------------------------------------------*/
void gpioInit()
{
    // Set the port's drive strength. In this MCU implementation, all GPIO cells
    // in a "Port" share the same drive strength setting.
    GPIO_DriveStrengthSet(SENSOR_port, gpioDriveStrengthWeakAlternateWeak); // Strong, 10mA
    GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthWeakAlternateWeak); // Weak, 1mA
    GPIO_DriveStrengthSet(gpioPortF,gpioDriveStrengthWeakAlternateWeak);
    // Set the GPIOs mode of operation
   // GPIO_PinModeSet(LED_port, LED0_pin, gpioModePushPull, false);
   // GPIO_PinModeSet(LED_port, LED1_pin, gpioModePushPull, false);
   // GPIO_PinModeSet(SENSOR_port, TEMP_SENSOR_pin, gpioModePushPull, false);
    gpioInterruptConfigure();
}

/* -------------------------------------------------------------------------------------
 * gpioLed0SetOn()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns LED0 ON.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioLed0SetOn()
{
    GPIO_PinOutSet(LED_port, LED0_pin);
}

/* -------------------------------------------------------------------------------------
 * gpioLed0SetOff()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns LED0 OFF.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioLed0SetOff()
{
    GPIO_PinOutClear(LED_port, LED0_pin);
}

/* -------------------------------------------------------------------------------------
 * gpioLed1SetOn()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns LED1 ON.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioLed1SetOn()
{
    GPIO_PinOutSet(LED_port, LED1_pin);
}

/* -------------------------------------------------------------------------------------
 * gpioLed1SetOff()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns LED1 OFF.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioLed1SetOff()
{
    GPIO_PinOutClear(LED_port, LED1_pin);
}

/* -------------------------------------------------------------------------------------
 * gpioTempSensorOff()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns the temperature sensor OFF.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioTempSensorOff()
{
    GPIO_PinOutClear(SENSOR_port, TEMP_SENSOR_pin);
}

/* -------------------------------------------------------------------------------------
 * gpioTempSensorOn()
 * -------------------------------------------------------------------------------------
 * @Purpose : Turns the temperature sensor ON.
 * @Param   : None
 * @Return  : None
 * -------------------------------------------------------------------------------------*/
void gpioTempSensorOn()
{
    GPIO_PinOutSet(SENSOR_port, TEMP_SENSOR_pin);
}
/* -------------------------------------------------------------------------------------
 * gpioSetDisplayExtcomin
 * ------------------------------------------------------------------------------------
 * @Purpose : This function controls the state of the DISP_EXT_COMIN pin, which is used
 *            to drive the external COM signal for a display. It sets the pin high or low
 *            based on the input value, enabling or disabling the external COM signal.
 * @Param   : bool value - The desired state of the DISP_EXT_COMIN pin (true = high, false = low).
 * @Return  : void
 *-------------------------------------------------------------------------------------*/

void gpioSetDisplayExtcomin(bool value)
{
  if(value)
  {
    GPIO_PinOutSet(SENSOR_port, DISP_EXT_COMIN_pin);
  }
  else
  {
    GPIO_PinOutClear(SENSOR_port, DISP_EXT_COMIN_pin);
  }
}



void gpioInterruptConfigure(void)
{
    // Configure BMI INT1 pin as input with filter (NO pull)
    GPIO_PinModeSet(gpioPortF, BMI_INT1_PIN, gpioModeInputPullFilter, 0);

    // Configure MFIO also as input with no pull
  //  GPIO_PinModeSet(gpioPortF, MAX_MFIO_PIN, gpioModeInputPullFilter, 0);

    // BMI INT1 → interrupt on RISING edge ONLY
    GPIO_ExtIntConfig(gpioPortF, BMI_INT1_PIN, BMI_INT1_PIN, true, false, true);

    // MFIO same config
   // GPIO_ExtIntConfig(gpioPortF, MAX_MFIO_PIN, MAX_MFIO_PIN, true, false, true);

    // Clear IRQ flags and enable NVIC
    NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
    NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}







