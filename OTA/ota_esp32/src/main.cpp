// ESP32 OTA middleman
// Receives firmware binary over WiFi from a host PC and forwards it to STM32 over UART2
// HTTP endpoints: /fetch, /verify, /send_binary, /send, /flash
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <LittleFS.h>

#define LED_DELAY 250U
#define BINARY_MAX_SIZE 14000 // current size is 12708, could change later
#define SSID "LeChalet2.4Gz"
#define PASS "1001ABNBEmpire!"
#define LOCAL_IP "http://10.0.0.88:8080/OTA_app_a.bin"
#define VERSION_TXT "http://10.0.0.88:8080/version.txt"

// put function declarations here:
int myFunction(int, int);
void togglePin(int pin);
void handleFlash();
void handleSend();
void fetchBinary();
void storeBinaryToLittleFS(uint8_t* data, int size);
void readBinaryFromLittleFS(uint8_t* buffer, int* size);
void handleVerify();
void handleSendBinary();
void checkForUpdate();


// WARNING: update these credentials before flashing in the define at the top of file
const char* ssid = SSID;
const char* password = PASS;

unsigned long lastCheck = 0;
const unsigned long CHECK_INTERVAL = 30000; // check every 30 seconds
int currentVersion = 1;

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
  fetchBinary();
  server.send(200, "text/plain", "Binary fetched and stored");
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
  }

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

// Fetches binary file from HTTP server and stores it in LittleFS
// LittleFS is a flash based filesystem on the ESP32 that persists across power cycles
// You can access relevant sections by just using the name of the file, no need to deal with addresses
// currently the file name is firmware.bin, will likely change later...
void fetchBinary() {
  HTTPClient http;
  // Update this URL when the host PC IP or filename changes
  http.begin(LOCAL_IP);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    int contentLength = http.getSize();
    WiFiClient *stream = http.getStreamPtr();
    
    // Read binary data chunk by chunk
    int bytesRead = stream->readBytes(binaryBuffer, contentLength);

    Serial.println("Fetched " + String(bytesRead) + " bytes");
    storeBinaryToLittleFS(binaryBuffer, bytesRead);
  } else {
    Serial.println("HTTP GET failed: " + String(httpCode));
  }
  http.end();
}

// Writes binary data to LittleFS as /firmware.bin
void storeBinaryToLittleFS(uint8_t* data, int size) {
  File file = LittleFS.open("/firmware.bin", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.write(data, size);
  file.close();
  Serial.println("Binary stored to LittleFS");
}

// Reads /firmware.bin from LittleFS into a buffer
void readBinaryFromLittleFS(uint8_t* buffer, int* size) {
  File file = LittleFS.open("/firmware.bin", "r");
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
  int size = 0;
  readBinaryFromLittleFS(binaryBuffer, &size);
  if (size == 0) {
    server.send(500, "text/plain", "Failed to read file");
    return;
  }
  String response = "File size: " + String(size) + " bytes\nFirst 16 bytes: ";
  for (int i = 0; i < 16; i++) {
    response += String(binaryBuffer[i], HEX) + " ";
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
    String versionStr = http.getString();
    versionStr.trim();
    int serverVersion = versionStr.toInt();
    if (serverVersion > currentVersion) {
      Serial.println("New version found, fetching...");
      fetchBinary();
      currentVersion = serverVersion;
    } else {
      Serial.println("No update available (server: " + String(serverVersion) + ", current: " + String(currentVersion) + ")");
    }
  }
  http.end();
}

// HTTP handler: reads binary from LittleFS and sends it over UART2 to STM32
// SKELETON code to get started below
void handleSendBinary() {
  int size = 0;
  readBinaryFromLittleFS(binaryBuffer, &size);
  if (size == 0) {
    server.send(500, "text/plain", "Failed to read binary from LittleFS");
    return;
  }
  Serial2.write(binaryBuffer, size);
  Serial.println("Sent " + String(size) + " bytes over UART");
  server.send(200, "text/plain", "Sent " + String(size) + " bytes over UART");
}
