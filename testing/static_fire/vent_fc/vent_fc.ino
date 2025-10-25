#include "w5500.h"

// timing
const unsigned long OPEN_MILLIS = 500;

// bit‑masks for each valve in the 7‑bit rocket state
const uint8_t GN2_LNG_FLOW_MASK = 0b1000000;  // new “arm_lng”
const uint8_t GN2_LOX_FLOW_MASK = 0b0100000;  // new “arm_lox”
const uint8_t GN2_VENT_MASK     = 0b0010000;
const uint8_t LNG_FLOW_MASK     = 0b0001000;
const uint8_t LNG_VENT_MASK     = 0b0000100;
const uint8_t LOX_FLOW_MASK     = 0b0000010;
const uint8_t LOX_VENT_MASK     = 0b0000001;

// assign each valve its GPIO pin; feel free to adjust as you wire it up
const uint8_t GN2_LNG_FLOW_PIN = 4;
const uint8_t GN2_LOX_FLOW_PIN = 9;
const uint8_t GN2_VENT_PIN     = 6; //confirmed
const uint8_t LNG_FLOW_PIN     = 7;
const uint8_t LNG_VENT_PIN     = 3; //confirmed
const uint8_t LOX_FLOW_PIN     = 8;
const uint8_t LOX_VENT_PIN     = 5; //confirmed

// first byte 0x02 = locally‑administered, unicast
const uint8_t MAC_GROUND_STATION[6] = { 0x02, 0x47, 0x53, 0x00, 0x00, 0x01 }; // GS:  ground station
const uint8_t MAC_RELIEF_VALVE[6]   = { 0x02, 0x52, 0x56, 0x00, 0x00, 0x02 }; // RV:  relief valve
const uint8_t MAC_FLOW_VALVE[6]     = { 0x02, 0x46, 0x4C, 0x00, 0x00, 0x03 }; // FL:  flow valve
const uint8_t MAC_SENSOR_GIGA[6]    = { 0x02, 0x53, 0x49, 0x00, 0x00, 0x04 }; // SI:  sensor interface

// a single struct covers all valves
struct Valve {
  const char* name;
  uint8_t     mask;
  uint8_t     pin;
  bool        state;
  unsigned long lastOpened;
};

// list all 7 valves here
Valve valves[] = {
  { "GN2_LNG_FLOW", GN2_LNG_FLOW_MASK, GN2_LNG_FLOW_PIN, false, 0 },
  { "GN2_LOX_FLOW", GN2_LOX_FLOW_MASK, GN2_LOX_FLOW_PIN, false, 0 },
  { "GN2_VENT",     GN2_VENT_MASK,     GN2_VENT_PIN,     false, 0 },
  { "LNG_FLOW",     LNG_FLOW_MASK,     LNG_FLOW_PIN,     false, 0 },
  { "LNG_VENT",     LNG_VENT_MASK,     LNG_VENT_PIN,     false, 0 },
  { "LOX_FLOW",     LOX_FLOW_MASK,     LOX_FLOW_PIN,     false, 0 },
  { "LOX_VENT",     LOX_VENT_MASK,     LOX_VENT_PIN,     false, 0 }
};
const size_t NUM_VALVES = sizeof(valves)/sizeof(valves[0]);

Wiznet5500 w5500;
uint8_t buffer[1000];
uint8_t rocketState = 0b0000000; // default null

//——— simple hex printer for MAC debug ———
void printPaddedHex(uint8_t byte) {
  char str[2] = { (char)((byte>>4)&0x0f), (char)(byte&0x0f) };
  for(int i=0;i<2;i++){
    if(str[i]>9) str[i]+=39;
    str[i] += 48;
    Serial.print(str[i]);
  }
}
void printMACAddress(const uint8_t addr[6]) {
  for(int i=0;i<6;i++){
    printPaddedHex(addr[i]);
    if(i<5) Serial.print(':');
  }
  Serial.println();
}

void setup() {
  // serial + SPI for W5500
  Serial.begin(9600);
  SPI.begin();
  pinMode(10, OUTPUT); digitalWrite(10, HIGH);
  Serial.println("[W5500MacRaw]");
  printMACAddress(MAC_RELIEF_VALVE);
  w5500.begin(MAC_RELIEF_VALVE);

  // init all valve pins
  for(size_t i=0;i<NUM_VALVES;i++){
    pinMode(valves[i].pin, OUTPUT);
    digitalWrite(valves[i].pin, LOW);
    valves[i].state = false;
    valves[i].lastOpened = 0;
  }
}

void loop() {
  receiveRocketState();
  updateValveStates();
  applyValveVoltages();
  delay(50);
}

void receiveRocketState() {
  uint16_t len = w5500.readFrame(buffer, sizeof(buffer));
  if(len > 0) {
    if (buffer[12] != 0x63 || buffer[13] != 0xe4) return;
    uint8_t newState = buffer[14];
    if(newState != rocketState) {
      rocketState = newState;
      Serial.println("----------");
      Serial.print("Received Rocket State: ");
      Serial.println(rocketState, BIN);
    }
  }
}

void updateValveStates() {
  // for each valve, compare mask bit → open/close transition
  for(size_t i=0;i<NUM_VALVES;i++) {
    bool shouldBeOpen = rocketState & valves[i].mask;
    if(shouldBeOpen && !valves[i].state) {
      Serial.print(valves[i].name);
      Serial.println(": OPENING");
      valves[i].state = true;
      valves[i].lastOpened = millis();
    }
    else if(!shouldBeOpen && valves[i].state) {
      Serial.print(valves[i].name);
      Serial.println(": CLOSING");
      valves[i].state = false;
    }
  }
}

void applyValveVoltages() {
  // full PWM for OPEN_MILLIS, then trickle
  for(size_t i=0;i<NUM_VALVES;i++) {
    if(valves[i].state) {
      unsigned long elapsed = millis() - valves[i].lastOpened;
      uint8_t pwm = (elapsed > OPEN_MILLIS) ? 77 : 255;
      analogWrite(valves[i].pin, pwm);
    } else {
      analogWrite(valves[i].pin, 0);
    }
  }
}
