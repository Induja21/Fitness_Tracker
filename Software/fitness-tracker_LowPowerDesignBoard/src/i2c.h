/***********************************************************************
* @file i2c.h
* @version 0.0.1
* @brief This file has implementations related to i2c
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





#ifndef SRC_I2C_H_
#define SRC_I2C_H_

#include "em_i2c.h"

//Macro stating the success status of I2C operation
#define I2C_OPERATION_SUCCESSFUL 0
//Macro stating the failure status of I2C operation
#define I2C_OPERATION_FAILED  -1
//Time taken for the chip to be powered on for operation
#define SI7021_POWER_ON_TIME_IN_US  80000

/* -------------------------------------------------------------------------------------
 * powerOnSi7021
 * ------------------------------------------------------------------------------------
 * @Purpose : This function turns on the power to the Si7021 temperature sensor by setting
 *            the GPIO pin PD15 to high.
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void powerOnSi7021(void);

/* -------------------------------------------------------------------------------------
 * powerOffSi7021
 * ------------------------------------------------------------------------------------
 * @Purpose : This function turns off the power to the Si7021 temperature sensor by clearing
 *            the GPIO pin PD15 to low.
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void powerOffSi7021(void);

/* -------------------------------------------------------------------------------------
 * readTemperatureData
 * ------------------------------------------------------------------------------------
 * @Purpose : This function reads the temperature data from the Si7021 sensor over I2C.
 *            It sends a temperature measurement command and then reads the result.
 * @Param   : tempData - Pointer to a uint16_t variable where the temperature data will be stored.
 * @Return  : I2C_TransferReturn_TypeDef - Status of the I2C transfer operation.
 *-------------------------------------------------------------------------------------*/
I2C_TransferReturn_TypeDef readTemperatureData(float *tempData);


// Function to start I2C transaction
/* -------------------------------------------------------------------------------------
 * initiateTemperatureMeasurement
 * ------------------------------------------------------------------------------------
 * @Purpose : This function initiates a temperature measurement by sending the temperature
 *            measurement command (TEMP_MEASURE_CMD) to the Si7021 sensor over I2C.
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void initiateTemperatureMeasurement(void);

/* -------------------------------------------------------------------------------------
 * waitForConversion
 * ------------------------------------------------------------------------------------
 * @Purpose : This function waits for the temperature conversion to complete by delaying
 *            for the specified time (TIME_TAKEN_FOR_CONVERSION_IN_US).
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void waitForConversion(void);

/* -------------------------------------------------------------------------------------
 * sendReadCmdForTempData
 * ------------------------------------------------------------------------------------
 * @Purpose : This function sends a read command to the Si7021 sensor to retrieve the
 *            temperature data after the measurement is complete.
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void sendReadCmdForTempData(void);

/* -------------------------------------------------------------------------------------
 * getRawTempData
 * ------------------------------------------------------------------------------------
 * @Purpose : This function retrieves the raw temperature data from the Si7021 sensor
 *            after the read command has been sent and the data is available.
 * @Param   : None
 * @Return  : float - The raw temperature data as a 16-bit value.
 *-------------------------------------------------------------------------------------*/
float getRawTempData(void);

/* -------------------------------------------------------------------------------------
 * getConvertedTempData
 * ------------------------------------------------------------------------------------
 * @Purpose : This function converts the raw temperature data read from the Si7021 sensor
 *            into a temperature value in Celsius using the formula provided in the
 *            datasheet.
 * @Param   : None
 * @Return  : float - The converted temperature value in Celsius.
 *-------------------------------------------------------------------------------------*/
float getConvertedTempData(void);

/* -------------------------------------------------------------------------------------
 * waitForPowerOn
 * ------------------------------------------------------------------------------------
 * @Purpose : This function waits for the Si7021 sensor to power on completely by delaying
 *            for the specified time (SI7021_POWER_ON_TIME_IN_US).
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void waitForPowerOn(void);

/* -------------------------------------------------------------------------------------
 * si7021I2cInit
 * ------------------------------------------------------------------------------------
 * @Purpose : This function initializes the I2C peripheral for communication with the Si7021
 *            temperature sensor. It configures the I2C port, SCL and SDA pins, and other
 *            necessary parameters.
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void si7021I2cInit(void);


#endif /* SRC_I2C_H_ */
