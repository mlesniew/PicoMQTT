#pragma once

#include <Arduino.h>

#if defined(ESP8266)

#if (ARDUINO_ESP8266_MAJOR != 3) || (ARDUINO_ESP8266_MINOR < 1)
#error PicoMQTT requires ESP8266 board core version >= 3.1
#endif

#elif defined(ESP32)

#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(2, 0, 7)
#error PicoMQTT requires ESP32 board core version >= 2.0.7
#endif

#endif

#include "PicoMQTT/client.h"
#include "PicoMQTT/server.h"
