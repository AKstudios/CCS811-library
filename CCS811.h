/***************************************************
  This is a library for the CCS811 digital TVOC/eCO2 Sensor by CCMOSS/AMS
  http://www.ccmoss.com/gas-sensors#CCS811

  Updated: June 17, 2017

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

#ifndef CCS811_h
#define CCS811_h

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"  // to allow boolean and byte functions
#endif

#include "Wire.h"   // Wire library for I2C protocol

#define nWAKE           4      // pin 4 is tied to CCS811 nWAKE pin in sensor node
#define nINT            7      // pin 7 is tied to CCS811 nINT pin in sensor node

//#define ADDR            0x5A   // when I2C_ADDR pin is LOW
#define ADDR            0x5B   // when I2C_ADDR pin is HIGH

// Registers for CCS811
#define STATUS          0x00
#define MEAS_MODE       0x01
#define ALG_RESULT_DATA 0x02
#define ENV_DATA        0x05
#define APP_START       0xF4
#define HW_ID           0x20
#define ERROR_ID        0xE0
#define SW_RESET        0xFF

class CCS811
{
  public:
    CCS811();     // constructor
    boolean begin(uint8_t I2C_ADDR, uint8_t WAKE_PIN);
    byte readStatus(void);
    byte readHW_ID(void);
    byte readErrorID(byte _status);
    int readTVOC(void);
    int readCO2(void);
    void getData(void);
    void compensate(float t, float rh);
    void _digitalWrite(uint8_t WAKE_PIN, bool VAL);
    void reset(void);
    void sleep();
    int TVOC, CO2;

  private:
    uint8_t _WAKE_PIN;
    uint8_t _I2C_ADDR;
};

#endif
