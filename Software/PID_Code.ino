#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

const char* AP_SSID = "ESP32-LineFollower";
const char* AP_PASS = "12345678";
WebServer server(80);

#define PWMA 19
#define AIN1 21
#define AIN2 22
#define PWMB 23
#define BIN1 18
#define BIN2 13
#define STBY 5
#define PWM_FREQ 5000
#define PWM_RES 8

const int SENSOR_PINS[8] = {34,35,32,33,25,26,27,14};
const int NUM_SENSORS = 8;
const int STATUS_LED = 2;

int sensorMin[NUM_SENSORS];
int sensorMax[NUM_SENSORS];
int sensorNorm[NUM_SENSORS];

float Kp=0.40, Ki=0.00, Kd=1.00;
int baseSpeed=60, turnSpeed=180, maxSpeed=180;
float lastError=0, integral=0;
const float setPoint=3500.0;

bool botEnabled=false;
String currentPacket="IR:0,0,0,0,0,0,0,0 POS:-1";
String currentState="Idle";

void setMotors(int leftSpeed,int rightSpeed){
  leftSpeed=constrain(leftSpeed,-255,255);
  rightSpeed=constrain(rightSpeed,-255,255);
  if(leftSpeed>=0){digitalWrite(AIN1,HIGH);digitalWrite(AIN2,LOW);}
  else{digitalWrite(AIN1,LOW);digitalWrite(AIN2,HIGH);leftSpeed=-leftSpeed;}
  if(rightSpeed>=0){digitalWrite(BIN1,HIGH);digitalWrite(BIN2,LOW);}
  else{digitalWrite(BIN1,LOW);digitalWrite(BIN2,HIGH);rightSpeed=-rightSpeed;}
  ledcWrite(PWMA,leftSpeed);
  ledcWrite(PWMB,rightSpeed);
}

void stopMotors(){ledcWrite(PWMA,0);ledcWrite(PWMB,0);}
void readRawSensors(int*vals){for(int i=0;i<NUM_SENSORS;i++) vals[i]=analogRead(SENSOR_PINS[i]);}
int normalizeSensor(int raw,int index){int range=sensorMax[index]-sensorMin[index];if(range<=0) return 0;return constrain(((sensorMax[index]-raw)*1000)/range,0,1000);}
void readNormalizedSensors(){int raw[NUM_SENSORS];readRawSensors(raw);for(int i=0;i<NUM_SENSORS;i++) sensorNorm[i]=normalizeSensor(raw[i],i);}

int readLinePosition(){
  readNormalizedSensors();
  long weightedSum=0,total=0;
  bool seen=false;
  for(int i=0;i<NUM_SENSORS;i++){
    if(sensorNorm[i]>150) seen=true;
    weightedSum += (long)sensorNorm[i]*(i*1000);
    total += sensorNorm[i];
  }
  if(!seen || total==0) return -1;
  return weightedSum/total;
}

void calibrateSensors(){
  for(int i=0;i<NUM_SENSORS;i++){sensorMin[i]=4095;sensorMax[i]=0;}
  unsigned long start=millis();
  currentState="Calibrating";
  while(millis()-start<3000){
    int raw[NUM_SENSORS];
    readRawSensors(raw);
    for(int i=0;i<NUM_SENSORS;i++){
      if(raw[i]<sensorMin[i]) sensorMin[i]=raw[i];
      if(raw[i]>sensorMax[i]) sensorMax[i]=raw[i];
    }
    digitalWrite(STATUS_LED,!digitalRead(STATUS_LED));
    delay(10);
  }
  digitalWrite(STATUS_LED,LOW);
  currentState="Ready";
}

String buildTelemetry(int pos,const char* state){
  char buf[256];
  snprintf(buf,sizeof(buf),"STATE:%s IR:%d,%d,%d,%d,%d,%d,%d,%d POS:%d",state,
    sensorNorm[0],sensorNorm[1],sensorNorm[2],sensorNorm[3],
    sensorNorm[4],sensorNorm[5],sensorNorm[6],sensorNorm[7],pos);
  return String(buf);
}

void updatePacket(int pos,const char* state){
  currentPacket=buildTelemetry(pos,state);
  Serial.println(currentPacket);
}

void handleCommand(String s){
  s.trim();
  s.replace(" ","");
  if(!s.length()) return;
  if(s=="start"){botEnabled=true;integral=0;lastError=0;currentState="Running";updatePacket(-1,"STARTED");}
  else if(s=="stop"){botEnabled=false;stopMotors();currentState="Stopped";updatePacket(-1,"STOPPED");}
  else if(s=="cal"){calibrateSensors();updatePacket(-1,"CAL_DONE");}
  else if(s.startsWith("Kp=")) Kp=s.substring(3).toFloat();
  else if(s.startsWith("Ki=")) Ki=s.substring(3).toFloat();
  else if(s.startsWith("Kd=")) Kd=s.substring(3).toFloat();
  else if(s.startsWith("base=")) baseSpeed=s.substring(5).toInt();
  else if(s.startsWith("turn=")) turnSpeed=s.substring(5).toInt();
  else if(s.startsWith("max=")) maxSpeed=s.substring(4).toInt();
  currentState="Ready";
}

void handleCmd(){
  if(!server.hasArg("value")){server.send(400,"text/plain","missing value"); return;}
  handleCommand(server.arg("value"));
  server.send(200,"text/plain","OK");
}

void handleStatus(){ server.send(200,"application/json", String("{\"state\":\"")+currentState+"\",\"botEnabled\":"+(botEnabled?"true":"false")+"}"); }
void handlePacket(){ server.send(200,"text/plain",currentPacket); }

void setup(){
  Serial.begin(115200);
  pinMode(STATUS_LED,OUTPUT);
  pinMode(AIN1,OUTPUT); pinMode(AIN2,OUTPUT); pinMode(BIN1,OUTPUT); pinMode(BIN2,OUTPUT); pinMode(STBY,OUTPUT);
  digitalWrite(STBY,HIGH);
  ledcAttach(PWMA,PWM_FREQ,PWM_RES);
  ledcAttach(PWMB,PWM_FREQ,PWM_RES);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID,AP_PASS);

  calibrateSensors();
  server.on("/cmd", handleCmd);
  server.on("/status", handleStatus);
  server.on("/packet", handlePacket);
  server.begin();
  currentState="Ready";
  updatePacket(-1,"READY");
}

void loop(){
  server.handleClient();

  if(!botEnabled){
    stopMotors();
    delay(5);
    return;
  }

  int position=readLinePosition();
  const char* state="FOLLOW";

  if(position==-1){
    state="LOST";
    setMotors(50,50);
  } else if(position<3000){
    state="LEFT";
    setMotors(baseSpeed-turnSpeed, baseSpeed+turnSpeed);
  } else if(position>4000){
    state="RIGHT";
    setMotors(baseSpeed+turnSpeed, baseSpeed-turnSpeed);
  } else {
    float error=position-setPoint;
    integral += error;
    integral = constrain(integral,-2500,2500);
    float derivative=error-lastError;
    lastError=error;
    float correction=Kp*error+Ki*integral+Kd*derivative;
    int leftMotor=constrain((int)(baseSpeed+correction),0,maxSpeed);
    int rightMotor=constrain((int)(baseSpeed-correction),0,maxSpeed);
    setMotors(leftMotor,rightMotor);
  }

  updatePacket(position,state);
  delay(10);
}