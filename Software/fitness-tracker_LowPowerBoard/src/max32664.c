#include "max32664.h"
#include "timer.h"
#include <stddef.h>
#include <stdio.h>

#define MAX32664_RESET_PIN 13  // Example pin for RESET (Port B, Pin 13)
#define MAX32664_MFIO_PIN  5  // Example pin for MFIO (Port F, Pin 6)
#define MAX32664_I2C_ADDR  0x55 // Default I2C address for MAX32664
#define CMD_DELAY_IN_US 60000


uint8_t readResponse[256];
uint8_t lastReadSize=0;
I2C_TransferSeq_TypeDef i2cTransfer;

static bioSensorHub_t max32664_hub;
I2C_TransferSeq_TypeDef i2cTransfer;

typedef struct {
    uint16_t heartRate;
    uint8_t confidence;
    uint8_t spo2;
    uint8_t status;
} max32664_data_t;

max32664_data_t maxData;
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
                                                         uint8_t payloadLen)
{
    static uint8_t txBuf[16];

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

void waitForBPTAlgoSuiteToEnable()
{
  timerWaitUs_interrupt(40000);
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


  GPIO_IntClear(1 << MAX_MFIO_PIN);
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);


  // MFIO same config
 GPIO_ExtIntConfig(gpioPortF, MAX_MFIO_PIN, MAX_MFIO_PIN, false, true, true);

 GPIO_PinModeSet(gpioPortF, MAX_MFIO_PIN, gpioModeInputPullFilter, 1);
 // NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
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
  timerWaitUs_interrupt(2500);
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

void performSensorReadOperation(uint8_t readSize)
{
  lastReadSize = readSize;

      // Read processed algorithm output sample (FIFO one sample)
      // Family: 0x12, Index: 0x01
  startReadBioSensorRegFiFo(readResponse, lastReadSize);

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

bool checkIfDataIsValid()
{
  return (maxData.status == 3 && maxData.confidence > 80);
}
void parseAlgoData(void)
{
    // Parse BPT algorithm data from FIFO sample (bytes 12+ of 23-byte sample)
    // Assumes readResponse contains full 23-byte FIFO sample
    maxData.heartRate  = ((uint16_t)readResponse[14] << 8) | readResponse[15];  // Bytes 14-15: 10x HR (MSB first)
    maxData.confidence = readResponse[13];                                      // Byte 13: Progress % (confidence)
    maxData.spo2       = ((uint16_t)readResponse[18] << 8) | readResponse[19]; // Bytes 18-19: 10x SpO2 (MSB first)
    maxData.status     = readResponse[12];                                      // Byte 12: BP status
}


void max32664SetReportPeriod(uint8_t reportPeriodValue)
{
  startWriteBioSensorReg(OUTPUT_MODE, 0x02, &reportPeriodValue, 1);

}
