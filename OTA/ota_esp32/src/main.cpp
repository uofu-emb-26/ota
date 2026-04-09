#include <Arduino.h>

#define LED_DELAY 250U

// put function declarations here:
int myFunction(int, int);
void togglePin(int pin);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  Serial.begin(115200);
  delay(500); // Wait for serial to initialize
  Serial.println("Hello, World!");
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  togglePin(LED_BUILTIN);
  delay(LED_DELAY);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}

//toggle pin
void togglePin(int pin) {
  digitalWrite(pin, !digitalRead(pin));
}