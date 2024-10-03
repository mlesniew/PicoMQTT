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

unsigned long last_subscribe_time = 0;
int resubscribe = 1;

void handler_foo(const char * topic, const char * payload) {
    Serial.printf("Handler foo received message in topic '%s': %s\n", topic, payload);
}

void handler_bar(const char * topic, const char * payload) {
    Serial.printf("Handler bar received message in topic '%s': %s\n", topic, payload);
}

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

    // Resubscribe every 5 seconds
    if (millis() - last_subscribe_time >= 5000) {
        if (++resubscribe % 2 == 0) {
            mqtt.subscribe("picomqtt/#", handler_foo);
        } else {
            mqtt.subscribe("picomqtt/#", handler_bar);
        }
        last_subscribe_time = millis();
    }
}
