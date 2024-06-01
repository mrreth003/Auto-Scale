#include <OneWire.h>
#include <DallasTemperature.h>
                          
#define ONE_WIRE_BUS 7          // Data wire is plugged into digital pin 2 on the Arduino
                                
OneWire oneWire(ONE_WIRE_BUS);	// Setup a oneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire); // Pass oneWire reference to DallasTemperature library

void setup(void)
{
  sensors.begin();	// Start up the library
}

void loop(void)
{ 
  sensors.requestTemperatures(); // Send the command to get temperatures
  float temp = sensors.getTempCByIndex(0);
}
