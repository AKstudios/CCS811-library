/***************************************************
  This is a library for the CCS811 digital TVOC/eCO2 Sensor by CCMOSS/AMS
  http://www.ccmoss.com/gas-sensors#CCS811

  November 8, 2016 [Hillary or Trump?]
  November 9, 2016 [Update: Nevermind.]

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
  pinMode(_WAKE_PIN, OUTPUT);   // set WAKE pin as OUTPUT
  //PORTD &= ~(1<<PORTD4);  // assert WAKE pin LOW to initiate communication with sensor
  digitalWrite(_WAKE_PIN, LOW);  // WAKE_PIN on the sensor is active low, must always be asserted before any communication and held low throughout

  byte hw_id = CCS811::readHW_ID();
  if(hw_id != 0x81)  // this is the expected hardware ID
  {
    Serial.println("Error: Incorrect Hardware ID detected.");
    return false;
  }

  byte status = CCS811::readStatus();
  uint8_t bit = (status & (1 << 5-1)) != 0; // black magic to read APP_VALID bit from STATUS register
  if(bit != 1)
  {
    Serial.println("Error: No application firmware loaded.");
    readErrorID(status);
    return false;
  }

  digitalWrite(_WAKE_PIN, LOW);
  Wire.beginTransmission(_I2C_ADDR); // least significant bit indicates write (0) or read (1)
  Wire.write(APP_START);
  Wire.endTransmission();
  digitalWrite(_WAKE_PIN, HIGH);

  status = CCS811::readStatus();
  bit = (status & (1<<8-1)) !=0; // black magic to read FW_MODE bit from STATUS register
  if(bit != 1)
  {
    Serial.println("Error: Firmware still in boot mode.");
    readErrorID(status);
    return false;
  }

  digitalWrite(_WAKE_PIN, LOW);
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
  digitalWrite(_WAKE_PIN, LOW);
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
  digitalWrite(_WAKE_PIN, LOW);
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
  digitalWrite(_WAKE_PIN, LOW);
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
  digitalWrite(_WAKE_PIN, LOW);
  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(MEAS_MODE);
  Wire.write(0x00000000);  // sets sensor to idle; measurements are disabled; lowest power mode
  Wire.endTransmission();
  digitalWrite(_WAKE_PIN, HIGH);  // set WAKE_PIN high - this puts sensor in sleep mode (~2uA) and all I2C communications are ignored
}


void CCS811::getData(void)
{
  //CCS811::compensate(t, rh);
  digitalWrite(_WAKE_PIN, LOW);
  delay(1000);

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



/*
void CCS811::readData(void)
{
  digitalWrite(_WAKE_PIN, LOW);
  //CCS811::compensate();
  //boolean dataFlag = true;
  uint8_t buffer[4];
  uint8_t bit = 0;
  //unsigned long t1 = millis(), t2;
  while(bit != 1)  // if bit is 0, no new samples are ready
  {
    byte status = CCS811::readStatus();
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
    dataFlag = false;
    delayMicroseconds(50);  // give some time before polling STATUS register again
  }

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
*/

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
  digitalWrite(_WAKE_PIN, LOW);

  int _temp, _rh;
  if(t>0)
    _temp = (int)t + 0.5;  // this will round off the floating point to the nearest integer value
  else if(t<0)
    _temp = (int)t - 0.5;
  _temp = _temp + 25;  // temperature high byte is stored as T+25°C so the value of byte is positive
  _rh = (int)rh + 0.5;  // this will round off the floating point to the nearest integer value

  Wire.beginTransmission(_I2C_ADDR);
  Wire.write(ENV_DATA);
  Wire.write(_rh);           // 7 bit humidity value
  Wire.write(0);            // most significant fractional bit. Using 0 here - gives us accuracy of +/-1%. Current firmware (2016) only supports fractional increments of 0.5
  Wire.write(_temp);
  Wire.write(0);
  Wire.endTransmission();

  digitalWrite(_WAKE_PIN, HIGH);
}
