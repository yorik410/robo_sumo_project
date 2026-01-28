#include "EEPROM.h"
#include <Wire.h>
#include <VL53L0X.h>

#define FwdPin_L PA5
#define BwdPin_L PA4
#define Speed_L PA8 // PA6
  
#define FwdPin_R PA12
#define BwdPin_R PA10
#define Speed_R PB0

#define PIN_TRIG PA11 // TRIG pin (Sensor 6)
#define PIN_ECHO PB5 // ECHO pin (Sensor 7)
#define MAX_DISTANCE 100 // (cm) Константа для определения максимального расстояния, которое мы будем считать корректным

#define LineSensorFR PA3  // Sensor 8
#define LineSensorFL PB1  // Sensor 3

#define MaxSpeed 100
#define TURBO_SPEED MaxSpeed * 2.2

#define COEFFICIENT_SPEED_R 1.25
#define COEFFICIENT_SPEED_L 1.0

#define EEPROM_SIZE 4

VL53L0X distLaserSensor;  // i2c Sensor 1 (SDA), Sensor 2 (SCL)
HardwareSerial SerialBT(PB4, PA2);  // D12, A7

int stage = 0;
bool turboMode = false;

unsigned int prevDist = 0;
unsigned int prevLaserDist = 0;

int ignoreTime = 0; // ms
unsigned long startIgnoring = 0; // ms
bool interruptIgnoring = false;
const int turnTimeMax = 1000; // ms
unsigned long startTurning = 0; // ms
const int distCheckTimeMax = 50; // ms
unsigned long lastDistCheck = 0; // ms
const int laserDistCheckTimeMax = 25; // ms
unsigned long laserLastDistCheck = 0; // ms

int maxesBlack[2] = {0, 0};

struct LineSensorsData 
{
  bool right;
  bool left;
};

bool laserDistInit = true;

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
   Wire.begin();

   distLaserSensor.setTimeout(50);
   distLaserSensor.setSignalRateLimit(0.1);
   distLaserSensor.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
   distLaserSensor.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
   if (!distLaserSensor.init())
   {
     SerialBT.println("Failed to detect and initialize distLaserSensor!");
     laserDistInit = false;
   }
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
//  delay(1);
}

void findEnemy(){
  if (checkSomeLineCrossing()) {stage = 2; return;}
  if (startTurning == 0) startTurning = millis();
  if (!checkEnemyVisibility()){ // we are going around while enemy isn't being found
    if (millis() - turnTimeMax >= startTurning){ // if enemy hasn't been found
      startTurning = 0;
      startIgnoring = millis();
      ignoreTime = 1000; // ms
      interruptIgnoring = true;
      stage = 1;
    }
    else go_around(MaxSpeed, -MaxSpeed);
  }
  else {  // if enemy has been found
    startTurning = 0;
    startIgnoring = millis();
    ignoreTime = 1000; // ms
    interruptIgnoring = false;
    turboMode = true;
    SerialBT.println("found");
    stage = 1;
    brake();
  }
}

void goToEnemy(){
  if (checkSomeLineCrossing()) {stage = 2; return;}

  turboMode = checkEnemyVisibility();
  
  if (turboMode) go_forward(TURBO_SPEED); // turbo to enemy
  else go_forward(MaxSpeed); // normal behaviour
  
  if (ignoreTime == 0){ // if we don't ignore anything
    if (!checkEnemyVisibility()) stage = 0;
  }
  if (ignoreTime > 0 && millis() - ignoreTime >= startIgnoring){ // if ignoring ran out of time
    if (!checkEnemyVisibility()) stage = 0; // if we don't see enemy we should find it
    ignoreTime = 0; // reset ignoring
  }
  if (ignoreTime > 0 && interruptIgnoring){ // if ignoring can be interrupted by vision
    if (checkEnemyVisibility()) ignoreTime = 0; // if we see enemy we should reset ignoring
  }
}

void goFromLine(){
  LineSensorsData data = getLineCrossing();
  if (data.right && data.left){
    go_around(-TURBO_SPEED, -TURBO_SPEED);
  }
  else if (data.right){
    go_around(-MaxSpeed * 1.5, -MaxSpeed * 2.2);
//    go_around(-TURBO_SPEED, -TURBO_SPEED);
//    go_around(0, TURBO_SPEED);
  }
  else if (data.left){
    go_around(-MaxSpeed * 1.5, -MaxSpeed * 2.2);
//    go_around(-TURBO_SPEED, -TURBO_SPEED);
//    go_around(TURBO_SPEED, 0);
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
  unsigned int dist = checkDistance();
  unsigned int dist1 = checkDistanceLaser();
  return (((0 < dist) && (dist <= MAX_DISTANCE)) || ((0 < dist1) && (dist1 <= MAX_DISTANCE * 10)));
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
    lastDistCheck = millis();
    unsigned int distance;
    long duration;
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    duration = pulseIn(PIN_ECHO, HIGH, 8000);
    prevDist = distance = duration * 0.034 / 2;  // centimeter
    SerialBT.print("US: "); SerialBT.println(distance);
    return distance;
  }
  else {
    return prevDist;
  }
}

int checkDistanceLaser(){
  if (laserDistInit){
    if (millis() - laserDistCheckTimeMax >= laserLastDistCheck){
      laserLastDistCheck = millis();
      int dist = distLaserSensor.readRangeSingleMillimeters();
      if (distLaserSensor.timeoutOccurred()) { SerialBT.println("TIMEOUT"); return 0;}
      prevLaserDist = dist;
      SerialBT.print("Laser: "); SerialBT.println(dist);
      return dist;
    }
    else{
      return prevLaserDist;
    }
  }
  return 0;
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
  
  analogWrite(Speed_R, speed>=0 ? (int)constrain(speed * COEFFICIENT_SPEED_R, 0, 255) : (int)constrain(-speed * COEFFICIENT_SPEED_R, 0, 255));
  analogWrite(Speed_L, speed>=0 ? (int)constrain(speed * COEFFICIENT_SPEED_L, 0, 255) : (int)constrain(-speed * COEFFICIENT_SPEED_L, 0, 255));
}

void go_around(int speedR, int speedL){
  digitalWrite(BwdPin_R,!(speedR >= 0));
  digitalWrite(FwdPin_R,(speedR >= 0)); 
  
  digitalWrite(BwdPin_L,!(speedL >= 0)); 
  digitalWrite(FwdPin_L,(speedL >= 0));
  speedR *= COEFFICIENT_SPEED_R;
  speedL *= COEFFICIENT_SPEED_L;
  speedR = constrain(speedR, -255, 255);
  speedL = constrain(speedL, -255, 255);
  analogWrite(Speed_R, speedR>=0 ? speedR : -speedR);
  analogWrite(Speed_L, speedL>=0 ? speedL : -speedL);
}
