/*****************************************************
 * Name: I2C_Address
 * Function: Read the address of the I2C LCD1602
 * Connection:
 * I2C                 Arduino UNO 
 * GND                 GND
 * VCC                 5V
 * SDA                 A4 (pin 20 in Mega2560)
 * SCL                 A5 (pin 21 in Mega2560)
 ********************************************************/

#include <Wire.h>  // Include Wire library for I2C communication

void setup() {
  Wire.begin();                     // Initialize I2C communication
  Serial.begin(9600);               // Start serial communication at 9600 baud rate
  Serial.println("\nI2C Scanner");  // Print a message to the serial monitor
}

void loop() {
  byte error, address;  // Declare variables for storing error status and I2C address
  int nDevices;         // Variable to keep track of number of devices found

  Serial.println("Scanning...");  // Print scanning message
  nDevices = 0;                   // Initialize the device count to 0

  // Loop through all possible I2C addresses (1 to 126)
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);  // Start a transmission to the I2C address
    error = Wire.endTransmission();   // End the transmission and get the status

    // Check if device responded without error (acknowledged)
    if (error == 0) {
      Serial.print("I2C device found at address 0x");  // Notify device found
      if (address < 16) Serial.print("0");             // Print leading zero for addresses less than 16
      Serial.print(address, HEX);                      // Print the address in hexadecimal
      Serial.println(" !");
      nDevices++;                                   // Increment the device count
    } else if (error == 4) {                        // If there was an unknown error
      Serial.print("Unknown error at address 0x");  // Notify about the error
      if (address < 16) Serial.print("0");          // Print leading zero for addresses less than 16
      Serial.println(address, HEX);                 // Print the address in hexadecimal
    }
  }

  // After scanning, print the results
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");  // No devices found
  else
    Serial.println("done\n");  // Scanning done

  delay(5000);  // Wait 5 seconds before the next scan
}
