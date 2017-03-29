/***************************************************
  This is an example for the CCS811 digital TVOC/eCO2 Sensor by CCMOSS/AMS
  http://www.ccmoss.com/gas-sensors#CCS811

  Updated: March 28, 2017
  
  The sensor uses I2C protocol to communicate, and requires 2 pins - SDA and SCL
  Another GPIO is also required to assert the WAKE pin for communication. this
  pin is passed by an argument in the begin function.

  A breakout board is available: https://github.com/AKstudios/CCS811-Breakout

  The ADDR pin on the sensor can be connected to VCC to set the address as 0x5A.
  The ADDR pin on the sensor can be connected to GND to set the address as 0x5B.

  Written by Akram Ali from AKstudios (www.akstudios.com)
  GitHub: https://github.com/AKstudios/
 ****************************************************/

#include <CCS811.h>

//#define ADDR      0x5A
#define ADDR      0x5B
#define WAKE_PIN  4

CCS811 sensor;

void setup()
{
  Serial.begin(115200);
  Serial.println("CCS811 test");
  if(!sensor.begin(uint8_t(ADDR), uint8_t(WAKE_PIN)))
    Serial.println("Initialization failed.");
}

void loop()
{ 
  //sensor.compensate(22.56, 30.73);  // replace with t and rh values from sensor
  sensor.getData();
  Serial.print("CO2 concentration : "); Serial.print(sensor.readCO2()); Serial.println(" ppm");
  Serial.print("TVOC concentration : "); Serial.print(sensor.readTVOC()); Serial.println(" ppb");
  Serial.println();
  delay(2000);
}
