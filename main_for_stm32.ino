#include "EEPROM.h"

#define FwdPin_L PA5
#define BwdPin_L PA4
#define Speed_L PA8 // PA6
  
#define FwdPin_R PA12
#define BwdPin_R PA10
#define Speed_R PB0

#define PIN_TRIG PA11 // TRIG pin (Sensor 6)
#define PIN_ECHO PB5 // ECHO pin (Sensor 7)
#define MAX_DISTANCE 150 // (cm) Константа для определения максимального расстояния, которое мы будем считать корректным

#define LineSensorFR PA3  // Sensor 8
#define LineSensorFL PB1  // Sensor 3

#define MaxSpeed 92

#define EEPROM_SIZE 4

HardwareSerial SerialBT(PB7, PB6);

int stage = 0;

unsigned int prevDist = 0;

int ignoreTime = 0; // ms
unsigned long startIgnoring = 0; // ms
bool interruptIgnoring = false;
const int turnTimeMax = 1000; // ms
unsigned long startTurning = 0; // ms
const int distCheckTimeMax = 50; // ms
unsigned long lastDistCheck = 0; // ms

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

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  for (int i=0; i<2; i++){
    maxesBlack[i] = EEPROM.read(2*i) * 256 + EEPROM.read(2*i + 1);
    if (maxesBlack[i] == 65535 || maxesBlack[i] == 0){
      maxesBlack[i] = 800;
    }
  }
   SerialBT.begin(115200);
}

void loop() {
  switch(stage){
    case 0:
      findEnemy();
      break;
    case 1:
      goToEnemy();
      break;
    case 2:
      goFromLine();
      break;
  }
  delay(1);
}

void findEnemy(){
  if (checkSomeLineCrossing()) {stage = 2; return;}
  startTurning = millis();
  while (!checkEnemyVisibility()){ // we are going around while enemy isn't being found
    if (millis() - turnTimeMax >= startTurning){ // if enemy hasn't been found
      startIgnoring = millis();
      ignoreTime = 1000; // ms
      interruptIgnoring = true;
      stage = 1;
      return;
    }
    if (checkSomeLineCrossing()) {stage = 2; return;}
    go_around(MaxSpeed, -MaxSpeed);
  }
  // if enemy has been found
  brake();
  startIgnoring = millis();
  ignoreTime = 2000; // ms
  interruptIgnoring = false;
  stage = 1;
}

void goToEnemy(){
  if (checkSomeLineCrossing()) {stage = 2; return;}
  
  go_forward(MaxSpeed); // normal behaviour
  
  if (ignoreTime == 0){ // if we don't ignore anything
    if (!checkEnemyVisibility) stage = 0;
  }
  if (ignoreTime > 0 && millis() - ignoreTime >= startIgnoring){ // if ignoring ran out of time
    if (!checkEnemyVisibility) stage = 0; // if we don't see enemy we should find it
    ignoreTime = 0; // we are starting checking for enemy
  }
  if (ignoreTime > 0 && interruptIgnoring){ // if ignoring can be interrupted by vision
    if (checkEnemyVisibility) ignoreTime = 0; // if we see enemy we should start checking for enemy
  }
}

void goFromLine(){
  LineSensorsData data = getLineCrossing();
  if (data.right){
    go_around(MaxSpeed, -MaxSpeed);
  }
  else if (data.left){
    go_around(MaxSpeed, -MaxSpeed);
  }
  else {
    stage = 0;
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

bool checkEnemyVisibility(){
  return (checkDistance() <= MAX_DISTANCE);
}

//int checkDistance(){
//  if (millis() - distCheckTimeMax >= lastDistCheck){
//    unsigned int distance = sonar.ping_cm(); // get distance
////    unsigned int distance = 25; // get distance
//    prevDist = distance;
//    lastDistCheck = millis();
//    return distance;
//  }
//  else {
//    return prevDist;
//  }
//}

int checkDistance(){
  if (millis() - distCheckTimeMax >= lastDistCheck){
    unsigned int distance;
    long duration;
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    duration = pulseIn(PIN_ECHO, HIGH);
    distance = duration * 0.034 / 2;  // centimeter
    SerialBT.println(distance);
    return distance;
  }
  else {
    return prevDist;
  }
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
  
  analogWrite(Speed_R, speed>=0 ? speed : -speed);
  analogWrite(Speed_L, speed>=0 ? speed : -speed);
}

void go_around(int speedR, int speedL){
  digitalWrite(BwdPin_R,!(speedR >= 0));
  digitalWrite(FwdPin_R,(speedR >= 0)); 
  
  digitalWrite(BwdPin_L,!(speedL >= 0)); 
  digitalWrite(FwdPin_L,(speedL >= 0));

  analogWrite(Speed_R, speedR>=0 ? speedR : -speedR);
  analogWrite(Speed_L, speedL>=0 ? speedL : -speedL);
}
