// https://docs.arduino.cc/learn/communication/uart/#transmit--receive-messages

/* 
Some notes currently:

Can send sentences while ESP32 is plugged into the computer with USB to USB mini chord, then ESP32 using
GPIO 16/17 for RX/TX, then pluggin pin 16 into PC4, and pin 17 into PC5, then connecting them with ground.

When reading from bprom in ESP32 this will move to using:
GPIO3 -> PC4
GPIO1 -> PC5

right now i'm not doing that because to use the Serial, this uses TX/RX0.
*/
#include <Arduino.h>
#include <stdint.h>

String sendMessage;

void setup(){
  
  Serial.begin(115200); //initialize serial communication at a 9600 baud rate
  
  /* void begin(unsigned long baud, uint32_t config = SERIAL_8N1, int8_t rxPin = -1, int8_t txPin = -1, 
  bool invert = false, unsigned long timeout_ms = 20000UL, 
  uint8_t rxfifo_full_thrhd = 120);*/

  Serial1.begin(115200, SERIAL_8N1, 16, 17);   // Initialize Serial1 for sending data
}

void loop(){
  // Serial.println("Hello World!");
  delay(100);
  if (Serial.available() > 0) {
    char inputChar = Serial.read();
    Serial.println(inputChar);
    if (inputChar == '|') {
      Serial.print("sending: ");
      Serial.println(sendMessage);
      Serial1.println(sendMessage);  // Send the message through Serial1 with a newline character
      sendMessage = "";  // Reset the message
    } else {
      sendMessage += inputChar;  // Append characters to the message
    }
  }
  Serial.println(sendMessage);
}

