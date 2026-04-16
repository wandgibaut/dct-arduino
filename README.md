# dct-arduino

Arduino helpers and examples for the Distributed Cognitive Toolkit (DCT).

This repository is the Arduino companion to `../dct-python`. Arduino boards do
not connect directly to Redis, MongoDB, or local JSON files. Instead, they talk
to the HTTP API exposed by the Python DCT server:

- `GET /get_memory/<memory_name>`
- `POST /set_memory/`

The included `DCTClient` wrapper keeps sketches small while staying compatible
with the memory format and endpoints used by `dct-python`.

## Supported Boards

- ESP8266
- ESP32

Other Arduino boards can be supported later if they provide a compatible WiFi
and HTTP client stack.

## Dependencies

Install these libraries in the Arduino IDE Library Manager or through
`arduino-cli`:

- `ArduinoJson`
- ESP8266 or ESP32 board support package

## Python Server Requirement

Start a DCT Python server on the same network as the board:

```bash

pip install "dct-python[server]"
ROOT_NODE_DIR=/path/to/node python -m dct.server 0.0.0.0:9999
```

The node directory should contain codelet `fields.json` files like the Python
package expects. A minimal memory entry looks like this:

```json
{
  "enable": true,
  "lock": false,
  "timestep": 0.1,
  "inputs": [
    {
      "name": "perceptual-memory",
      "ip/port": "/path/to/memories",
      "type": "local",
      "group": []
    }
  ],
  "outputs": []
}
```

For a local memory backend, create `/path/to/memories/perceptual-memory.json`:

```json
{
  "name": "perceptual-memory",
  "ip/port": "/path/to/memories",
  "type": "local",
  "group": [],
  "I": 0,
  "eval": 0.0
}
```

## Basic Sketch

```cpp
#include <ArduinoJson.h>
#include <DCTArduino.h>

DCTClient dct("192.168.1.10", 9999);

void loop() {
  DynamicJsonDocument memory(1024);

  if (dct.getFirstMemory("perceptual-memory", memory)) {
    int value = memory["I"] | 0;
    dct.setMemoryField("perceptual-memory", "I", value + 1);
  }

  delay(500);
}
```

`getMemory()` returns the raw Python server response. The server response is
usually a JSON array because more than one codelet may expose a memory with the
same name. `getFirstMemory()` is a convenience helper for the common Arduino
case where only one memory is expected.

## Examples

- `examples/EspMemoryIncrement/EspMemoryIncrement.ino`: current ESP8266/ESP32
  example that increments `perceptual-memory.I`.
- `arduino/arduinoCodelet/espTest/espClient.ino`: compatibility path for the
  original sketch, updated to use the HTTP API.

Before uploading an example:

1. Set `WIFI_SSID` and `WIFI_PASSWORD`.
2. Set `DCT_HOST`/`host` to the machine running `dct-python`.
3. Set `DCT_PORT`/`port` to the server port.
4. Confirm the memory name exists in the Python node's `fields.json`.

## API

```cpp
DCTClient dct("192.168.1.10", 9999);
dct.setTimeout(5000);

DynamicJsonDocument raw(2048);
dct.getMemory("perceptual-memory", raw);

DynamicJsonDocument first(1024);
dct.getFirstMemory("perceptual-memory", first);

dct.setMemoryField("perceptual-memory", "I", 42);
dct.setMemoryField("perceptual-memory", "eval", 0.8);
dct.setMemoryField("perceptual-memory", "value", "ready");

DynamicJsonDocument node(1024);
dct.getNodeInfo(node);

DynamicJsonDocument codelet(2048);
dct.getCodeletInfo("my-codelet", codelet);

DynamicJsonDocument idea(1024);
dct.getIdea("candidate-idea", idea);
dct.setIdeaField("candidate-idea", "value", 1.0);
```

For object or array values, use `setMemoryFieldJson()` with an ArduinoJson
variant. For idea objects, use `setIdeaFieldJson()` or `setFullIdea()`.

## Compatibility Notes

- `dct-python` uses HTTP for the `tcp` connection type, so this library uses
  HTTP rather than raw socket command strings.
- `POST /set_memory/` only updates one field at a time, matching
  `dct.api.set_tcp_memory`.
- The wrapper also includes the Python server's small metadata and idea
  endpoints: `/get_node_info`, `/get_codelet_info/<codelet_name>`,
  `/get_idea/<idea_name>`, and `/set_idea/`.
- Authentication, TLS, retries, Redis, MongoDB, and local file access remain on
  the Python server side.
- Memory documents should be small enough for the `DynamicJsonDocument`
  capacity used in the sketch.
