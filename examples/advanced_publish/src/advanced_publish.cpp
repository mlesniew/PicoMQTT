#include <PicoMQTT.h>

#if __has_include("config.h")
#include "config.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "WiFi SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "password"
#endif

PicoMQTT::Client mqtt("broker.hivemq.com");

// The example will also work with a broker -- just replace the line above with:
// PicoMQTT::Server mqtt;

unsigned long last_publish_time = 0;

static const char flash_string[] PROGMEM = "This is a string stored in flash.";

void setup() {
    // Setup serial
    Serial.begin(115200);

    // Connect to WiFi
    Serial.printf("Connecting to WiFi %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) { delay(1000); }
    Serial.println("WiFi connected.");

    mqtt.begin();
}

void loop() {
    mqtt.loop();

    // Publish a greeting message every 10 seconds.
    if (millis() - last_publish_time >= 10000) {
        // We're publishing to a topic, which we're subscribed too.
        // The broker should deliver the messages back to us.

        // publish a literal flash string
        mqtt.publish("picomqtt/flash_string/literal", F("Literal PGM string"));

        // publish a string from a PGM global
        mqtt.publish_P("picomqtt/flash_string/global", flash_string);

        // The topic can be an F-string too:
        mqtt.publish_P(F("picomqtt/flash_string/global"), flash_string);

        // publish binary data
        const char binary_payload[] = "This string could contain binary data including a zero byte";
        size_t binary_payload_size = strlen(binary_payload);
        mqtt.publish("picomqtt/binary_payload", (const void *) binary_payload, binary_payload_size);

        // Publish a big message in small chunks
        auto publish = mqtt.begin_publish("picomqtt/chunks", 1000);

        // Here we're writing 10 bytes 100 times
        for (int i = 0; i < 1000; i += 10) {
            publish.write((const uint8_t *) "1234567890", 10);
        }

        // In case of chunked published, an explicit call to send() is required.
        publish.send();

        last_publish_time = millis();
    }
}
