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

unsigned long last_publish_time = 0;
int greeting_number = 1;

void setup() {
    // Setup serial
    Serial.begin(115200);

    // Connect to WiFi
    Serial.printf("Connecting to WiFi %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) { delay(1000); }
    Serial.println("WiFi connected.");

    // Subscribe to a topic and attach a callback
    mqtt.subscribe("picomqtt/#", [](const char * topic, const char * payload) {
        // payload might be binary, but PicoMQTT guarantees that it's zero-terminated
        Serial.printf("Received message in topic '%s': %s\n", topic, payload);
    });

    mqtt.begin();
}

void loop() {
    mqtt.loop();

    // Publish a greeting message every 3 seconds.
    if (millis() - last_publish_time >= 3000) {
        // We're publishing to a topic, which we're subscribed too.
        // The broker should deliver the messages back to us.
        String topic = "picomqtt/esp-" + WiFi.macAddress();
        String message = "Hello #" + String(greeting_number++);
        Serial.printf("Publishing message in topic '%s': %s\n", topic.c_str(), message.c_str());
        mqtt.publish(topic, message);
        last_publish_time = millis();
    }
}
