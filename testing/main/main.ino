//LIBRARIES*****************************************************
//SPI Libs
#include <SPI.h>


//Ethernet Lib
#include "w5500.h"
// #include <SD.h>
// File myFile;

//Load Cell Lib
#include "HX711.h"
//Thermocouple Libs
#include <Wire.h>
#include <DallasTemperature.h>


#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include "Adafruit_MCP9600.h"
#include <OneWire.h>

//Pin Definitions **********************************************
//Ethernet CS
#define EthernetCS 10
#define SDCS 40

//Load Cell Pins
#define DOUT1  30
#define CLK1  28
#define DOUT2  34
#define CLK2  32
#define DOUT3  38
#define CLK3  36
//ThermoCouple OneWire
#define ONE_WIRE_BUS 22
#define I2C_ADDRESS (0x67)

// ─────────────────────────────────────────────────────────────────────────────
//                    Ethernet (MAC‑RAW) & Rocket Setup
// ─────────────────────────────────────────────────────────────────────────────
// timing for solenoids
const unsigned long OPEN_MILLIS = 500;

// bit‑masks for each valve in the 7‑bit rocket state
const uint8_t GN2_LNG_FLOW_MASK = 0b1000000; // new “arm_lng”
const uint8_t GN2_LOX_FLOW_MASK = 0b0100000; // new “arm_lox”
const uint8_t GN2_VENT_MASK = 0b0010000;
const uint8_t LNG_FLOW_MASK = 0b0001000;
const uint8_t LNG_VENT_MASK = 0b0000100;
const uint8_t LOX_FLOW_MASK = 0b0000010;
const uint8_t LOX_VENT_MASK = 0b0000001;

// PWM pins for solenoids
const uint8_t GN2_LNG_FLOW_PIN = 8; // confirmed 
const uint8_t GN2_LOX_FLOW_PIN = 9; // confirmed
const uint8_t GN2_VENT_PIN = 4;
const uint8_t LNG_FLOW_PIN = 7; // confirmed 
const uint8_t LNG_VENT_PIN = 3;
const uint8_t LOX_FLOW_PIN = 10; // confirmed 
const uint8_t LOX_VENT_PIN = 6;

// first byte 0x02 = locally‑administered, unicast
const uint8_t MAC_GROUND_STATION[6] = {0x02, 0x47, 0x53,
                                       0x00, 0x00, 0x01}; // GS:  ground station
const uint8_t MAC_RELIEF_VALVE[6] = {0x02, 0x52, 0x56,
                                     0x00, 0x00, 0x02}; // RV:  relief valve
const uint8_t MAC_FLOW_VALVE[6] = {0x02, 0x46, 0x4C,
                                   0x00, 0x00, 0x03}; // FL:  flow valve
const uint8_t MAC_SENSOR_GIGA[6] = {0x02, 0x53, 0x49,
                                    0x00, 0x00, 0x04}; // SI:  sensor interface

// a single struct covers all valves
struct Valve {
  const char *name;
  uint8_t mask;
  uint8_t pin;
  bool state;
  unsigned long lastOpened;
};

// list all 7 valves here, whether or not they ar used
Valve valves[] = {
    {"GN2_LNG_FLOW", GN2_LNG_FLOW_MASK, GN2_LNG_FLOW_PIN, false, 0},
    {"GN2_LOX_FLOW", GN2_LOX_FLOW_MASK, GN2_LOX_FLOW_PIN, false, 0},
    {"GN2_VENT", GN2_VENT_MASK, GN2_VENT_PIN, false, 0},
    {"LNG_FLOW", LNG_FLOW_MASK, LNG_FLOW_PIN, false, 0},
    {"LNG_VENT", LNG_VENT_MASK, LNG_VENT_PIN, false, 0},
    {"LOX_FLOW", LOX_FLOW_MASK, LOX_FLOW_PIN, false, 0},
    {"LOX_VENT", LOX_VENT_MASK, LOX_VENT_PIN, false, 0}};
const size_t NUM_VALVES = sizeof(valves) / sizeof(valves[0]);

uint8_t buffer[500];
uint8_t rocketState = 0b0000000; // default null

struct TelemetryFrame {
  // –––– Ethernet header (14 B) ––––
  uint8_t dstMac[6];
  uint8_t srcMac[6];
  uint16_t ethType; // always 0x8889

  // –––– Payload ––––
  uint8_t payload[200]; // 200 bytes usually enough.. optimize later
};
TelemetryFrame f;

//object creation **********************************************
//Ethernet
Wiznet5500 w5500;
//Load Cells
HX711 loadCell1;
HX711 loadCell2;
HX711 loadCell3;
//Thermocouples
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature thermoCouples(&oneWire);
DeviceAddress addr;
//Adafruit_MCP9600 mcp;

//Other Consts ***************************************************
float calibration_factor1 = -7050; //-7050 worked for my 440lb max scale setup
float calibration_factor2 = -7050; //-7050 worked for my 440lb max scale setup
float calibration_factor3 = -7050; //-7050 worked for my 440lb max scale setup

Ambient_Resolution ambientRes = RES_ZERO_POINT_0625;

void setup() {
  Serial.begin(9600);

  // if (!SD.begin(SDCS)) {
  //   Serial.println("initialization failed!");
  //   //while (1);
  // }else{
  // Serial.println("SD Initlaized");
  // }

  //PT INPUTS
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  pinMode(A2,INPUT);
  pinMode(A3,INPUT);
  pinMode(A4,INPUT);

  
  //ethernet setup
  w5500.begin(MAC_SENSOR_GIGA);
  Serial.println("Ethernet Setup Complete");
  //load cell setup
  // loadCell1.begin(DOUT1, CLK1);
  // loadCell1.set_scale();
  // loadCell1.tare(); //Reset the scale to 0
  // Serial.println("Load 1 Done");
  
  // loadCell2.begin(DOUT2, CLK2);
  // loadCell2.set_scale();
  // loadCell2.tare(); //Reset the scale to 0
  // Serial.println("Load 2 Done");

  // loadCell3.begin(DOUT3, CLK3);
  // loadCell3.set_scale();
  // loadCell3.tare(); //Reset the scale to 0
  // Serial.println("Load 3 Done");

  //thermocouple setup
  
  // thermoCouples.begin();
/*
  if (! mcp.begin(I2C_ADDRESS)) {
    Serial.println("Sensor not found. Check wiring!");
    while (1);
  }
  

  //mcp.setAmbientResolution(ambientRes);
  */

  Serial.println("Done Initialized");
}

void receiveRocketState() {
  uint16_t len = w5500.readFrame(buffer, sizeof(buffer));
  if (len > 0) {
    if (buffer[12] != 0x63 || buffer[13] != 0xe4)
      return;
    uint8_t newState = buffer[14];
    if (newState != rocketState) {
      rocketState = newState;
      Serial.println("----------");
      Serial.print("Received Rocket State: ");
      Serial.println(rocketState, BIN);
    }
  }
}

void updateValveStates() {
  // for each valve, compare mask bit → open/close transition
  for (size_t i = 0; i < NUM_VALVES; i++) {
    bool shouldBeOpen = rocketState & valves[i].mask;
    if (shouldBeOpen && !valves[i].state) {
      Serial.print(valves[i].name);
      Serial.println(": OPENING");
      valves[i].state = true;
      valves[i].lastOpened = millis();
    } else if (!shouldBeOpen && valves[i].state) {
      Serial.print(valves[i].name);
      Serial.println(": CLOSING");
      valves[i].state = false;
    }
  }
}

void applyValveVoltages() {
  // full PWM for OPEN_MILLIS, then trickle
  for (size_t i = 0; i < NUM_VALVES; i++) {
    if (valves[i].state) {
      unsigned long elapsed = millis() - valves[i].lastOpened;
      uint8_t pwm = (elapsed > OPEN_MILLIS) ? 77 : 255;
      analogWrite(valves[i].pin, pwm);
      // Serial.print("Writing ");
      // Serial.print(pwm);
      // Serial.print(" to ");
      // Serial.println(valves[i].pin);
    } else {
      analogWrite(valves[i].pin, 0);
    }
  }
}

void sendSensorData() {
  double PT1_a = analogRead(A0) / 1023.0 * 5.0;
  double PT1 = ((PT1_a - 0.5) / (4.5 - 0.5)) * 5000;
  double PT2_a = analogRead(A1) / 1023.0 * 5.0;
  double PT2 = ((PT2_a - 1) / (5.0 - 1.0)) * 1000;
  double PT3_a = analogRead(A2) / 1023.0 * 5.0;
  double PT3 = ((PT3_a - 1) / (5.0 - 1.0)) * 1000;
  double PT4_a = analogRead(A3) / 1023.0 * 5.0;
  double PT4 = ((PT4_a - 1) / (5.0 - 1.0)) * 1000;
  double PT5_a = analogRead(A4) / 1023.0 * 5.0;
  double PT5 = ((PT5_a - 1) / (5.0 - 1.0)) * 1000;
  

  // LOAD CELL CALIBRATION
  // loadCell1.set_scale(calibration_factor1); // Adjust to this calibration factor
  // loadCell2.set_scale(calibration_factor2); // Adjust to this calibration factor
  // loadCell3.set_scale(calibration_factor3); // Adjust to this calibration factor

  // LOAD CELL DATA
  // double loadOutput1 = loadCell1.get_units();
  // double loadOutput2 = loadCell2.get_units();
  // double loadOutput3 = loadCell3.get_units();
  double loadOutput1 = 0;
  double loadOutput2 = 0;
  double loadOutput3 = 0;

  // THERMOCOUPLE DATA

  // thermoCouples.requestTemperatures();
  // double thermoCouple1 = thermoCouples.getTempCByIndex(0);
  // double thermoCouple2 = thermoCouples.getTempCByIndex(1);
  double thermoCouple1 = 0;
  double thermoCouple2 = 0;
  // double thermoCouple3=thermoCouples.getTempCByIndex(2);

  // LIVE ETHERNET TRANSMISSION
  // Put all data into an output string to be sent over
  // String
  // dataOut=String(PT1)+","+String(PT2)+","+String(PT3)+","+String(PT4)+","+String(PT5);
  // String
  // dataOut=String(loadOutput1)+","+String(loadOutput2)+","+String(loadOutput3)+","+String(thermoCouple1)+","+String(thermoCouple2);

  
  String dataOut = String(PT1) + "," + String(PT2) + "," + String(PT3) + "," +
                   String(PT4) + "," + String(PT5) + "," + String(loadOutput1) +
                   "," + String(loadOutput2) + "," + String(loadOutput3) + "," +
                   String(thermoCouple1) + "," + String(thermoCouple2);
                   
  // String dataOut = String(PT1_a) + ","
  //                + String(PT2_a) + ","
  //                + String(PT3_a) + ","
  //                + String(PT4_a) + ","
  //                + String(PT5_a) + ",";
  Serial.println(dataOut);

  // Send data

  char *data = dataOut.c_str();
  int len = strlen(data);
  memset(&f, 0, sizeof(f)); // set to 0
  memcpy(f.dstMac, MAC_GROUND_STATION, 6);
  // ((byte *)&f)[0] = 0xFF;
  // ((byte *)&f)[1] = 0xFF;
  // ((byte *)&f)[2] = 0xFF;
  // ((byte *)&f)[3] = 0xFF;
  // ((byte *)&f)[4] = 0xFF;
  // ((byte *)&f)[5] = 0xFF;
  memcpy(f.srcMac, MAC_SENSOR_GIGA, 6);
  ((byte *)&f)[12] = 0x88;
  ((byte *)&f)[13] = 0x89;
  // Serial.print("Data len: ");
  // Serial.println(len);
  memcpy(f.payload, data, len);
  if (w5500.sendFrame((byte *)&f, sizeof(f)) <= 0) {
    Serial.println("Ethernet send Failed");
  }
  
  // myFile = SD.open("StaticFire.txt", FILE_WRITE);
  // if (myFile) {
  //   Serial.print("Writing to file...");
  //   myFile.println(dataOut);
  //   // close the file:
  //   myFile.close();
  //   Serial.println("done.");
  // } else {
  //   // if the file didn't open, print an error:
  //   Serial.println("error opening file");
  // }
}

void loop() {
  receiveRocketState();
  // Serial.println("Received");
  updateValveStates();
  // Serial.println("Updated");
  applyValveVoltages();
  // Serial.println("Applied");
  sendSensorData();
  delay(100);
}