/*
  Backward-compatible location for the original ESP codelet example.

  This sketch now uses the DCTArduino library wrapper instead of sending legacy
  raw TCP command strings. It is compatible with the dct-python server HTTP API:
    GET  /get_memory/<memory_name>
    POST /set_memory/
*/

#include <ArduinoJson.h>
#include <DCTArduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

const char* ssid = "your-ssid";
const char* password = "your-password";

const char* host = "192.168.1.10";
const uint16_t port = 9999;
const char* memoryName = "perceptual-memory";

DCTClient dct(host, port);

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  DynamicJsonDocument memory(1024);

  if (!dct.getFirstMemory(memoryName, memory)) {
    Serial.print("DCT read failed: ");
    Serial.println(dct.lastError());
    delay(5000);
    return;
  }

  int I = memory["I"] | 0;
  I += 1;

  if (!dct.setMemoryField(memoryName, "I", I)) {
    Serial.print("DCT write failed: ");
    Serial.println(dct.lastError());
    delay(5000);
    return;
  }

  Serial.print(memoryName);
  Serial.print(".I = ");
  Serial.println(I);

  delay(500);
}
