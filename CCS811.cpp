/***************************************************
  This is a library for the CCS811 digital TVOC/eCO2 Sensor by CCMOSS/AMS
  http://www.ccmoss.com/gas-sensors#CCS811

  Updated: July 13, 2017

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
  // empty constructor, just because.
}


boolean CCS811::begin(uint8_t I2C_ADDR, uint8_t WAKE_PIN)
{
  delay(70); // from datasheet - up to 70ms on the first Reset after new application download; up to 20ms delay after power on
  _I2C_ADDR = I2C_ADDR;
  _WAKE_PIN = WAKE_PIN;
  Wire.begin();
  pinMode(_WAKE_PIN, OUTPUT);   // set WAKE pin as OUTPUT
  //digitalWrite(_WAKE_PIN, LOW);  // WAKE_PIN on the sensor is active low, must always be asserted before any communication and held low throughout

  byte hw_id = readHW_ID();
  if(hw_id != 0x81)  // this is the expected hardware ID
  {
    Serial.println("Error: Incorrect Hardware ID detected.");
    return false;
  }

  byte status = readStatus();
  uint8_t bit = (status & (1 << 5-1)) != 0; // black magic to read APP_VALID bit from STATUS register
  if(bit != 1)
  {
    Serial.println("Error: No application firmware loaded.");
    readErrorID(status);
    return false;
  }

  _digitalWrite(_WAKE_PIN, LOW);
  delayMicroseconds(50); // recommended 50us delay after asserting WAKE pin
  Wire.beginTransmission(_I2C_ADDR); // least significant bit indicates write (0) or read (1)
  Wire.write(APP_START);
  Wire.endTransmission();
  digitalWrite(_WAKE_PIN, HIGH);

  status = readStatus();
  bit = (status & (1<<8-1)) !=0; // black magic to read FW_MODE bit from STATUS register
  if(bit != 1)
  {
    Serial.println("Error: Firmware still in boot mode.");
    readErrorID(status);
    return false;
  }

  _digitalWrite(_WAKE_PIN, LOW);
  delayMicroseconds(50); // recommended 50us delay after asserting WAKE pin
  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(MEAS_MODE);
  Wire.write(0x10);  // constant power mode, IAQ measurement every second
  Wire.endTransmission();
  //CCS811::sleep();
  digitalWrite(_WAKE_PIN, HIGH);

  return true;
}


byte CCS811::readStatus(void)
{
  delayMicroseconds(20); // recommended 20us delay while performing back to back I2C operations
  _digitalWrite(_WAKE_PIN, LOW);
  delayMicroseconds(50); // recommended 50us delay after asserting WAKE pin
  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(STATUS);
  Wire.endTransmission();

  Wire.requestFrom(_I2C_ADDR, (uint8_t)1);
  byte status;
  if(Wire.available() == 1)
    status = Wire.read();

  digitalWrite(_WAKE_PIN, HIGH);
  return status;
}


byte CCS811::readHW_ID(void)
{
  _digitalWrite(_WAKE_PIN, LOW);
  delayMicroseconds(50); // recommended 50us delay after asserting WAKE pin
  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(HW_ID);
  Wire.endTransmission();

  Wire.requestFrom(_I2C_ADDR, (uint8_t)1);
  byte hw_id = Wire.read();

  digitalWrite(_WAKE_PIN, HIGH);
  return hw_id;
}


byte CCS811::readErrorID(byte _status)
{
  _digitalWrite(_WAKE_PIN, LOW);
  delayMicroseconds(50); // recommended 50us delay after asserting WAKE pin
  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(ERROR_ID);
  Wire.endTransmission();

  Wire.requestFrom(_I2C_ADDR, (uint8_t)1);
  byte error_id = Wire.read();

  digitalWrite(_WAKE_PIN, HIGH);
  uint8_t bit = (_status & (1 << 1-1)) != 0; // black magic to read ERROR bit from STATUS register
  if(bit == 1)
  {
    Serial.print("Error ID: ");
    Serial.println(error_id);
  }

  return error_id;
}


void CCS811::sleep()
{
  _digitalWrite(_WAKE_PIN, LOW);
  delayMicroseconds(50); // recommended 50us delay after asserting WAKE pin
  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(MEAS_MODE);
  Wire.write(0x00000000);  // sets sensor to idle; measurements are disabled; lowest power mode
  Wire.endTransmission();
  digitalWrite(_WAKE_PIN, HIGH);  // set WAKE_PIN high - this puts sensor in sleep mode (~2uA) and all I2C communications are ignored
}


void CCS811::getData(void)
{
  //CCS811::compensate(t, rh);
  _digitalWrite(_WAKE_PIN, LOW);
  delayMicroseconds(50); // recommended 50us delay after asserting WAKE pin
  //delay(1000);

  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(ALG_RESULT_DATA);    // reading ALG_RESULT_DATA clears DATA_READY bit in 0x00
  Wire.endTransmission();

  Wire.requestFrom(_I2C_ADDR, (uint8_t)4);
  delay(1);
  int buffer[4];
  if(Wire.available() == 4)
  {
    for(int i=0; i<4; i++)
    {
      buffer[i] = Wire.read();
      //Serial.print(buffer[i]);
    }
  }
  CO2 = ((uint8_t)buffer[0] << 8) + buffer[1];
  TVOC = ((uint8_t)buffer[2] << 8) + buffer[3];

  digitalWrite(_WAKE_PIN, HIGH);
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
  _digitalWrite(_WAKE_PIN, LOW);
  delayMicroseconds(50); // recommended 50us delay after asserting WAKE pin
  int _temp, _rh;
  if(t>0)
    _temp = (int)t + 0.5;  // this will round off the floating point to the nearest integer value
  else if(t<0) // account for negative temperatures
    _temp = (int)t - 0.5;
  _temp = _temp + 25;  // temperature high byte is stored as T+25°C in the sensor's memory so the value of byte is positive
  _rh = (int)rh + 0.5;  // this will round off the floating point to the nearest integer value

  byte _ENV_DATA[4];

  _ENV_DATA[0] = _rh << 1;  // shift the binary number to left by 1. This is stored as a 7-bit value
  _ENV_DATA[1] = 0;  // most significant fractional bit. Using 0 here - gives us accuracy of +/-1%. Current firmware (2016) only supports fractional increments of 0.5
  _ENV_DATA[2] = _temp << 1;
  _ENV_DATA[3] = 0;

  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(ENV_DATA);
  Wire.write(_ENV_DATA[0]); // 7 bit humidity value
  Wire.write(_ENV_DATA[1]);
  Wire.write(_ENV_DATA[2]);
  Wire.write(_ENV_DATA[3]);
  Wire.endTransmission();

  digitalWrite(_WAKE_PIN, HIGH);
}

void CCS811::_digitalWrite(uint8_t WAKE_PIN, bool VAL)    // asserts WAKE pin with a small delay to ensure reliable communication - thanks to djdehaan for this fix
{
  digitalWrite(WAKE_PIN, VAL);
  delayMicroseconds(60);
}

// bruh
