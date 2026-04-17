#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

#define LED_DELAY 250U

// put function declarations here:
int myFunction(int, int);
void togglePin(int pin);
void handleFlash();
void handleSend();

const char* ssid = "iPhone";
const char* password = "Lesant4278";

WebServer server(8080);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  delay(500);
  Serial.println("Hello, World!");
  pinMode(LED_BUILTIN, OUTPUT);

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

  server.on("/flash", handleFlash);
  server.on("/send", handleSend);
  server.begin();

  //void loop is called here so it's acutally doing:
  /* 
  void loop() {
    server.handleClient();
  }*/
}

void loop() {
  server.handleClient();
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}

//toggle pin
void togglePin(int pin) {
  digitalWrite(pin, !digitalRead(pin));
}

void handleFlash() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
  server.send(200, "text/plain", "LED flashed!");
}

void handleSend() {
  if (server.hasArg("msg")) {
    String message = server.arg("msg");
    Serial2.print(message);
    server.send(200, "text/plain", "Sent: " + message);
  } else {
    server.send(400, "text/plain", "Missing msg parameter");
  }
}

void fetchBinary() {
  HTTPClient http;
  http.begin("http://<your-computer-IP>:8080/firmware.bin");
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    int contentLength = http.getSize();
    WiFiClient *stream = http.getStreamPtr();
    
    // Read binary data chunk by chunk
    uint8_t buffer[128];
    while (http.connected() && contentLength > 0) {
      int bytesRead = stream->readBytes(buffer, sizeof(buffer));
      // Do something with buffer here
      contentLength -= bytesRead;
    }
    Serial.println("Done fetching binary");
  } else {
    Serial.println("HTTP GET failed: " + String(httpCode));
  }
  http.end();
}
