#include <Arduino.h>
#include <Keypad.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>

#define TEMP_PIN 2
#define HEATER_PIN 15
#define SERVO_PIN 23
#define MAX_TEMP 125 //Temp above which OVERHEATING will be considered

enum TempSystemState {
  STATE_IDLE,
  STATE_HEATING,
  STATE_STABILIZING,
  STATE_TARGET_REACHED,
  STATE_OVERHEAT
};

TempSystemState currState = STATE_IDLE;
TempSystemState lastState = STATE_IDLE;
unsigned long stateStartTime = 0;
const int STABILIZE_TIME_MS = 5000; // Stabilize Time
const float TEMP_TOLERANCE = 4.0;   //Temp Tolerance 

unsigned long tempLastChecked = 0;
unsigned long lastDisplayUpdate = 0;
#define TEMP_CHECK_MS 1000  // 1 second for checking Temp
#define DISPLAY_UPDATE_MS 2000  // 2 seconds for displaying stats

OneWire tempBus(TEMP_PIN);
DallasTemperature sensor(&tempBus);
Servo myServo;

//Initializing Values
float currTemp = 0.0;
int targetTemp = 0;
bool heaterRunning = false;

const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};

Keypad numpad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String tempInput = "";
bool inputting = false;

void setState(TempSystemState newState);
void processKeypad();
void readTemperature();
void controlHeater();
void updateSystemState();
void displayStatus();
bool isAtTarget();




void processKeypad() {
  char key = numpad.getKey();
  
  if (!key) return; 
  
  Serial.print("Key: ");
  Serial.println(key);
  
  if (key == 'A') {
    inputting = true;
    tempInput = "";
    Serial.println("Enter temp:");
  } 
  else if (key == 'C') {
    heaterRunning = false;
    targetTemp = 0;
    digitalWrite(HEATER_PIN, LOW);
    myServo.write(0);
    Serial.println("Heater OFF");
    inputting = false;
    setState(STATE_IDLE);  
  } 
  else if (key == '#' && inputting) {
    if (tempInput.length() < 2 || tempInput.length() > 3) {
      Serial.println("Invalid! Need 2-3 digits");
      tempInput = "";
      return;
    }
    
    int temp = tempInput.toInt();
    if (temp <= 0 || temp > MAX_TEMP) {
      Serial.println("Out of range! (1-" + String(MAX_TEMP) + ")");
      tempInput = "";
      return;
    }
    
    targetTemp = temp;
    heaterRunning = true;
    inputting = false;
    Serial.println("Set to: " + String(targetTemp) + "C");
    
    if (currTemp < targetTemp - TEMP_TOLERANCE) {
      setState(STATE_HEATING);
    } else if (isAtTarget()) {
      setState(STATE_STABILIZING);
    }
  } 
  else if (inputting && isDigit(key)) {
    if (tempInput.length() < 3) {
      tempInput += key;
      Serial.println("Input: " + tempInput);
    }
  }
}

void readTemperature() {
  sensor.requestTemperatures();
  float reading = sensor.getTempCByIndex(0);
  
  if (reading == DEVICE_DISCONNECTED_C) {
    Serial.println("Sensor error!");
    return;
  }
  
  currTemp = reading;
}

void controlHeater() {
  if (currState == STATE_OVERHEAT) {
    digitalWrite(HEATER_PIN, LOW);
    myServo.write(0);
    return;
  }
  
  if (!heaterRunning || targetTemp <= 0) {
    digitalWrite(HEATER_PIN, LOW);
    myServo.write(0);
    return;
  }
  
  if (currState == STATE_HEATING) {
    digitalWrite(HEATER_PIN, HIGH);
    myServo.write(targetTemp);
  } 
  else if (currState == STATE_STABILIZING) {
    if (currTemp < targetTemp) {
      digitalWrite(HEATER_PIN, HIGH);
      myServo.write(targetTemp);
    } else {
      digitalWrite(HEATER_PIN, LOW);
    }
  }
  else if (currState == STATE_TARGET_REACHED) {
    if (currTemp < targetTemp - 0.5) {
      digitalWrite(HEATER_PIN, HIGH);
    } else if (currTemp >= targetTemp) {
      digitalWrite(HEATER_PIN, LOW);
    }
    myServo.write(targetTemp);
  }
  else {
    digitalWrite(HEATER_PIN, LOW);
    myServo.write(0);
  }
}

bool isAtTarget() {
  return (currTemp >= targetTemp - TEMP_TOLERANCE && 
          currTemp <= targetTemp + TEMP_TOLERANCE);
}

void setState(TempSystemState newState) {
  if (newState == currState) return; 
  
  lastState = currState;
  currState = newState;
  stateStartTime = millis();
  
  Serial.print("State: ");
  
  switch(newState) {
    case STATE_IDLE: Serial.println("IDLE"); break;
    case STATE_HEATING: Serial.println("HEATING"); break;
    case STATE_STABILIZING: Serial.println("STABILIZING"); break;
    case STATE_TARGET_REACHED: Serial.println("TARGET REACHED"); break;
    case STATE_OVERHEAT: Serial.println("OVERHEAT!"); break;
  }
}

void updateSystemState() {
  unsigned long stateTime = millis() - stateStartTime;
  
  if (currTemp >= MAX_TEMP) {
    setState(STATE_OVERHEAT);
    return;
  }
  
  if (!heaterRunning || targetTemp <= 0) {
    setState(STATE_IDLE);
    return;
  }
  
  if (currTemp > targetTemp + TEMP_TOLERANCE){
    setState(STATE_IDLE);
  }

  switch (currState) {
    case STATE_IDLE:
      if (currTemp < targetTemp - TEMP_TOLERANCE) {
        setState(STATE_HEATING);
      }
      else if (isAtTarget()) {
        setState(STATE_TARGET_REACHED);
      }
      break;
      
    case STATE_HEATING:
      if (isAtTarget()) {
        setState(STATE_STABILIZING);
      }
      break;
      
    case STATE_STABILIZING:
      if (stateTime >= STABILIZE_TIME_MS && isAtTarget()) {
        setState(STATE_TARGET_REACHED);
      }
      else if (currTemp < targetTemp - TEMP_TOLERANCE) {
        setState(STATE_HEATING);
      }
      break;
      
    case STATE_TARGET_REACHED:
      if (currTemp < targetTemp - TEMP_TOLERANCE) {
        setState(STATE_HEATING);
      }
      break;
      
    case STATE_OVERHEAT:
      if (currTemp < MAX_TEMP - 5) {
        if (isAtTarget()) {
          setState(STATE_TARGET_REACHED);
        } else if (currTemp < targetTemp - TEMP_TOLERANCE) {
          setState(STATE_HEATING);
        } else {
          setState(STATE_IDLE);
        }
      }
      break;
  }
}

void displayStatus() {
  if (heaterRunning) {
    Serial.print("Temp: ");
    Serial.print(currTemp, 1);
    Serial.print("C | Target: ");
    Serial.print(targetTemp);
    Serial.print("C | Heater: ");
    Serial.print(digitalRead(HEATER_PIN) ? "ON" : "OFF");
    Serial.print(" | Servo: ");
    Serial.print(targetTemp);
    Serial.print("° | State: ");
    
    switch(currState) {
      case STATE_IDLE: Serial.print("IDLE"); break;
      case STATE_HEATING: Serial.print("HEATING"); break;
      case STATE_STABILIZING: Serial.print("STABILIZING"); break;
      case STATE_TARGET_REACHED: Serial.print("TARGET REACHED"); break;
      case STATE_OVERHEAT: Serial.print("OVERHEAT!"); break;
    }
    
    Serial.println();
  } else {
    Serial.print("Temp: ");
    Serial.print(currTemp, 1);
    Serial.print("C | Heater: OFF | State: ");
    
    switch(currState) {
      case STATE_IDLE: Serial.print("IDLE"); break;
      case STATE_HEATING: Serial.print("HEATING"); break;
      case STATE_STABILIZING: Serial.print("STABILIZING"); break;
      case STATE_TARGET_REACHED: Serial.print("TARGET REACHED"); break;
      case STATE_OVERHEAT: Serial.print("OVERHEAT!"); break;
    }
    
    Serial.println();
  }
}

void setup() {
  pinMode(TEMP_PIN, INPUT);
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW); 
  
  Serial.begin(115200);
  while(!Serial); 
  
  sensor.begin();
  myServo.attach(SERVO_PIN);
  myServo.write(0); 
  
  sensor.requestTemperatures();
  Serial.println("Sensors Initialized !!");
  Serial.println("Configuring BLE !!");

  Serial.println("Enter Temperature : Press 'A'; Set Temperature : Press '#'; Shut Off Heater : Press 'C'. ");
  
  setState(STATE_IDLE);
}

void loop() {
  unsigned long now = millis();
  
  processKeypad();
  
  if (now - tempLastChecked >= TEMP_CHECK_MS) {
    tempLastChecked = now;
    readTemperature();
    controlHeater();
    updateSystemState();
  }
  
  if (now - lastDisplayUpdate >= DISPLAY_UPDATE_MS && !inputting) {
    lastDisplayUpdate = now;
    displayStatus();
  }
 
}
