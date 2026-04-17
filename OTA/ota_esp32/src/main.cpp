#include <Arduino.h>
#include <iterator>
#include <Serial.h>
#define LED_DELAY 500U
// put function declarations here:
uint32_t write(const char *buffer, uint8_t size);
void setup();
void loop();
void flush(bool txOnly);
bool setClockSource(SerialClkSrc clkSrc);
bool setMode(SerialMode mode);

// using GPIO1 (TX0) and GPIO3 (RX0) for ESP32 -> STM connection
void setup() {
  //set the UART clock source
  Serial1.setClockSource(UART_CLK_SRC_DEFAULT);

  //set the UART operating mode
  Serial1.setMode(UART_MODE_UART);
  
  // Initializes the Serial port with the specified baud rate and default configuration.
  Serial1.begin(115200)

  const char *buffer = "Hello world";
  uint8_t size = sizeof(buffer);
  uint32_t bytes_written = Serial1.write(buffer, size);

  /* Waits for all data in the TX buffer to be transmitted. */
  flush(true);
}

void loop() {
  
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}
