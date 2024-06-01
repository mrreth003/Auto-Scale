#include <SD.h>
#include <SPI.h>
#include <avr/sleep.h>
#include "HX711.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define DOUT1 3
#define CLK1 2

#define DOUT2 5
#define CLK2 4

#define ONE_WIRE_BUS 7

//FUNCTION DECLARATIONS
void movingAvgFilter(float* values, int bufferUsed, int windowSize);
void writeToFile(float meanVal, int startT, float tempVal);
void takePic();
void wakeUpISR();
void readScaleVals();
void sleep();

//VARIABLE DECLARATIONS
HX711 scale1, scale2;
float calibration_factor = -91000;  // Change this for calibration your load cell

float scale1Value = 0;  // Reading from the HX711
float scale2Value = 0;
float totalScaleValue = 0;
unsigned long startTime1;  // Start time of the current sampling
unsigned long startTime2;

File myFile;                    // File to write to on SD card
const int chipSelect = 53;      // Pin 10 of Arduino
unsigned long startMillis = 0;  // Time once setup is all done

const int wakeUpPin = 3;            // Pin set for waking from sleep mode.
const float wakeUpThreshold = 2.0;  // Initial threshold value
const float secondBirdThreshold = 4.0;

const int picPin = 39;

int dataNumber = 0;

OneWire oneWire(ONE_WIRE_BUS);  // Temp Sensor
DallasTemperature sensors(&oneWire);
float meanValue2 = 0;
float tempValue2 = 0;

float sampleBuffer1[200];  // Sample buffer
float sampleBuffer2[200];
int bufferIndex1 = 0;  // Index of the last written sample in the buffer
int bufferIndex2 = 0;
int numBirds = 0;  // Number of birds on the scale at one time


//RUNNING OF ARDUINO
void setup() {
  Serial.begin(9600);
  sensors.begin();

  // testing for presence of SD card
  if (!(SD.begin(chipSelect))) {
    Serial.println("SD card missing or failure.");
    while (1)
      ;  //wait here forever

  } else {
    Serial.println("SD card is present and ready.");
  }

  // scale1
  scale1.begin(DOUT1, CLK1);
  scale1.set_scale(calibration_factor);
  scale1.tare();  //Reset the scale1 to 0
  // scale2
  scale2.begin(DOUT2, CLK2);
  scale2.set_scale(calibration_factor);
  scale2.tare();  //Reset the scale2 to 0
}

void loop() {
  readScaleVals();

  numBirds = 0;
  bufferIndex1 = 0;
  bufferIndex2 = 0;

  float sum;
  float meanValue1 = totalScaleValue;
  sensors.requestTemperatures();
  float tempValue1 = sensors.getTempCByIndex(0);

  //reads values and stores into array depending on number of birds on scale
  while (totalScaleValue > wakeUpThreshold) {
    if (bufferIndex1 < 200 && bufferIndex2 < 200) {
      if (bufferIndex1 == 0) { //first reading of bird 1
        startTime1 = millis();
        takePic();
        sampleBuffer1[bufferIndex1++] = totalScaleValue;
      } else if ((totalScaleValue < secondBirdThreshold)) {
        sampleBuffer1[bufferIndex1++] = totalScaleValue;
      } else if (totalScaleValue >= secondBirdThreshold) {
        if (numBirds == 0) { //first reading of bird 2
          startTime2 = millis();
          takePic();
          numBirds++;
        }
        sampleBuffer2[bufferIndex2++] = totalScaleValue;
      } else if (totalScaleValue < (sampleBuffer2[bufferIndex2 - 1] - wakeUpThreshold)) {
        break;
      }
    }
    readScaleVals();
  }

  // Check if the threshold value has been dropped
  if (totalScaleValue < wakeUpThreshold && bufferIndex1 > 0) {
    // movingAvgFilter(sampleBuffer1, bufferIndex1, 3);
    // movingAvgFilter(sampleBuffer2, bufferIndex2, 3);
    for (int i = 0; i < bufferIndex1; i++) {
      Serial.print(String(sampleBuffer1[i]) + " ");
    }
    // Calculate the mean value
    sum = 0;
    for (int i = 0; i < bufferIndex1; i++) {
      sum += sampleBuffer1[i];
    }
    Serial.println(String(sum) + " " + String(bufferIndex1));
    meanValue1 = sum / bufferIndex1;
    Serial.println(meanValue1);

    if (bufferIndex2 > 0) {
      sum = 0;
      for (int i = 0; i < bufferIndex2; i++) {
        sum += sampleBuffer2[i];
      }

      for (int i = 0; i < bufferIndex2; i++) {
        Serial.print(String(sampleBuffer2[i]) + " ");
      }
      meanValue2 = sum / bufferIndex2 - meanValue1;
      Serial.println(String(sum) + " " + String(bufferIndex2));
      Serial.println(meanValue2);
      tempValue2 = sensors.getTempCByIndex(0);
    }
    if (meanValue1 > 0) {
      writeToFile(meanValue1, startTime1, tempValue1);
      Serial.println(7);
      if (bufferIndex2 > 0) {
        Serial.println(8);
        writeToFile(meanValue2, startTime2, tempValue2);
      }
    }
  }


  // sleep();
}

//FUNCTIONS
void movingAvgFilter(float* values, int bufferUsed, int windowSize) {
  float window[windowSize];  // Window to hold the last few values
  float sum = 0;             // Sum of the values in the window

  // Initialize the window
  for (int i = 0; i < windowSize; i++) {
    window[i] = values[i];
    sum += values[i];
  }

  // Apply the moving average filter
  for (int i = windowSize; i < bufferUsed; i++) {
    values[i] = sum / windowSize;  // Update the current array element with the average

    sum -= window[i - windowSize];  // Remove the oldest value from the sum
    for (int j = 0; j < windowSize - 1; j++) {
      window[j] = window[j + 1];  // Shift the window values
    }

    window[windowSize - 1] = values[i];  // Add the newest value to the window
    sum += values[i];                    // Add the newest value to the sum
  }
}

void writeToFile(float meanVal, int startT, float tempVal) {
  myFile = SD.open("/data" + String(dataNumber++) + ".txt", FILE_WRITE);

  if (myFile) {
    myFile.print(startT);
    myFile.print(";");
    myFile.print(meanVal);
    myFile.print(";");
    myFile.print(tempVal);
    myFile.print(";");
    myFile.println(tempVal / 10 * 0.0005 * 20 * 1000);

    myFile.close();
  }
}

void takePic() {
  digitalWrite(picPin, HIGH);  // Set the output pin to LOW - take pic
  delay(5000);                 // Wait for 5 second
  digitalWrite(picPin, LOW);   // Set the output pin to HIGH - do not take another pic
}

void wakeUpISR() {
  if (totalScaleValue > wakeUpThreshold) {
    loop();
  } else {
    sleep();
  }
}

void readScaleVals() {
  scale1Value = abs(scale1.get_units()) * 0.453592;  // Gets the average of the last defined number of readings minus the tare weight divided by the calibration factor.
  scale2Value = abs(scale2.get_units()) * 0.453592;
  totalScaleValue = scale1Value + scale2Value;
}

void sleep() {
  // Put the Arduino to sleep
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
}

// end of functions