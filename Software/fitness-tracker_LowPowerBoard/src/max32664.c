#include "max32664.h"
#include "timer.h"


#define MAX32664_RESET_PIN 11  // Example pin for RESET (Port B, Pin 11)
#define MAX32664_MFIO_PIN  14  // Example pin for MFIO (Port B, Pin 14)
#define MAX32664_I2C_ADDR  0x55 // Default I2C address for MAX32664
#define CMD_DELAY_IN_US 60000


uint8_t readResponse[4];
uint8_t lastReadSize=0;
I2C_TransferSeq_TypeDef i2cTransfer;

static bioSensorHub_t max32664_hub;
I2C_TransferSeq_TypeDef i2cTransfer;


void max32664StartInitAppmode(void)
{
  // Initialize hub structure
  max32664_hub.device.i2c_cfg.i2c_port = I2C0;
  max32664_hub.device.i2c_cfg.address = MAX32664_I2C_ADDR;
  max32664_hub.resetPin = MAX32664_RESET_PIN;
  max32664_hub.mfioPin = MAX32664_MFIO_PIN;

  // Configure RESET and MFIO pins
  GPIO_PinModeSet(gpioPortB, max32664_hub.resetPin, gpioModePushPull, 0);
 // GPIO_PinModeSet(gpioPortB, max32664_hub.mfioPin, gpioModePushPull, 0);

  // Enter application mode
//  GPIO_PinOutSet(gpioPortB,  max32664_hub.mfioPin);
  GPIO_PinOutClear(gpioPortB,  max32664_hub.resetPin);
  timerWaitUs_interrupt(10000);

}


void setBioSensorHubResetPin()
{
    GPIO_PinOutSet(gpioPortB, max32664_hub.resetPin);
}

void setBioSensorHubMfioPin()
{
  GPIO_PinModeSet(gpioPortB, max32664_hub.mfioPin, gpioModeInputPull, 1);
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
void readDeviceMode()
{
  lastReadSize=1;
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


uint8_t getLastReadBuffer(const uint8_t** readBuffer)
{
  *readBuffer=readResponse;
  return lastReadSize;
}
