#include "HX711.h"

#define DOUT1  3
#define CLK1  2

#define DOUT2  5
#define CLK2  4

HX711 scale, scale1;

float calibration_factor = -91000; //Change this for calibration your load cell

void setup() {
  Serial.begin(9600);
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");
  
  scale.begin(DOUT1, CLK1);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0

  scale1.begin(DOUT2, CLK2);
  scale1.set_scale();
  scale1.tare(); //Reset the scale to 0
  
  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor 1: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);

  zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor 2: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
}

void loop() {
  
  
    scale.set_scale(-89000); //Adjust to this calibration factor
    scale1.set_scale(calibration_factor); //Adjust to this calibration factor

    float sum = abs(scale.get_units())*0.453592 + abs(scale1.get_units())*0.453592;
    Serial.print("SUM: ");
    Serial.print(sum, 4);
    Serial.print(" kg"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor);
    Serial.println();

    Serial.print("Reading1: ");
    Serial.print(abs(scale.get_units())*0.453592, 4);
    Serial.print(" kg"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor);
    Serial.println();
    Serial.print("Reading2: ");
    Serial.print(abs(scale1.get_units())*0.453592, 4);
    Serial.print(" kg"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor);
    Serial.println();

    delay(1000);

    if(Serial.available())
    {
      char temp = Serial.read();
      if(temp == '+' || temp == 'a')
        calibration_factor += 10;
      else if(temp == '-' || temp == 'z')
        calibration_factor -= 10;
    }
  
}