// bmi270.h
/*
 * bmi270.h
 *
 *  Created on: 10-Nov-2025
 *      Author: nindu
 */

#ifndef SRC_BMI270_H_
#define SRC_BMI270_H_

#include "sl_status.h"
#include "sl_i2cspm.h"
#include "em_i2c.h"
#include <stdint.h>

/*******************************************************************************
 ***************************  Defines / Macros  ********************************
 ******************************************************************************/

#define BMI270_ADDR_LOW   0x68  // SDO = GND
#define BMI270_ADDR_HIGH  0x69  // SDO = VDDIO
#define BMI270_CHIP_ID    0x24

#define INIT_SUCCESS_STATUS 0x01
#define SOFT_RESET_CMD         0xB6

#define BMI270_CONFIGFILE_LEN 8192
#define BMI270_CONFIG_CHUNK_MAX 32

/*******************************************************************************
 *******************************   ENUMS   *************************************
 ******************************************************************************/
typedef enum {
  BMI270_CHIP_ID_REG = 0x00,
  BMI270_ACC_X_LSB   = 0x0C,
  BMI270_GYR_X_LSB   = 0x12,
  BMI270_INT_STATUS_0   = 0x1C,
  BMI270_INIT_STATUS = 0x21,
  BMI270_FEAT_PAGE = 0x2F,
  BMI270_FEAT_SC_OUT_ADDR=0x30,
  BMI270_FEAT_STEP_COUNTER_ADDR=0x32,
  BMI270_FEAT_ANY_MOTION_ADDR=0x3C,
  BMI270_INT1_IO_CONTROL=0x53,
  BMI270_INT1_MAP_FEAT =0x56,
  BMI270_INIT_ADDR_0_REG = 0x5B,
  BMI270_INIT_ADDR_1_REG = 0x5C,
  BMI270_INIT_DATA_REG = 0x5E,
  BMI270_INIT_CTRL = 0x59,
  BMI270_POWER_CONF=0x7C,
  BMI270_PWR_CTRL_REG = 0x7D,
  BMI270_CMD_REG     = 0x7E

} bmi270_registers_t;

/*******************************************************************************
 *****************************   STRUCTURES   **********************************
 ******************************************************************************/
typedef struct {
  I2C_TypeDef *i2c_port;     // I2C peripheral instance (e.g., I2C0)
  uint8_t      address;      // 7-bit I2C address
} bmi270_i2c_config_t;

typedef struct {
  bmi270_i2c_config_t i2c_cfg;
} bmi270_device_t;

/*******************************************************************************
 *****************************   API PROTOTYPES   *******************************
 ******************************************************************************/
/***************************************************************************//**
 * @brief Initialize BMI270 I2C configuration
 ******************************************************************************/
void bmi270StartInitSequence(void);

/***************************************************************************//**
 * @brief Issue a soft reset command to BMI270
 ******************************************************************************/
void bmi270SendSoftReset(void);

/***************************************************************************//**
 * @brief Read the BMI270 chip ID (non-blocking)
 ******************************************************************************/
void bmi270ReadChipID(void);

/***************************************************************************//**
 * @brief Read the BMI270 initialization status register
 ******************************************************************************/
void bmi270ReadInitStatus(void);

/***************************************************************************//**
 * @brief Read accelerometer data (X, Y, Z) - non-blocking
 ******************************************************************************/
void bmi270ReadAccel(void);

/***************************************************************************//**
 * @brief Read gyroscope data (X, Y, Z) - non-blocking
 ******************************************************************************/
void bmi270ReadGyro(void);

/***************************************************************************//**
 * @brief Helper to get last read buffer contents
 * @param dataPtr pointer to receive buffer address
 * @return size of the data read
 ******************************************************************************/
uint8_t getLastBMI270ReadBuffer(const uint8_t **dataPtr);

/***************************************************************************//**
 * @brief Validate BMI270 chip ID
 * @return true if valid, false otherwise
 ******************************************************************************/
bool isValidBMI270ChipID(void);

/***************************************************************************//**
 * @brief Validate BMI270 initialization status
 * @return true if initialization completed successfully, false otherwise
 ******************************************************************************/
bool isBMI270InitSuccessful(void);

void waitForSoftReset(void);

void sendNextConfigChunk(void);

bool hasMoreConfigData(void);

void startWriteBMI270DataChunk(uint8_t reg, const uint8_t *data, unsigned int len);
void waitForInternalStatusToUpdate(void);
void waitForInitCtrlToDisable(void);
void bmi270SetInitCtrl(uint8_t value);
void waitBeforeNextTransfer(void);
void setInitAddr0(uint8_t value);
void setInitAddr1(uint8_t value);

void startReadBMI270Reg(uint8_t reg, uint8_t *buffer, unsigned int len);
void bmi270UpdatePowerMode(uint8_t value);
void waitForDisablePowerMode(void);
extern unsigned int configUploadOffset;
void bmi270EnableAccel();
void waitForAccelEnable();
void startStepCounterEnable();
void readSelectedFeaturePage();
void enableStepOpAndWriteToFeaturePage();
void resetStepCounterAndWriteToFeaturePage();
void mapStepCounterToInterrupt1();
void configureInt1ToOutputEnable();
void bmi270RestoreFeaturePage(void);
void bmi270EnableODR();
uint8_t getIntStatus0Value();
uint32_t getStepCounterData();
void bmi270StartReadingStepCounterData(void);
void bmi270StartReadingIntStatus0Reg(void);
#endif /* SRC_BMI270_H_ */
