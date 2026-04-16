/*
  DCT Arduino memory increment example.

  Requirements:
  - ESP8266 or ESP32 board.
  - ArduinoJson library installed.
  - A running dct-python server, for example:
      ROOT_NODE_DIR=/path/to/node python -m dct.server 0.0.0.0:9999
  - A fields.json entry that exposes a memory named "perceptual-memory".

  The sketch reads the first memory object returned by the server, increments
  its I field, and writes the new value back through /set_memory/.
*/

#include <ArduinoJson.h>
#include <DCTArduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

const char* WIFI_SSID = "your-ssid";
const char* WIFI_PASSWORD = "your-password";

const char* DCT_HOST = "192.168.1.10";
const uint16_t DCT_PORT = 9999;
const char* MEMORY_NAME = "perceptual-memory";

DCTClient dct(DCT_HOST, DCT_PORT);

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(100);
  connectWifi();
}

void loop() {
  DynamicJsonDocument memory(1024);

  if (!dct.getFirstMemory(MEMORY_NAME, memory)) {
    Serial.print("DCT read failed: ");
    Serial.println(dct.lastError());
    delay(5000);
    return;
  }

  int value = memory["I"] | 0;
  value += 1;

  if (!dct.setMemoryField(MEMORY_NAME, "I", value)) {
    Serial.print("DCT write failed: ");
    Serial.println(dct.lastError());
    delay(5000);
    return;
  }

  Serial.print(MEMORY_NAME);
  Serial.print(".I = ");
  Serial.println(value);

  delay(500);
}
