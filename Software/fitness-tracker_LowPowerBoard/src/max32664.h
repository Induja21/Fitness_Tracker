/*
 * max32664.h
 *
 *  Created on: 09-Nov-2025
 *      Author: nindu
 */

#ifndef SRC_MAX32664_H_
#define SRC_MAX32664_H_

#include "sl_i2cspm.h"
#include "sl_status.h"
#include "em_gpio.h"


typedef enum
{
    MAX32664_MODE_APP       = 0x00,  // Enter Application Mode (exit bootloader)
    MAX32664_MODE_SHUTDOWN  = 0x01,  // Shutdown B/C (requires power cycle or reset pin)
    MAX32664_MODE_RESET     = 0x02,  // Soft reset device (same as RSTN pulse)
    MAX32664_MODE_BOOT      = 0x08   // Enter Bootloader mode
} max32664_mode_t;


// Status and Error Codes
#define SFE_BIO_SUCCESS          0x00
#define SFE_BIO_ERR_UNKNOWN      0xFF
#define INCORR_PARAM             0xEE
#define MAX32664_I2C_ADDR        0x55

// Family Byte Definitions
#define SELECT_DEVICE_MODE       0x01
#define READ_DEVICE_MODE         0x02
#define IDENTITY                 0xFF
#define BOOTLOADER_INFO          0x81
#define OUTPUT_MODE              0x10
#define READ_DATA_OUTPUT         0x12
#define ENABLE_SENSOR            0x44
#define ENABLE_ALGORITHM         0x52
#define READ_ALGORITHM_CONFIG    0x51

// Index Byte Definitions
#define READ_MCU_TYPE            0x00
#define READ_SENSOR_HUB_VERS     0x03
#define READ_ALGO_VERS           0x07
#define BOOTLOADER_VERS          0x00
#define SET_FORMAT               0x00
#define WRITE_SET_THRESHOLD      0x01
#define ENABLE_MAX30101          0x03
#define ENABLE_AGC_ALGO          0x00
#define ENABLE_WHRM_ALGO         0x02
#define READ_AGC_NUM_SAMPLES     0x00
#define READ_DATA                0x01
#define NUM_SAMPLES              0x00

// Mode Definitions
#define MODE_ONE                 0x01
#define MODE_TWO                 0x02
#define ALGO_DATA                0x02
#define SENSOR_DATA              0x01
#define SENSOR_AND_ALGORITHM     0x03
#define ENABLE                   0x01

// Array Sizes
#define MAXFAST_ARRAY_SIZE       6
#define MAXFAST_EXTENDED_DATA    5
#define MAX30101_LED_ARRAY       12

// Structure Definitions
typedef struct {
    I2C_TypeDef *i2c_port;
    uint8_t address;
} bio_i2c_config_t;

typedef struct {
    bio_i2c_config_t i2c_cfg;
} bio_device_t;

typedef struct {
    uint32_t irLed;
    uint32_t redLed;
    uint16_t heartRate;
    uint8_t confidence;
    uint16_t oxygen;
    uint8_t status;
    float rValue;
    uint8_t extStatus;
} bioData_t;

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t revision;
} version_t;

typedef struct {
    bio_device_t device;
    int resetPin;
    int mfioPin;
    uint8_t userSelectedMode;
    uint8_t sampleRate;
    uint8_t bpmArr[MAXFAST_ARRAY_SIZE];
    uint8_t bpmArrTwo[MAXFAST_ARRAY_SIZE + MAXFAST_EXTENDED_DATA];
} bioSensorHub_t;

typedef enum
{
  MAX32664_INITIALIZATION_SUCCESSFUL,
  MAX32664_INITIALIZATION_IN_PROGRESS,
  MAX32664_INITIALIZATION_FAILED
}max32664_init_state;

void max32664StartInitAppmode(void);

void setBioSensorHubResetPin(void);

void setBioSensorHubMfioPin(void);


void readDeviceMode(void);

void readSensorHubVersion(void);

bool isAValidHubVersion(void);

uint8_t getLastReadBuffer(const uint8_t** readBuffer);
float getHubVersion(void);
void setBioSensorHubResetPin();
void clearBioSensorHubResetPin();
void setBioSensorHubMfioPin();
void clearBioSensorHubMfioPin();
void configureBioSensorHubMfioPin();
void configureBioSensorHubResetPin();
void selectDeviceMode(max32664_mode_t mode);
void waitForDeviceModeSelection();
void selectAlgoMode();
void waitForAlgoModeSelection();
void setThresholdData(uint8_t thresholdValue);
void waitToSetThresholdData();
void enableSensor();
void waitForSensorToEnable();
void enableAGCAlgorithm();
void waitForAGCAlgoToEnable();
void enableBPTAlgoSuite();
void waitForBPTAlgoSuiteToEnable();
void max32664ConfigInterrupts();
void max32664ReadFirstTime();
void max32664SetReportPeriod(uint8_t reportPeriodValue);
bool checkIfDataIsValid();
void parseAlgoData(void);
void waitForInitComplete();
void readNoOfSamplesinFiFo();
uint8_t getNoOfSamplescurrentlyAvailableInFifo();
void startperformSensorRead();
void performSensorReadOperation(uint16_t readSize);
void performReadOfNofSamplesInFiFo();
void startreadNoOfSamplesinFiFo();
void i2cDelayForReadOperation();
void setDiastolicValues();
void setSystolicValues();
void setDateAndTime();
void enableBPTAlgoSuiteInCalibrationMode();
uint8_t getProgress();
uint8_t* getCalibrationData();
void readCalibrationDataSecond();
void disableBPTAlgorithm();
void readCalibrationData();
void waitForBPTAlgoSuiteCalibrationToEnable();
void readStatusByte();
uint8_t getStatusByte();
void setSpO2Data();
uint8_t getBPStatus();
#endif /* SRC_MAX32664_H_ */
