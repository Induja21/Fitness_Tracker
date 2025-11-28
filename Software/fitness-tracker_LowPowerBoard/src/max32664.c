#include "max32664.h"
#include "timer.h"
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#define MAX32664_RESET_PIN 13  // Example pin for RESET (Port B, Pin 13)
#define MAX32664_MFIO_PIN  5  // Example pin for MFIO (Port F, Pin 6)
#define MAX32664_I2C_ADDR  0x55 // Default I2C address for MAX32664
#define CMD_DELAY_IN_US 60000


uint8_t readResponse[1024];
uint8_t calibrationData[825] = {0x03,0,0,3,0,0,5,0,0,0,0,0,220,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,1,0,0,0,0,0,221,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,4};
uint8_t lastReadSize=0;
I2C_TransferSeq_TypeDef i2cTransfer;

static bioSensorHub_t max32664_hub;
I2C_TransferSeq_TypeDef i2cTransfer;

typedef struct {
    uint32_t ir;
    uint32_t red;
    uint8_t bpStatus;
    uint8_t progress;
    float heartRate;
    uint8_t systolic;
    uint8_t diastolic;
    float spo2;
    float rRatio;
    uint8_t hrAboveResting;
} maxBptData_t;

maxBptData_t maxData;
void max32664StartInitAppmode(void)
{
  // Initialize hub structure
  max32664_hub.device.i2c_cfg.i2c_port = I2C0;
  max32664_hub.device.i2c_cfg.address = MAX32664_I2C_ADDR;
  max32664_hub.resetPin = MAX32664_RESET_PIN;
  max32664_hub.mfioPin = MAX32664_MFIO_PIN;

  // Configure RESET and MFIO pins
  GPIO_PinModeSet(gpioPortB, max32664_hub.resetPin, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortF, max32664_hub.mfioPin, gpioModePushPull, 0);

  // Enter application mode
  GPIO_PinOutSet(gpioPortF,  max32664_hub.mfioPin);
  GPIO_PinOutClear(gpioPortB,  max32664_hub.resetPin);
  timerWaitUs_interrupt(10000);

}


// Set the reset pin high
void setBioSensorHubResetPin()
{
    GPIO_PinOutSet(gpioPortB, max32664_hub.resetPin);
}

// Clear the reset pin low
void clearBioSensorHubResetPin()
{
    GPIO_PinOutClear(gpioPortB, max32664_hub.resetPin);
}

// Set the MFIO pin high
void setBioSensorHubMfioPin()
{
    GPIO_PinOutSet(gpioPortF, max32664_hub.mfioPin);
}

// Clear the MFIO pin low
void clearBioSensorHubMfioPin()
{
    GPIO_PinOutClear(gpioPortF, max32664_hub.mfioPin);
}

// Configure the MFIO pin mode
void configureBioSensorHubMfioPin()
{
    GPIO_PinModeSet(gpioPortF, max32664_hub.mfioPin, gpioModeInputPull, 1);
}

// Configure the reset pin mode
void configureBioSensorHubResetPin()
{
    GPIO_PinModeSet(gpioPortB, max32664_hub.resetPin, gpioModePushPull, 0);
}

static I2C_TransferReturn_TypeDef startReadBioSensorReg( uint8_t command, uint8_t index, uint8_t *data, uint8_t len)
{
    uint8_t cmd[2] = { command, index };

    i2cTransfer.addr = max32664_hub.device.i2c_cfg.address << 1;
    i2cTransfer.flags = I2C_FLAG_WRITE_READ;
    i2cTransfer.buf[0].data = cmd;
    i2cTransfer.buf[0].len = 2;
    i2cTransfer.buf[1].data = data;
    i2cTransfer.buf[1].len = len + 1; // +1 for status byte

    I2C_TransferReturn_TypeDef transferStatus = I2C_TransferInit(max32664_hub.device.i2c_cfg.i2c_port, &i2cTransfer);
    if (transferStatus < 0) {
        //LOG_ERROR("I2C_TransferInit() Read error = %d", transferStatus);
    }
    I2C_IntEnable(I2C0, I2C_IEN_MSTOP);
    NVIC_EnableIRQ(I2C0_IRQn);
    return transferStatus;
}

static I2C_TransferReturn_TypeDef startReadBioSensorRegCalibration( uint8_t command, uint8_t index, uint8_t cmd1)
{
    uint8_t cmd[3] = { command, index,cmd1 };

    i2cTransfer.addr = max32664_hub.device.i2c_cfg.address << 1;
    i2cTransfer.flags = I2C_FLAG_WRITE;
    i2cTransfer.buf[0].data = cmd;
    i2cTransfer.buf[0].len = 3;
    i2cTransfer.buf[1].data = NULL;
    i2cTransfer.buf[1].len = 0; // +1 for status byte

    I2C_TransferReturn_TypeDef transferStatus = I2C_TransferInit(max32664_hub.device.i2c_cfg.i2c_port, &i2cTransfer);
    if (transferStatus < 0) {
        //LOG_ERROR("I2C_TransferInit() Read error = %d", transferStatus);
    }
    I2C_IntEnable(I2C0, I2C_IEN_MSTOP);
    NVIC_EnableIRQ(I2C0_IRQn);
    return transferStatus;
}

static I2C_TransferReturn_TypeDef startReadBioSensorRegCalibrationOperation( uint8_t *data, uint8_t len)
{


    i2cTransfer.addr = max32664_hub.device.i2c_cfg.address << 1;
    i2cTransfer.flags = I2C_FLAG_READ;
    i2cTransfer.buf[0].data = data;
    i2cTransfer.buf[0].len = len+1;
    i2cTransfer.buf[1].data = NULL;
    i2cTransfer.buf[1].len = 0; // +1 for status byte

    I2C_TransferReturn_TypeDef transferStatus = I2C_TransferInit(max32664_hub.device.i2c_cfg.i2c_port, &i2cTransfer);
    if (transferStatus < 0) {
        //LOG_ERROR("I2C_TransferInit() Read error = %d", transferStatus);
    }
    I2C_IntEnable(I2C0, I2C_IEN_MSTOP);
    NVIC_EnableIRQ(I2C0_IRQn);
    return transferStatus;
}

static I2C_TransferReturn_TypeDef startRequestReadBioSensorRegFiFo( uint8_t command, uint8_t index)
{
  uint8_t cmd[2] = { command, index };

  i2cTransfer.addr = max32664_hub.device.i2c_cfg.address << 1;
  i2cTransfer.flags = I2C_FLAG_WRITE;
  i2cTransfer.buf[0].data = cmd;
  i2cTransfer.buf[0].len = 2;
  i2cTransfer.buf[1].data = NULL;
  i2cTransfer.buf[1].len = 0; // +1 for status byte

  I2C_TransferReturn_TypeDef transferStatus = I2C_TransferInit(max32664_hub.device.i2c_cfg.i2c_port, &i2cTransfer);
  if (transferStatus < 0) {
      //LOG_ERROR("I2C_TransferInit() Read error = %d", transferStatus);
  }
  I2C_IntEnable(I2C0, I2C_IEN_MSTOP);
  NVIC_EnableIRQ(I2C0_IRQn);
  return transferStatus;
}

static I2C_TransferReturn_TypeDef startReadBioSensorRegFiFo( uint8_t *data, uint8_t len)
{

  i2cTransfer.addr = max32664_hub.device.i2c_cfg.address << 1;
  i2cTransfer.flags = I2C_FLAG_READ;
  i2cTransfer.buf[0].data = data;
  i2cTransfer.buf[0].len = len+1;
  i2cTransfer.buf[1].data = NULL;
  i2cTransfer.buf[1].len = 0; // +1 for status byte

  I2C_TransferReturn_TypeDef transferStatus = I2C_TransferInit(max32664_hub.device.i2c_cfg.i2c_port, &i2cTransfer);
  if (transferStatus < 0) {
      //LOG_ERROR("I2C_TransferInit() Read error = %d", transferStatus);
  }
  I2C_IntEnable(I2C0, I2C_IEN_MSTOP);
  NVIC_EnableIRQ(I2C0_IRQn);
  return transferStatus;
}


static I2C_TransferReturn_TypeDef startWriteBioSensorReg(uint8_t command,
                                                         uint8_t index,
                                                         const uint8_t *payload,
                                                         uint16_t payloadLen)
{
    static uint8_t txBuf[830];

    txBuf[0] = command;
    txBuf[1] = index;

    // Copy payload if present
    if(payload != NULL && payloadLen > 0)
    {
        memcpy(&txBuf[2], payload, payloadLen);
    }

    // Setup I2C transfer (write only)
    i2cTransfer.addr        = max32664_hub.device.i2c_cfg.address << 1;
    i2cTransfer.flags       = I2C_FLAG_WRITE;

    i2cTransfer.buf[0].data = txBuf;
    i2cTransfer.buf[0].len  = payloadLen + 2;  // command + index + payload

    i2cTransfer.buf[1].data = NULL;
    i2cTransfer.buf[1].len  = 0;

    I2C_TransferReturn_TypeDef transferStatus =
        I2C_TransferInit(max32664_hub.device.i2c_cfg.i2c_port, &i2cTransfer);

    if (transferStatus < 0)
    {
        //LOG_ERROR("I2C Write error %d", transferStatus);
    }

    I2C_IntEnable(I2C0, I2C_IEN_MSTOP);
    NVIC_EnableIRQ(I2C0_IRQn);
    return transferStatus;
}

void selectDeviceMode(max32664_mode_t mode)
{
    // The device mode value is the payload
    uint8_t payload = (uint8_t)mode;

    // Send command: [0x02][0x00][MODE]
    startWriteBioSensorReg(SELECT_DEVICE_MODE, 0, &payload, 1);

}

void loadCalibrationData()
{
  startWriteBioSensorReg(0x50,0x04,calibrationData,825);

}
void waitForDeviceModeSelection()
{
  // Timing requirement: allow internal state switch
  timerWaitUs_interrupt(50000);  // 50ms recommended delay
}


void selectAlgoMode()
{
  uint8_t algoMode = 0x03;
  startWriteBioSensorReg(OUTPUT_MODE, 0, &algoMode, 1);
}

void waitForAlgoModeSelection()
{
  timerWaitUs_interrupt(50000);  // 50ms recommended delay
}

void setThresholdData(uint8_t thresholdValue)
{
  startWriteBioSensorReg(OUTPUT_MODE, 0x01, &thresholdValue, 1);

}

void waitToSetThresholdData()
{
  timerWaitUs_interrupt(50000);

}

void enableSensor()
{
  uint8_t on = 0x01;
  startWriteBioSensorReg(0x44, 0x03, &on, 1);

}

void waitForSensorToEnable()
{
  timerWaitUs_interrupt(50000);
}

void enableAGCAlgorithm()
{
  uint8_t on = 0x01;
  startWriteBioSensorReg(0x52, 0x00, &on, 1);
}

void waitForAGCAlgoToEnable()
{
  timerWaitUs_interrupt(30000);
}
void enableBPTAlgoSuite()
{
  // FAMILY = 0x52 (Algorithm)
  // INDEX  = 0x04 (Wearable Suite WHRM + WSpO2)
  // VALUE  = 0x02 (Enable Mode 1)
  uint8_t estimationMode = 0x02;
  startWriteBioSensorReg(0x52, 0x04, &estimationMode, 1);

}

void enableBPTAlgoSuiteInCalibrationMode()
{
  // FAMILY = 0x52 (Algorithm)
  // INDEX  = 0x04 (Wearable Suite WHRM + WSpO2)
  // VALUE  = 0x02 (Enable Mode 1)
  uint8_t calibrationMode = 0x01;
  startWriteBioSensorReg(0x52, 0x04, &calibrationMode, 1);

}

void setDateAndTime()
{
  uint8_t dateTime[]={0x04,0x87,0xD5,0x03,00,0x5B,0x2F,0x02,0x00};
  startWriteBioSensorReg(0x50, 0x04, dateTime, sizeof(dateTime));
}

void setSpO2Data()
{
  uint8_t spO2[]={0x06,0x00,0x02,0x6F,0x60,0xFF,0xCB,0x1D,0x12,0x00,0xAB,0xF3,0x7B};
  startWriteBioSensorReg(0x50, 0x04, spO2, sizeof(spO2));
}

void setSystolicValues()
{
  uint8_t systolicValues[]={0x01,0x78,0x7A,0x7D};
  startWriteBioSensorReg(0x50, 0x04, systolicValues, sizeof(systolicValues));
}

void setDiastolicValues()
{
  uint8_t systolicValues[]={0x02,0x50,0x51,0x52};
  startWriteBioSensorReg(0x50, 0x04, systolicValues, sizeof(systolicValues));
}
void waitForBPTAlgoSuiteToEnable()
{
  timerWaitUs_interrupt(40000);
}

void waitForBPTAlgoSuiteCalibrationToEnable()
{
  timerWaitUs_interrupt(100000);
}


void waitForInitComplete()
{
  timerWaitUs_interrupt(1000000);
}
void readDeviceMode()
{
  lastReadSize=2;
  readResponse[0]=0xFF;
  readResponse[1]=0xFF;
  startReadBioSensorReg(READ_DEVICE_MODE, 0x00, readResponse, lastReadSize);

}


/* Read sensor hub version */
void readSensorHubVersion()
{
    lastReadSize=4;
    startReadBioSensorReg(IDENTITY, READ_SENSOR_HUB_VERS, readResponse, lastReadSize);

}

bool isAValidHubVersion()
{
  version_t bioHubVers = {0, 0, 0};

  bioHubVers.major = readResponse[1];
  bioHubVers.minor = readResponse[2];
  bioHubVers.revision = readResponse[3];

  if (bioHubVers.major == 0 && bioHubVers.minor == 0 && bioHubVers.revision == 0) {
         return false;
     }

  return true;

}

float getHubVersion()
{
  version_t bioHubVers = {0, 0, 0};

  bioHubVers.major = readResponse[1];
  bioHubVers.minor = readResponse[2];
  bioHubVers.revision = readResponse[3];

  float version =
         (float)bioHubVers.major +
         ((float)bioHubVers.minor / 100.0f) +
         ((float)bioHubVers.revision / 10000.0f);

     return version;

}


uint8_t getLastReadBuffer(const uint8_t** readBuffer)
{
  *readBuffer=readResponse;
  return lastReadSize;
}
#define MAX_MFIO_PIN (5)

void max32664ConfigInterrupts()
{
    // 1. Configure MFIO pin as input with pull-up (MFIO is active LOW)
    GPIO_PinModeSet(gpioPortF, MAX_MFIO_PIN, gpioModeInputPullFilter, 1);

    // 2. Configure interrupt (falling edge)
    GPIO_ExtIntConfig(gpioPortF, MAX_MFIO_PIN, MAX_MFIO_PIN,
                      false,    // risingEdge = false
                      true,     // fallingEdge = true
                      true);    // enable = true

    // 3. Clear flags
    GPIO_IntClear(1 << MAX_MFIO_PIN);
    NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);

    // 4. Enable NVIC IRQ
    NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

void max32664ReadFirstTime()
{

//  if (GPIO_PinInGet(gpioPortF, MAX_MFIO_PIN) == 0)
//  {

      lastReadSize = 5;

      // Read processed algorithm output sample (FIFO one sample)
      // Family: 0x12, Index: 0x01
      startReadBioSensorReg(0x12, 0x01, readResponse, lastReadSize);
  //}


}
void startreadNoOfSamplesinFiFo()
{
 // lastReadSize=2;
 // startReadBioSensorReg(0x12, 0x00, readResponse, lastReadSize);
  startRequestReadBioSensorRegFiFo(0x12,0x00);
}

void i2cDelayForReadOperation()
{
  timerWaitUs_interrupt(2000);
}

void performReadOfNofSamplesInFiFo()
{
  lastReadSize=2;
  startReadBioSensorRegFiFo(readResponse,lastReadSize);
}

uint8_t getNoOfSamplescurrentlyAvailableInFifo()
{
   if(readResponse[0]==0)
     {
       return readResponse[1];
     }
   return 0;
}

void startperformSensorRead()
{
      // Read processed algorithm output sample (FIFO one sample)
      // Family: 0x12, Index: 0x01
  startRequestReadBioSensorRegFiFo(0x12, 0x01);


}

void performSensorReadOperation(uint16_t readSize)
{
  lastReadSize = readSize;

      // Read processed algorithm output sample (FIFO one sample)
      // Family: 0x12, Index: 0x01
  startReadBioSensorRegFiFo(readResponse, lastReadSize);

}

uint8_t getProgress()
{
  return readResponse[13];
}


uint8_t getBPStatus()
{
  return readResponse[12];
}
void readStatusByte()
{
  lastReadSize=2;
  startReadBioSensorReg(0x00, 0x00, readResponse, lastReadSize);
}

uint8_t getStatusByte()
{
  if(readResponse[0]==0)
    {
      return readResponse[1];
    }
  return 0;
}
void disableAFE()
{
  uint8_t off = 0x00;
  startWriteBioSensorReg(0x44, 0x03, &off, 1);

}

void disableBPTAlgorithm()
{
  uint8_t off = 0x00;
   startWriteBioSensorReg(0x52, 0x04, &off, 1);

}

void readCalibrationData()
{

  startReadBioSensorRegCalibration(0x52,0x04,0x03);
}

void readCalibrationDataSecond()
{
  lastReadSize = 824;
  memset(readResponse,0,sizeof(readResponse));
  startReadBioSensorRegCalibrationOperation(readResponse,824);

}
uint8_t* getCalibrationData()
{
  return readResponse[1];
}
bool checkIfDataIsValid()
{
  return (maxData.bpStatus == 3 && maxData.progress > 90);
}
void parseAlgoData(void)
{
    // Raw PPG values: 24-bit (MSB first)
    maxData.ir  = (readResponse[0] << 16) | (readResponse[1] << 8) | readResponse[2];
    maxData.red = (readResponse[3] << 16) | (readResponse[4] << 8) | readResponse[5];

    // BPT algorithm outputs
    maxData.bpStatus  = readResponse[12];                    // BP Status
    maxData.progress  = readResponse[13];                    // Percentage progress

    maxData.heartRate = ((readResponse[14] << 8) |
                          readResponse[15]) / 10.0f;         // HR x10 -> float BPM

    maxData.systolic  = readResponse[16];                    // SBP (mmHg)
    maxData.diastolic = readResponse[17];                    // DBP (mmHg)

    maxData.spo2      = ((readResponse[18] << 8) |
                          readResponse[19]) / 10.0f;         // SpO2 x10 -> %

    maxData.rRatio    = ((readResponse[20] << 8) |
                          readResponse[21]) / 1000.0f;       // Ratio R

    maxData.hrAboveResting = readResponse[22];               // 0 or 1
}



void max32664SetReportPeriod(uint8_t reportPeriodValue)
{
  startWriteBioSensorReg(OUTPUT_MODE, 0x02, &reportPeriodValue, 1);

}
