// ESP32 OTA middleman
// Receives firmware binary over WiFi from a host PC and forwards it to STM32 over UART2
// HTTP endpoints: /fetch, /verify, /send_binary, /send, /flash
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <stdint.h>
#include <esp_rom_crc.h>

#define CRC_OFFSET  0xD0
#define LED_DELAY 250U
#define BINARY_MAX_SIZE 14000 // current size is 12708, could change later
#define SSID "gogol2"
#define PASS "bordello2"
#define LOCAL_IP_A "http://10.0.0.55:8080/OTA_app_a.bin"
#define LOCAL_IP_B "http://10.0.0.55:8080/OTA_app_b.bin"
#define VERSION_TXT "http://10.0.0.55:8080/version.txt"

// put function declarations here:
int myFunction(int, int);
void togglePin(int pin);
void handleFlash();
void handleSend();
void fetchBinary(const char* url, const char* filename);
void storeBinaryToLittleFS(uint8_t* data, int size, const char* filename);
void readBinaryFromLittleFS(uint8_t* buffer, int* size, const char* filename);
void handleVerify();
void handleSendBinary();
void checkForUpdate();
void newUpdateAvailable();
void handleCriticalFailure();
void enterSendLoop();
char waitForSTMResponse();
void handleSuccessfulUpdate();
void sendPartialBinary();


// WARNING: update these credentials before flashing in the define at the top of file
const char* ssid = SSID;
const char* password = PASS;

unsigned long lastCheck = 0;
const unsigned long CHECK_INTERVAL = 30000; // check every 30 seconds

int currentVersionA = 1;
int currentVersionB = 1;

// Global buffer for binary data
uint8_t binaryBuffer[BINARY_MAX_SIZE];

WebServer server(8080);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  delay(500);
  Serial.println("Hello, World!");
  pinMode(LED_BUILTIN, OUTPUT);

  if (!LittleFS.begin(true)) {
  Serial.println("LittleFS mount failed");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect!");
  } else {
    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  }

  server.on("/fetch", []() {
  fetchBinary(LOCAL_IP_A, "/firmware_a.bin");
  fetchBinary(LOCAL_IP_B, "/firmware_b.bin");
  server.send(200, "text/plain", "Binaries fetched and stored");
  });
  server.on("/verify", handleVerify);
  server.on("/flash", handleFlash);
  server.on("/send", handleSend);
  server.on("/send_binary", handleSendBinary);
  server.begin();

  //void loop is called here so it's acutally doing:
  /* 
  void loop() {
    server.handleClient();
  }*/
}

void loop() {
  server.handleClient();

  if (millis() - lastCheck > CHECK_INTERVAL) {
    lastCheck = millis();
    checkForUpdate();
    //newUpdateAvailable();
  }

}

void handleSuccessfulUpdate()
{
  Serial.print("Update sequence complete!");
  for (int i = 0; i < 5; i++) {
    handleFlash();
  }
  delay(60000);
}

void newUpdateAvailable()
{
  Serial.print("checking for update");
  checkForUpdate();
  Serial.print("telling stm ready for transmit");
  //send a 0 to the stm32
  Serial2.print(0);
  //wait for response
  char response = waitForSTMResponse();
  Serial.print("STMs response: ");
  Serial.print(response);
  /* STM32 wants to enter the send, receive, ack loop*/
  if (response == 0) 
  {
    enterSendLoop();
  }
      
      /* STM32 wants a 1-Minute Delay On This Path*/
  if (response == 1)
  {
    //delay for a minute
    delay(60000); //this is millisecond value
  }
      
  /* STM32 Sends there is a Critical Failure */
  if (response == 2) 
  {
    handleCriticalFailure();
  }
    
}

char waitForSTMResponse()
{
  while (!Serial2.available()) {};
  char response = Serial2.read();
  return response;
}

void handleCriticalFailure()
{
  return;
}

void enterSendLoop()
{
  sendPartialBinary();
}

//toggle pin
void togglePin(int pin) {
  digitalWrite(pin, !digitalRead(pin));
}

// HTTP handler: blinks the onboard LED 5 times
void handleFlash() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
  server.send(200, "text/plain", "LED flashed!");
}

// HTTP handler: sends a text message over UART2 with ?msg= parameter
void handleSend() {
  if (server.hasArg("msg")) {
    String message = server.arg("msg");
    Serial2.print(message);
    server.send(200, "text/plain", "Sent: " + message);
  } else {
    server.send(400, "text/plain", "Missing msg parameter");
  }
}

uint32_t crc32(uint8_t* image, uint32_t image_size, uint32_t field_offset) {
  uint32_t crc = 0xFFFFFFFFU;
  for (uint32_t i = 0U; i < image_size; i++) {
    uint8_t b = image[i];
    if (i >= field_offset && i < field_offset + 4U) {
      b = 0U;
    }
    crc ^= b;
    for (uint32_t j = 0U; j < 8U; j++) {
      if ((crc & 1U) != 0U) {
        crc = (crc >> 1) ^ 0xEDB88320U;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc ^ 0xFFFFFFFFU;
}


// Fetches binary file from HTTP server and stores it in LittleFS
// LittleFS is a flash based filesystem on the ESP32 that persists across power cycles
// You can access relevant sections by just using the name of the file, no need to deal with addresses
// currently the file name is firmware.bin, will likely change later...
void fetchBinary(const char* url, const char* filename) {
  HTTPClient http;
  // Update this URL when the host PC IP or filename changes
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    int contentLength = http.getSize();
    WiFiClient *stream = http.getStreamPtr();
    
    // Read binary data chunk by chunk
    int bytesRead = stream->readBytes(binaryBuffer, contentLength);

    Serial.println("Fetched " + String(bytesRead) + " bytes");
    storeBinaryToLittleFS(binaryBuffer, bytesRead, filename);

    
    // CRC Checks
    uint32_t storedCRC;
    memcpy(&storedCRC, binaryBuffer + CRC_OFFSET, 4);
    Serial.printf("Stored CRC:   0x%08X\n", storedCRC);

    uint32_t computedCRC = crc32(binaryBuffer, bytesRead, CRC_OFFSET);
    Serial.printf("Computed CRC: 0x%08X\n", computedCRC);

    if (computedCRC == storedCRC) {
      Serial.println("CRC valid!");
    } else {
      Serial.println("CRC mismatch — binary may be corrupt");
    }
  } else {
    Serial.println("HTTP GET failed: " + String(httpCode));
  }
  http.end();
}

// Writes binary data to LittleFS as /firmware.bin
void storeBinaryToLittleFS(uint8_t* data, int size, const char* filename) {
  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.write(data, size);
  file.close();
  Serial.println("Binary stored to LittleFS");
}

// Reads /firmware.bin from LittleFS into a buffer
void readBinaryFromLittleFS(uint8_t* buffer, int* size, const char* filename) {
  File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  *size = file.read(buffer, BINARY_MAX_SIZE);
  file.close();
  Serial.println("Binary read from LittleFS");
}

// HTTP handler: reads binary from LittleFS and returns size + first 16 byte
// used to check if write was done properly
void handleVerify() {
  String response = "";

  // Verify firmware A
  int sizeA = 0;
  readBinaryFromLittleFS(binaryBuffer, &sizeA, "/firmware_a.bin");
  if (sizeA == 0) {
    response += "A: Failed to read file\n";
  } else {
    response += "A — size: " + String(sizeA) + " bytes, first 16 bytes: ";
    for (int i = 0; i < 16; i++) {
      response += String(binaryBuffer[i], HEX) + " ";
    }
    response += "\n";
  }

  // Verify firmware B
  int sizeB = 0;
  readBinaryFromLittleFS(binaryBuffer, &sizeB, "/firmware_b.bin");
  if (sizeB == 0) {
    response += "B: Failed to read file\n";
  } else {
    response += "B — size: " + String(sizeB) + " bytes, first 16 bytes: ";
    for (int i = 0; i < 16; i++) {
      response += String(binaryBuffer[i], HEX) + " ";
    }
    response += "\n";
  }

  server.send(200, "text/plain", response);
}

// Polls the update server for the latest firmware version number.
// Fetches version.txt from the server and compares it to the current version.
// If a newer version is available, triggers fetchBinary() to download the new firmware
// and updates currentVersion to reflect the installed version.
void checkForUpdate() {
  HTTPClient http;
  http.begin(VERSION_TXT);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String body = http.getString();
    body.trim();

    // Parse A=x
    int aIndex = body.indexOf("A=");
    int serverVersionA = (aIndex != -1) ? body.substring(aIndex + 2).toInt() : -1;

    // Parse B=x
    int bIndex = body.indexOf("B=");
    int serverVersionB = (bIndex != -1) ? body.substring(bIndex + 2).toInt() : -1;

    Serial.println("Server versions — A: " + String(serverVersionA) + ", B: " + String(serverVersionB));

    if (serverVersionA > currentVersionA) {
      Serial.println("New version for A found, fetching...");
      fetchBinary(LOCAL_IP_A, "/firmware_a.bin");
      currentVersionA = serverVersionA;
    } else {
      Serial.println("No update for A (server: " + String(serverVersionA) + ", current: " + String(currentVersionA) + ")");
    }

    if (serverVersionB > currentVersionB) {
      Serial.println("New version for B found, fetching...");
      fetchBinary(LOCAL_IP_B, "/firmware_b.bin");
      currentVersionB = serverVersionB;
    } else {
      Serial.println("No update for B (server: " + String(serverVersionB) + ", current: " + String(currentVersionB) + ")");
    }
  } else {
    Serial.print("http.GET failed - not 200 return ");
  }
  http.end();
}

// HTTP handler: reads binary from LittleFS and sends it over UART2 to STM32
// SKELETON code to get started below
void handleSendBinary() {
  int size = 0;
  // Right now just sends firmwareA, FIX LATER!!
  readBinaryFromLittleFS(binaryBuffer, &size, "/firmware_a.bin");
  if (size == 0) {
    server.send(500, "text/plain", "Failed to read binary from LittleFS");
    return;
  }
  Serial2.write(binaryBuffer, size);
  Serial.println("Sent " + String(size) + " bytes over UART");
  server.send(200, "text/plain", "Sent " + String(size) + " bytes over UART");
}

void sendPartialBinary() {
  int size = 0;
  readBinaryFromLittleFS(binaryBuffer, &size, "/firmware_a.bin");
  
  if (size == 0) {
    server.send(500, "text/plain", "Failed to read binary from LittleFS");
    return;
  }

  for (int location = 0; location <= size -2; location += 2) 
  {
    /* 		[1][x][x] - Half word data transmission */
    uint8_t data_transmission[] = {0x01, binaryBuffer[location], binaryBuffer[location + 1]};

    //send the data
    Serial2.write(data_transmission, 3);
    Serial.println("Sent " + String(3) + " bytes over UART");
    server.send(200, "text/plain", "Sent " + String(3) + " bytes over UART");  

    //check to make sure the STM is good for more data
    char response = waitForSTMResponse();
    if (response != 0xFF) {
      continue;
    }
    handleCriticalFailure();
  }

  //send CRC value, we finishe transmitting all the data
  /* 	[2][x][x] - Transmission complete stop byte (must send 3 bytes still) */
  uint8_t crc_data[] = {0x1,0x2};
  uint8_t data_transmission[] = {0x02, crc_data[0], crc_data[1]};
  char response = waitForSTMResponse();
  if (response != 0xFF ) {
    
    //made it to the EOF and send CRC already, the response from the STM32 is good
    /* 	[0][x][x] - Transmission complete stop byte (must send 3 bytes still) */
    uint8_t data_transmission[] = {0x00, 0x99, 0x99};
    char response = waitForSTMResponse();
    if (response == 0x00 ) {
      handleSuccessfulUpdate();
      return;
    }
  }
  handleCriticalFailure();
}
