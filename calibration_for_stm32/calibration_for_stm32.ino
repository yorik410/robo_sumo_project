#include "EEPROM.h"

#define LineSensorFR PA3  // Sensor 8
#define LineSensorFL PB1  // Sensor 3


#define EEPROM_SIZE 4


int maxesBlack[2] = {0, 0};
int current[2] = {0, 0};
int prevValues[2] = {0, 0};

bool flag = false;

HardwareSerial SerialBT(PB4, PA2);  // D12, A7

void setup() {
  // put your setup code here, to run once:
  pinMode(LineSensorFR, INPUT);
  pinMode(LineSensorFL, INPUT);

  SerialBT.begin(115200);
  while (!SerialBT.available()){}
  SerialBT.readString();
  SerialBT.println("prev values");
  for (int i=0; i<2; i++){
    SerialBT.print(EEPROM.read(2*i) * 256 + EEPROM.read(2*i + 1)); SerialBT.print(" | ");
    prevValues[i] = EEPROM.read(2*i) * 256 + EEPROM.read(2*i + 1);
  }
  SerialBT.println();
  while (!SerialBT.available()){}
  SerialBT.readString();
  SerialBT.println("Start");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!flag){
    current[0] = analogRead(LineSensorFR);
    current[1] = analogRead(LineSensorFL);
//    current[2] = analogRead(LineSensorBR);
//    current[3] = analogRead(LineSensorBL);
  
    for (int i=0; i<2; i++){
      if (current[i] > maxesBlack[i]){
        maxesBlack[i] = current[i];
      }
      SerialBT.print(current[i]); SerialBT.print(" | ");
    }
    SerialBT.println();
  
    if (SerialBT.available()){
      SerialBT.readString();
      SerialBT.println("Black");
      for (int i=0; i<2; i++){
        SerialBT.print(maxesBlack[i]); SerialBT.print(" | ");
        EEPROM.write(2*i, floor(maxesBlack[i] / 256));
        EEPROM.write(2*i + 1, maxesBlack[i] % 256);
      }
      SerialBT.println();
      flag = true;
    }
  }
  
  delay(100);
}
