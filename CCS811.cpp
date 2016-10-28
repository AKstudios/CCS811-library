/***************************************************
  This is a library for the CCS811 digital TVOC/eCO2 Sensor by CCMOSS/AMS
  http://www.ccmoss.com/gas-sensors#CCS811

  October 28, 2016

  The sensor uses I2C protocol to communicate, and requires 2 pins - SDA and SCL
  Another GPIO is also required to assert the WAKE pin for communication. this
  pin is passed by an argument in the begin function.

  A breakout board is available: https://github.com/AKstudios/CCS811-Breakout

  The ADDR pin on the sensor can be connected to VCC to set the address as 0x5A.
  The ADDR pin on the sensor can be connected to GND to set the address as 0x5B.

  Written by Akram Ali from AKstudios (www.akstudios.com)
  GitHub: https://github.com/AKstudios/
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include "CCS811.h"

CCS811::CCS811()
{

}


boolean CCS811::begin(uint8_t I2C_ADDR, uint8_t WAKE_PIN)
{
  _I2C_ADDR = I2C_ADDR;
  _WAKE_PIN = WAKE_PIN;
  Wire.begin();
  PORTD &= ~(1<<PORTD4);
  pinMode(_WAKE_PIN, OUTPUT);
  digitalWrite(_WAKE_PIN, LOW);  // WAKE_PIN on the sensor is active low, must always be asserted before any communication and held low throughout

  byte hw_id = CCS811::readHW_ID();
  if(hw_id != 0x81)  // this is the expected hardware ID
  {
    Serial.println("Error: Incorrect Hardware ID detected.");
    while(1);
  }

  byte status = CCS811::readStatus();
  uint8_t bit = (status & (1 << 5-1)) != 0; // black magic to read APP_VALID bit from STATUS register
  if(bit != 1)
  {
    Serial.println("Error: No application firmware loaded.");
    while(1);
  }

  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(APP_START);
  Wire.endTransmission();

  status = CCS811::readStatus();
  bit = (status & (1<<8-1)) !=0; // black magic to read FW_MODE bit from STATUS register
  if(bit != 1)
  {
    Serial.println("Error: Firmware still in boot mode.");
    while(1);
  }

  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(MEAS_MODE);
  Wire.write(0x00010000);  // constant power mode, IAQ measurement every second
  Wire.endTransmission();

  //CCS811::sleep();

  return true;
}


byte CCS811::readStatus(void)
{
  digitalWrite(_WAKE_PIN, LOW);
  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(STATUS);
  Wire.endTransmission();

  Wire.requestFrom(_I2C_ADDR, (uint8_t)1);
  byte status;
  if(Wire.available() == 1)
    status = Wire.read();

  return status;
}


byte CCS811::readHW_ID(void)
{
  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(HW_ID);
  Wire.endTransmission();

  Wire.requestFrom(_I2C_ADDR, (uint8_t)1);
  byte hw_id = Wire.read();

  return hw_id;
}


void CCS811::sleep()
{
  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(MEAS_MODE);
  Wire.write(0x00000000);  // sets sensor to idle; measurements are disabled; lowest power mode
  Wire.endTransmission();
  digitalWrite(_WAKE_PIN, HIGH);  // set WAKE_PIN high - this puts sensor in sleep mode and all I2C communications are ignored
}


void CCS811::readData(void)
{
  digitalWrite(_WAKE_PIN, LOW);
  //CCS811::compensate();
  boolean dataFlag = true;
  uint8_t buffer[4];
  /*
  byte status = CCS811::readStatus();
  uint8_t bit = (status & (1<<4-1)) !=0; // black magic to read DATA_READY bit from STATUS register
  //unsigned long t1 = millis(), t2;
  while(bit != 1)  // if bit is 0, no new samples are ready
  {
    dataFlag = false;
    delayMicroseconds(50);  // give some time before polling STATUS register again
    status = CCS811::readStatus();
    bit = (status & (1<<4-1)) !=0; // black magic to read DATA_READY bit from STATUS register
    if(bit == 1)  // if bit is 1, new data is ready in ALG_RESULT_DATA register
    {
      dataFlag = true;
      break;
    }
    t2 = millis();
    if(t2-t1 > 1000)  // timeout after 1 second if no new data is available
    {
      TVOC  =-1;
      CO2 = -1;
      break;
    }
  }
  */
  if(dataFlag)
  {
    Wire.beginTransmission(_I2C_ADDR);
    Wire.write(ALG_RESULT_DATA);    // reading ALG_RESULT_DATA clears DATA_READY bit in 0x00
    Wire.endTransmission();

    Wire.requestFrom(_I2C_ADDR, (uint8_t)4);
    if(Wire.available() != 4)
      return;

    for(uint8_t i=0; i<4; i++)
      buffer[i] = Wire.read();

    CO2 = ((uint8_t)buffer[0] << 8) + buffer[1];
    TVOC = ((uint8_t)buffer[2] << 8) + buffer[3];
  }
}


int CCS811::readTVOC(void)
{
  return TVOC;
}


int CCS811::readCO2(void)
{
  return CO2;
}


void CCS811::compensate(float t, float rh)    // compensate for temperature and relative humidity
{
  if(t>0)
    int _t = (int)t + 0.5;  // this will round off the floating point to the nearest integer value
  else if(t<0)
    int _t = (int)t - 0.5;

  int _t = _t + 25;  // temperature high byte is stored as T+25°C so the value of byte is positive

  int _rh = (int)rh + 0.5;  // this will round off the floating point to the nearest integer value

  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(ENV_DATA);
  //Wire.write()
  Wire.endTransmission();
}
