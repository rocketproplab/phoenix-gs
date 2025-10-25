//LIBRARIES*****************************************************
//SPI Libs
#include <SPI.h>


//Ethernet Lib
#include "w5500.h"
#include <SD.h>
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
#define EthernetCS 42
#define SDCS 40

//Load Cell Pins
#define DOUT1  30
#define CLK1  28
#define DOUT2  34
#define CLK2  32
#define DOUT3  39
#define CLK3  36
//ThermoCouple OneWire
#define ONE_WIRE_BUS 22
#define I2C_ADDRESS (0x67)

// ─────────────────────────────────────────────────────────────────────────────
//                           Ethernet (MAC‑RAW) Setup
// ─────────────────────────────────────────────────────────────────────────────
// first byte 0x02 = locally‑administered, unicast
const uint8_t MAC_GROUND_STATION[6] = {0x02, 0x47, 0x53,
                                       0x00, 0x00, 0x01}; // GS:  ground station
const uint8_t MAC_RELIEF_VALVE[6] = {0x02, 0x52, 0x56,
                                     0x00, 0x00, 0x02}; // RV:  relief valve
const uint8_t MAC_FLOW_VALVE[6] = {0x02, 0x46, 0x4C,
                                   0x00, 0x00, 0x03}; // FL:  flow valve
const uint8_t MAC_SENSOR_GIGA[6] = {0x02, 0x53, 0x49,
                                    0x00, 0x00, 0x04}; // SI:  sensor interface

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
Adafruit_MCP9600 mcp;

//Other Consts ***************************************************
float calibration_factor1 = -7050; //-7050 worked for my 440lb max scale setup
float calibration_factor2 = -7050; //-7050 worked for my 440lb max scale setup
float calibration_factor3 = -7050; //-7050 worked for my 440lb max scale setup

Ambient_Resolution ambientRes = RES_ZERO_POINT_0625;

void setup() {
  Serial.begin(9600);
   //ethernet setup
  w5500.begin(MAC_SENSOR_GIGA);
  //software SPI setup
  /*
  SPI.begin();

  if (!SD.begin(SDCS)) {
    Serial.println("initialization failed!");
   // while (1);
  }
  
 

  //load cell setup
  loadCell1.begin(DOUT1, CLK1);
  loadCell1.set_scale();
  loadCell1.tare(); //Reset the scale to 0
  
  loadCell2.begin(DOUT2, CLK2);
  loadCell2.set_scale();
  loadCell2.tare(); //Reset the scale to 0

  loadCell3.begin(DOUT3, CLK3);
  loadCell3.set_scale();
  loadCell3.tare(); //Reset the scale to 0

  //thermocouple setup
  thermoCouples.begin();

  if (! mcp.begin(I2C_ADDRESS)) {
    Serial.println("Sensor not found. Check wiring!");
    //while (1);
  }

  mcp.setAmbientResolution(ambientRes);
  */
  Serial.print("setup complete");
}

void loop() {

  
  int PT1=analogRead(A0);
  int PT2=analogRead(A1);
  int PT3=analogRead(A2);
  int PT4=analogRead(A3);
  int PT5=analogRead(A4);

  
  
/*
  //LOAD CELL CALIBRATION
  loadCell1.set_scale(calibration_factor1); //Adjust to this calibration factor
  loadCell2.set_scale(calibration_factor2); //Adjust to this calibration factor
  loadCell3.set_scale(calibration_factor3); //Adjust to this calibration factor

  //LOAD CELL DATA
  double loadOutput1=loadCell1.get_units();
  double loadOutput2=loadCell1.get_units();
  double loadOutput3=loadCell1.get_units();
  
  //THERMOCOUPLE DATA
  thermoCouples.requestTemperatures();
  double thermoCouple1=thermoCouples.getTempCByIndex(0);
  double thermoCouple2=thermoCouples.getTempCByIndex(1);
 

  */
  //LIVE ETHERNET TRANSMISSION
  //Put all data into an output string to be sent over
  String dataOut=String(PT1)+","+String(PT2)+","+String(PT3)+","+String(PT4)+","+String(PT5);
  Serial.println(dataOut);
 // String dataOut=String(loadOutput1)+","+String(loadOutput2)+","+String(loadOutput3)+","+String(thermoCouple1)+","+String(thermoCouple2);
  //Serial.println(dataOut);
  byte pkt[dataOut.length() + 14];
  memcpy(pkt,     MAC_GROUND_STATION,   6);        // destination
  memcpy(pkt + 6, MAC_SENSOR_GIGA, 6);        // source
  pkt[12] = 0x88;
  pkt[13] = 0x86;

  dataOut.getBytes(pkt + 14, dataOut.length());

  //Send data
  if(w5500.sendFrame(pkt, sizeof(pkt)) < 0){
    Serial.println("Failed"); 
  }
     else{
       Serial.println("Success");  
     }
     delay(50);

}