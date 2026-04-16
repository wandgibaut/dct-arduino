/*
  DCTArduino

  Minimal Arduino client for the Distributed Cognitive Toolkit (DCT) HTTP API.
  It is intentionally small: Arduino boards use the DCT Python server as the
  bridge to local, Redis, MongoDB, or other memory backends.
*/

#ifndef DCT_ARDUINO_H
#define DCT_ARDUINO_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ctype.h>

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <HTTPClient.h>
#include <WiFi.h>
#else
#error "DCTArduino currently supports ESP8266 and ESP32 WiFi boards."
#endif

class DCTClient {
 public:
  DCTClient() : _host(""), _port(0), _timeoutMs(5000), _lastStatusCode(0), _lastError("") {}

  DCTClient(const char* host, uint16_t port)
      : _host(host), _port(port), _timeoutMs(5000), _lastStatusCode(0), _lastError("") {}

  void begin(const char* host, uint16_t port) {
    _host = host;
    _port = port;
    _lastStatusCode = 0;
    _lastError = "";
  }

  void setTimeout(uint16_t timeoutMs) {
    _timeoutMs = timeoutMs;
  }

  int lastStatusCode() const {
    return _lastStatusCode;
  }

  const String& lastError() const {
    return _lastError;
  }

  bool isConfigured() const {
    return _host.length() > 0 && _port > 0;
  }

  /*
    Fetch the raw DCT server response for GET /get_memory/<memory_name>.

    The Python server returns a JSON array because more than one memory with the
    same name may exist across codelets. Use getFirstMemory() when your Arduino
    codelet expects exactly one matching memory.
  */
  bool getMemory(const char* memoryName, DynamicJsonDocument& output) {
    String url = buildUrl("/get_memory/") + urlEncode(memoryName);
    return getJson(url, output);
  }

  /*
    Fetch the first memory object returned by GET /get_memory/<memory_name>.

    This mirrors the common Python helper flow where a codelet reads one named
    memory and then updates a field such as I, eval, value, or group.
  */
  bool getFirstMemory(const char* memoryName, DynamicJsonDocument& output) {
    DynamicJsonDocument response(output.capacity());

    if (!getMemory(memoryName, response)) {
      return false;
    }

    if (response.is<JsonArray>()) {
      JsonArray memories = response.as<JsonArray>();
      if (memories.size() == 0) {
        _lastError = "No memory returned";
        return false;
      }

      output.clear();
      output.set(memories[0]);
      if (output.overflowed()) {
        _lastError = "Output JSON document capacity exceeded";
        return false;
      }
      _lastError = "";
      return true;
    }

    if (response.is<JsonObject>()) {
      output.clear();
      output.set(response.as<JsonObject>());
      if (output.overflowed()) {
        _lastError = "Output JSON document capacity exceeded";
        return false;
      }
      _lastError = "";
      return true;
    }

    _lastError = "Memory response is not an object or array";
    return false;
  }

  /*
    Update one field through POST /set_memory/.

    The JSON body follows dct-python:
      {"memory_name": "<name>", "field": "<field>", "value": <value>}
  */
  template <typename T>
  bool setMemoryField(const char* memoryName, const char* field, const T& value) {
    DynamicJsonDocument payload(512);
    payload["memory_name"] = memoryName;
    payload["field"] = field;
    payload["value"] = value;
    return postSetMemory(payload);
  }

  bool setMemoryFieldJson(const char* memoryName, const char* field, JsonVariantConst value) {
    DynamicJsonDocument payload(1024);
    payload["memory_name"] = memoryName;
    payload["field"] = field;
    payload["value"].set(value);
    return postSetMemory(payload);
  }

  bool getIdea(const char* ideaName, DynamicJsonDocument& output) {
    String url = buildUrl("/get_idea/") + urlEncode(ideaName);
    return getJson(url, output);
  }

  template <typename T>
  bool setIdeaField(const char* ideaName, const char* field, const T& value) {
    DynamicJsonDocument payload(512);
    payload["name"] = ideaName;
    payload["field"] = field;
    payload["value"] = value;
    return postJson(buildUrl("/set_idea/"), payload);
  }

  bool setIdeaFieldJson(const char* ideaName, const char* field, JsonVariantConst value) {
    DynamicJsonDocument payload(1024);
    payload["name"] = ideaName;
    payload["field"] = field;
    payload["value"].set(value);
    return postJson(buildUrl("/set_idea/"), payload);
  }

  bool setFullIdea(JsonVariantConst idea) {
    DynamicJsonDocument payload(1024);
    payload["full_idea"].set(idea);
    return postJson(buildUrl("/set_idea/"), payload);
  }

  bool getNodeInfo(DynamicJsonDocument& output) {
    return getJson(buildUrl("/get_node_info"), output);
  }

  bool getCodeletInfo(const char* codeletName, DynamicJsonDocument& output) {
    String url = buildUrl("/get_codelet_info/") + urlEncode(codeletName);
    return getJson(url, output);
  }

 private:
  String _host;
  uint16_t _port;
  uint16_t _timeoutMs;
  int _lastStatusCode;
  String _lastError;

  bool ensureConfigured() {
    if (!isConfigured()) {
      _lastError = "DCTClient host/port not configured";
      return false;
    }
    return true;
  }

  String buildUrl(const char* path) const {
    String url = "http://";
    url += _host;
    url += ":";
    url += String(_port);
    url += path;
    return url;
  }

  bool postSetMemory(DynamicJsonDocument& payload) {
    return postJson(buildUrl("/set_memory/"), payload);
  }

  bool getJson(const String& url, DynamicJsonDocument& output) {
    if (!ensureConfigured()) {
      return false;
    }

    WiFiClient wifiClient;
    HTTPClient http;

    if (!http.begin(wifiClient, url)) {
      _lastError = "HTTP begin failed";
      return false;
    }

    http.setTimeout(_timeoutMs);
    _lastStatusCode = http.GET();

    if (_lastStatusCode != HTTP_CODE_OK) {
      _lastError = "GET failed with status " + String(_lastStatusCode);
      http.end();
      return false;
    }

    DeserializationError error = deserializeJson(output, http.getStream());
    http.end();

    if (error) {
      _lastError = "JSON parse failed: ";
      _lastError += error.c_str();
      return false;
    }

    _lastError = "";
    return true;
  }

  bool postJson(const String& url, DynamicJsonDocument& payload) {
    if (!ensureConfigured()) {
      return false;
    }

    WiFiClient wifiClient;
    HTTPClient http;
    String body;

    if (payload.overflowed()) {
      _lastError = "JSON payload capacity exceeded";
      return false;
    }

    serializeJson(payload, body);

    if (!http.begin(wifiClient, url)) {
      _lastError = "HTTP begin failed";
      return false;
    }

    http.setTimeout(_timeoutMs);
    http.addHeader("Content-Type", "application/json");
    _lastStatusCode = http.POST(body);
    http.end();

    if (_lastStatusCode < 200 || _lastStatusCode >= 300) {
      _lastError = "POST failed with status " + String(_lastStatusCode);
      return false;
    }

    _lastError = "";
    return true;
  }

  String urlEncode(const char* value) const {
    String encoded;
    const char* hex = "0123456789ABCDEF";

    while (*value) {
      uint8_t c = static_cast<uint8_t>(*value++);
      if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
        encoded += static_cast<char>(c);
      } else {
        encoded += '%';
        encoded += hex[(c >> 4) & 0x0F];
        encoded += hex[c & 0x0F];
      }
    }

    return encoded;
  }
};

#endif
