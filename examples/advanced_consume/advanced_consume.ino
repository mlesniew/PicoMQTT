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

void setup() {
    // Setup serial
    Serial.begin(115200);

    // Connect to WiFi
    Serial.printf("Connecting to WiFi %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) { delay(1000); }
    Serial.println("WiFi connected.");


    mqtt.subscribe("picomqtt/string_payload", [](const char * payload) {
        // Use this type of handler to work with messages which should be strings.  PicoMQTT will always add an extra
        // zero terminator at the end of the payload, so it's safe to  treat it as a string.
        // Note however, that in the general case, payload can be binary (so it can contain any data, including zero
        // bytes).
        Serial.printf("Received message in topic 'picomqtt/string_payload': %s\n", payload);
    });


    mqtt.subscribe("picomqtt/binary_payload", [](const void * payload, size_t payload_size) {
        // Here the payload is in a void * buffer and payload_size tells us its size.
        Serial.printf("Received message in topic 'picomqtt/binary_payload', size: %u\n", payload_size);
    });


    mqtt.subscribe("picomqtt/string_payload/+/wildcard", [](const char * topic, const char * payload) {
        // Use this type of callback to capture both the topic and the payload as string

        // If the topic contains wildcards, their values can be extracted easily too
        String wildcard_value = mqtt.get_topic_element(topic, 2);

        Serial.printf("Received message in topic '%s' (wildcard = '%s'): %s\n", topic, wildcard_value.c_str(), payload);
    });


    mqtt.subscribe("picomqtt/binary_payload/+/wildcard", [](const char * topic, const void * payload, size_t payload_size) {
        // Here the payload is in a void * buffer and payload_size tells us its size.
        // If the topic contains wildcards, their values can be extracted easily too
        String wildcard_value = mqtt.get_topic_element(topic, 2);

        Serial.printf("Received message in topic '%s' (wildcard = '%s'), size: %u\n",
                      topic, wildcard_value.c_str(), payload_size);

    });


    mqtt.subscribe("picomqtt/big_message", [](const char * topic, PicoMQTT::IncomingPacket & packet) {
        // This is the most advanced handler type.  The packet parameter is a subclass of Stream and can be read from
        // in small chunks using Stream's typical API.  This is useful for handling messages too big to fit in RAM.

        // Retrieve payload size
        size_t payload_size = packet.get_remaining_size();

        size_t digit_count = 0;
        while (payload_size--) {
            int val = packet.read();
            if ((val >= '0') && (val <= '9')) {
                // the byte is a digit!
                ++digit_count;
            }
        }

        // Note that it's OK to not read the entire content of the message.  Reading beyond the end of the packet
        // is an error.
        Serial.printf("Received message in topic '%s', it contained %u digits\n", topic, digit_count);
    });

    // Arduino Strings can be used instead of const char *.  Note that this can be inefficient, especially with big
    // payloads.
    mqtt.subscribe("picomqtt/arduino_string/payload", [](const String & payload) {
        Serial.printf("Received message in topic 'picomqtt/arduino_string/payload': %s\n", payload.c_str());
    });

    mqtt.subscribe("picomqtt/arduino_string/+/topic_and_payload", [](const String & topic, const String & payload) {
        // If the topic contains wildcards, their values can be extracted easily too
        String wildcard_value = mqtt.get_topic_element(topic, 2);

        Serial.printf("Received message in topic '%s' (wildcard = '%s'): %s\n", topic.c_str(), wildcard_value.c_str(), payload.c_str());
    });

    // Different types of strings can be mixed too
    mqtt.subscribe("picomqtt/arduino_string/mix/1", [](const String & topic, const char * payload) {
        Serial.printf("Received message in topic '%s': %s\n", topic.c_str(), payload);
    });

    mqtt.subscribe("picomqtt/arduino_string/mix/2", [](const char * topic, const String & payload) {
        Serial.printf("Received message in topic '%s': %s\n", topic, payload.c_str());
    });

    // const and reference can be skipped too
    mqtt.subscribe("picomqtt/arduino_string/mix/3", [](char * topic, String payload) {
        Serial.printf("Received message in topic '%s': %s\n", topic, payload.c_str());
    });

    mqtt.begin();
}

void loop() {
    mqtt.loop();
}
