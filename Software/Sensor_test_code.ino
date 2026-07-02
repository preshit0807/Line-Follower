#include <Arduino.h>

const int SENSOR_PINS[8] = {34, 35, 32, 33, 25, 26, 27, 14};
const int NUM_SENSORS = 8;
const int STATUS_LED = 2;
const int CALIB_BTN = 0;

int sensorMin[NUM_SENSORS];
int sensorMax[NUM_SENSORS];
int sensorNorm[NUM_SENSORS];

void readRawSensors(int *vals) {
  for (int i = 0; i < NUM_SENSORS; i++) {
    vals[i] = analogRead(SENSOR_PINS[i]);
  }
}

int normalizeSensor(int raw, int index) {
  int range = sensorMax[index] - sensorMin[index];
  if (range <= 0) return 0;
  int val = ((sensorMax[index] - raw) * 1000) / range;
  return constrain(val, 0, 1000);
}

void readNormalizedSensors() {
  int raw[NUM_SENSORS];
  readRawSensors(raw);
  for (int i = 0; i < NUM_SENSORS; i++) {
    sensorNorm[i] = normalizeSensor(raw[i], i);
  }
}

void calibrateSensors() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    sensorMin[i] = 4095;
    sensorMax[i] = 0;
  }

  int raw[NUM_SENSORS];
  unsigned long start = millis();

  while (millis() - start < 3000) {
    readRawSensors(raw);
    for (int i = 0; i < NUM_SENSORS; i++) {
      if (raw[i] < sensorMin[i]) sensorMin[i] = raw[i];
      if (raw[i] > sensorMax[i]) sensorMax[i] = raw[i];
    }
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
    delay(10);
  }

  digitalWrite(STATUS_LED, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(CALIB_BTN, INPUT_PULLUP);
  digitalWrite(STATUS_LED, LOW);

  Serial.println("Press BOOT to calibrate sensors");
  while (digitalRead(CALIB_BTN) == HIGH) delay(20);

  calibrateSensors();
  Serial.println("Calibration done");
}

void loop() {
  int raw[NUM_SENSORS];
  readRawSensors(raw);
  readNormalizedSensors();

  Serial.print("Raw: ");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(raw[i]);
    if (i < NUM_SENSORS - 1) Serial.print('\t');
  }
  Serial.println();

  Serial.print("IR:");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(sensorNorm[i]);
    if (i < NUM_SENSORS - 1) Serial.print(",");
  }
  Serial.println();

  delay(150);
}