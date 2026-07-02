#include <Arduino.h>

#define PWMA 32
#define AIN1 33
#define AIN2 13
#define PWMB 21
#define BIN1 22
#define BIN2 23
#define STBY 18

#define PWM_FREQ 5000
#define PWM_RES 8
#define PWM_CH_A 0
#define PWM_CH_B 1

void setLeftMotor(int speedVal) {
  speedVal = constrain(speedVal, -255, 255);
  if (speedVal >= 0) {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
  } else {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    speedVal = -speedVal;
  }
  ledcWrite(PWM_CH_A, speedVal);
}

void setRightMotor(int speedVal) {
  speedVal = constrain(speedVal, -255, 255);
  if (speedVal >= 0) {
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
  } else {
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
    speedVal = -speedVal;
  }
  ledcWrite(PWM_CH_B, speedVal);
}

void stopAll() {
  ledcWrite(PWM_CH_A, 0);
  ledcWrite(PWM_CH_B, 0);
}

void setup() {
  Serial.begin(115200);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);

  ledcSetup(PWM_CH_A, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWMA, PWM_CH_A);
  ledcAttachPin(PWMB, PWM_CH_B);

  Serial.println("Motor check start");
}

void loop() {
  Serial.println("Left motor forward");
  setLeftMotor(150);
  stopAll();
  delay(1000);
  setLeftMotor(150);
  delay(1500);
  stopAll();
  delay(1000);

  Serial.println("Left motor reverse");
  setLeftMotor(-150);
  delay(1500);
  stopAll();
  delay(1000);

  Serial.println("Right motor forward");
  setRightMotor(150);
  delay(1500);
  stopAll();
  delay(1000);

  Serial.println("Right motor reverse");
  setRightMotor(-150);
  delay(1500);
  stopAll();
  delay(1000);

  Serial.println("Both motors forward");
  setLeftMotor(150);
  setRightMotor(150);
  delay(2000);
  stopAll();
  delay(1500);
}