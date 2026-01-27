#include "EEPROM.h"

#define FwdPin_L PA5
#define BwdPin_L PA4
#define Speed_L PA8 // PA6
  
#define FwdPin_R PA12
#define BwdPin_R PA10
#define Speed_R PB0

#define LineSensorFR PA3  // Sensor 8
#define LineSensorFL PB1  // Sensor 3

#define MaxSpeed 100
#define TURBO_SPEED 255

#define COEFFICIENT_SPEED_R 1.0
#define COEFFICIENT_SPEED_L 1.0

#define EEPROM_SIZE 4

HardwareSerial SerialBT(PB4, PA2);  // D12, A7

int goingFromLineTime = 500; // ms
unsigned long startGoingFromLine = 0; // ms
int goingAroundFromLineTime = 1000; // ms
unsigned long startGoingAroundFromLine = 0; // ms

int maxesBlack[2] = {0, 0};

struct LineSensorsData 
{
  bool right;
  bool left;
};

void setup() {
  pinMode(LineSensorFR, INPUT);
  pinMode(LineSensorFL, INPUT);
  
  pinMode(FwdPin_R, OUTPUT);
  pinMode(BwdPin_R, OUTPUT);
  pinMode(FwdPin_L, OUTPUT);
  pinMode(BwdPin_L, OUTPUT);
  pinMode(Speed_R, OUTPUT);
  pinMode(Speed_L, OUTPUT);
  brake();

  for (int i=0; i<2; i++){
    maxesBlack[i] = EEPROM.read(2*i) * 256 + EEPROM.read(2*i + 1);
    if (maxesBlack[i] == 65535 || maxesBlack[i] == 0){
      maxesBlack[i] = 800;
    }
  }
  SerialBT.begin(115200);
}

void loop() {
  if (checkSomeLineCrossing()){
    goFromLine();
  }
  else {
    go_forward(TURBO_SPEED);
  }
//  delay(1);
}

void goFromLine(){
  LineSensorsData data = getLineCrossing();
  if (data.right && data.left){
    go_around(-MaxSpeed * 1.5, -MaxSpeed * 2.2);
//    go_around(-TURBO_SPEED, -TURBO_SPEED);
  }
  else if (data.right){
//    go_around(-MaxSpeed * 1.5, -MaxSpeed * 2.2);
//    go_around(-TURBO_SPEED, -TURBO_SPEED);
    go_around(0, TURBO_SPEED);
  }
  else if (data.left){
//    go_around(-MaxSpeed * 1.5, -MaxSpeed * 2.2);
//    go_around(-TURBO_SPEED, -TURBO_SPEED);
    go_around(TURBO_SPEED, 0);
  }
  else {
    go_around(TURBO_SPEED, TURBO_SPEED);
  }
}

bool checkSomeLineCrossing(){
  LineSensorsData data = getLineCrossing();
  return (data.right || data.left);
}

LineSensorsData getLineCrossing()
{
  LineSensorsData data;
  int r = analogRead(LineSensorFR) - maxesBlack[0] + 200;
  int l = analogRead(LineSensorFL) - maxesBlack[1] + 200;
//  SerialBT.print(r >= 0); SerialBT.print(" "); SerialBT.println(l >= 0);
  data.right = (r >= 0);
  data.left = (l >= 0);
  return data;
}

void brake(){
  digitalWrite(BwdPin_R,LOW);
  digitalWrite(BwdPin_L,LOW);
  
  digitalWrite(FwdPin_R,LOW);   
  digitalWrite(FwdPin_L,LOW);

  digitalWrite(Speed_R,LOW);
  digitalWrite(Speed_L,LOW);
}


void go_forward(int speed){
  digitalWrite(BwdPin_R,!(speed >= 0));
  digitalWrite(FwdPin_R,(speed >= 0)); 
  
  digitalWrite(BwdPin_L,!(speed >= 0)); 
  digitalWrite(FwdPin_L,(speed >= 0));
  
  analogWrite(Speed_R, speed>=0 ? speed * COEFFICIENT_SPEED_R : -speed * COEFFICIENT_SPEED_R);
  analogWrite(Speed_L, speed>=0 ? speed * COEFFICIENT_SPEED_L : -speed * COEFFICIENT_SPEED_L);
}

void go_around(int speedR, int speedL){
  digitalWrite(BwdPin_R,!(speedR >= 0));
  digitalWrite(FwdPin_R,(speedR >= 0)); 
  
  digitalWrite(BwdPin_L,!(speedL >= 0)); 
  digitalWrite(FwdPin_L,(speedL >= 0));
  speedR *= COEFFICIENT_SPEED_R;
  speedL *= COEFFICIENT_SPEED_L;
  analogWrite(Speed_R, speedR>=0 ? speedR : -speedR);
  analogWrite(Speed_L, speedL>=0 ? speedL : -speedL);
}
