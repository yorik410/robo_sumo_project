#define FwdPin_L PA5
#define BwdPin_L PA4
#define Speed_L PA8 // PA6
  
#define FwdPin_R PA12
#define BwdPin_R PA10
#define Speed_R PB0

#define MaxSpeed 100
#define TURBO_SPEED MaxSpeed * 2.2

#define COEFFICIENT_SPEED_R 1.25
#define COEFFICIENT_SPEED_L 1.0

HardwareSerial SerialBT(PB4, PA2);  // D12, A7

int stage = -1;

String a[9] = {"forward", "brake", "forward_turbo", "brake", "right", "left", "brake", "backward", "brake"};

void setup() {
  pinMode(FwdPin_R, OUTPUT);
  pinMode(BwdPin_R, OUTPUT);
  pinMode(FwdPin_L, OUTPUT);
  pinMode(BwdPin_L, OUTPUT);
  pinMode(Speed_R, OUTPUT);
  pinMode(Speed_L, OUTPUT);
  brake();

  SerialBT.begin(115200);
}

void loop() {
  if (SerialBT.available()){
    SerialBT.readString();
    stage = (stage + 1) % 5;
    SerialBT.println(a[stage]);
  }
  switch(stage){
    case 0:
      go_around(MaxSpeed, 0);
      break;
    case 1:
      go_around(-MaxSpeed, 0);
      break;
    case 2:
      go_around(MaxSpeed, 0);
      break;
    case 3:
      go_around(MaxSpeed, 0);
      break;
    case 4:
      brake();
      break;
  }
//  delay(1);  
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
