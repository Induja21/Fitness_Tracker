/***********************************************************************
* @file i2c.c
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
#include "app_assert.h"
#include "em_gpio.h"
#include "i2c.h"
#include "em_i2c.h"
#include "sl_i2cspm.h"
#include "gpio.h"
#include "timer.h"
#include "scheduler.h"
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

//I2C device address for SI7021 sensor
#define SI7021_DEVICE_ADDR        0x40
//Command to read temperature  data in Hold mode
#define TEMP_MEASURE_CMD          0xF3

//Time taken for the temperature sensor data to be ready
#define TIME_TAKEN_FOR_CONVERSION_IN_US 10800

//Macro to convert to celsius
#define CONVERT_TEMP(data) (((175.72 * (data)) / 65536) - 46.85)

static uint8_t readData[2] = {0};
static uint8_t cmd_data = TEMP_MEASURE_CMD;
static I2C_TransferSeq_TypeDef i2cTransfer;
/* -------------------------------------------------------------------------------------
 * powerOnSi7021
 * ------------------------------------------------------------------------------------
 * @Purpose : This function turns on the power to the Si7021 temperature sensor by setting
 *            the GPIO pin PD15 to high.
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void powerOnSi7021() {
    gpioTempSensorOn();
}

/* -------------------------------------------------------------------------------------
 * powerOffSi7021
 * ------------------------------------------------------------------------------------
 * @Purpose : This function turns off the power to the Si7021 temperature sensor by clearing
 *            the GPIO pin PD15 to low.
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void powerOffSi7021() {
    gpioTempSensorOff();
}

/* -------------------------------------------------------------------------------------
 * waitForConversion
 * ------------------------------------------------------------------------------------
 * @Purpose : This function waits for the temperature conversion to complete by delaying
 *            for the specified time (TIME_TAKEN_FOR_CONVERSION_IN_US).
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void waitForConversion()
{
  timerWaitUs_interrupt(TIME_TAKEN_FOR_CONVERSION_IN_US);
}

/* -------------------------------------------------------------------------------------
 * waitForPowerOn
 * ------------------------------------------------------------------------------------
 * @Purpose : This function waits for the Si7021 sensor to power on completely by delaying
 *            for the specified time (SI7021_POWER_ON_TIME_IN_US).
 * @Param   : None
 * @Return  : None
 *--------------------------------------------------------------------------------------*/
void waitForPowerOn()
{
  timerWaitUs_interrupt(SI7021_POWER_ON_TIME_IN_US);
}

/* -------------------------------------------------------------------------------------
 * initiateTemperatureMeasurement
 * ------------------------------------------------------------------------------------
 * @Purpose : This function initiates a temperature measurement by sending the temperature
 *            measurement command (TEMP_MEASURE_CMD) to the Si7021 sensor over I2C.
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void initiateTemperatureMeasurement() {
    I2C_TransferReturn_TypeDef transferStatus;

    i2cTransfer.addr = SI7021_DEVICE_ADDR << 1;
    i2cTransfer.flags = I2C_FLAG_WRITE;
    i2cTransfer.buf[0].data = &cmd_data;
    i2cTransfer.buf[0].len = sizeof(cmd_data);

    transferStatus = I2C_TransferInit(I2C0, &i2cTransfer); // Start non-blocking transfer
    if (transferStatus < 0) {
        LOG_ERROR("I2C_TransferInit() Write error = %d", transferStatus);
    }
    I2C_IntEnable(I2C0, I2C_IEN_MSTOP);
    NVIC_EnableIRQ(I2C0_IRQn);
}

/* -------------------------------------------------------------------------------------
 * sendReadCmdForTempData
 * ------------------------------------------------------------------------------------
 * @Purpose : This function sends a read command to the Si7021 sensor to retrieve the
 *            temperature data after the measurement is complete.
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void sendReadCmdForTempData()
{
  I2C_TransferReturn_TypeDef transferStatus;

  i2cTransfer.addr = SI7021_DEVICE_ADDR << 1;
  i2cTransfer.flags = I2C_FLAG_READ;
  i2cTransfer.buf[0].data = readData;
  i2cTransfer.buf[0].len = sizeof(readData);

  transferStatus = I2C_TransferInit(I2C0, &i2cTransfer); // Start non-blocking transfer
  if (transferStatus < 0) {
      LOG_ERROR("I2C_TransferInit() Write error = %d", transferStatus);
  }
  I2C_IntEnable(I2C0, I2C_IEN_MSTOP);
  NVIC_EnableIRQ(I2C0_IRQn);
}

/* -------------------------------------------------------------------------------------
 * getConvertedTempData
 * ------------------------------------------------------------------------------------
 * @Purpose : This function converts the raw temperature data read from the Si7021 sensor
 *            into a temperature value in Celsius using the formula provided in the
 *            datasheet.
 * @Param   : None
 * @Return  : float - The converted temperature value in Celsius.
 *-------------------------------------------------------------------------------------*/
float getConvertedTempData()
{
  float tempData = (readData[0] << 8) | readData[1];
  return CONVERT_TEMP(tempData);
}

/* -------------------------------------------------------------------------------------
 * si7021I2cInit
 * ------------------------------------------------------------------------------------
 * @Purpose : This function initializes the I2C peripheral for communication with the Si7021
 *            temperature sensor. It configures the I2C port, SCL and SDA pins, and other
 *            necessary parameters.
 * @Param   : None
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void si7021I2cInit(void)
{
  // Initialize I2C and other peripherals
  I2CSPM_Init_TypeDef i2cConfig = {
      .port = I2C0,
      .sclPort = gpioPortC,
      .sclPin = 10,
      .sdaPort = gpioPortC,
      .sdaPin = 11,
      .portLocationScl = 14,
      .portLocationSda = 16,
      .i2cRefFreq = 0,
      .i2cMaxFreq = I2C_FREQ_STANDARD_MAX,
      .i2cClhr = i2cClockHLRStandard
  };

  I2CSPM_Init(&i2cConfig);
}
