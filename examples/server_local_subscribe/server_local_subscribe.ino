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

PicoMQTT::ServerLocalSubscribe mqtt;
unsigned long last_publish;

static const char flash_string[] PROGMEM = "Hello from the broker's flash.";

void setup() {
    // Setup serial
    Serial.begin(115200);

    // Connect to WiFi
    Serial.printf("Connecting to WiFi %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) { delay(1000); }
    Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());

    mqtt.begin();

    mqtt.subscribe("picomqtt/foo", [](const char * payload) {
        Serial.printf("Message received: %s\n", payload);
    });
}


void loop() {
    mqtt.loop();

    if (millis() - last_publish >= 5000) {
        // this will publish the message to subscribed clients and fire the local callback
        mqtt.publish("picomqtt/foo", "hello from broker!");

        // this will also work as usual and fire the callback
        mqtt.publish_P("picomqtt/foo", flash_string);

        // begin_publish will publish to clients, but it will NOT fire the local callback
        auto publish = mqtt.begin_publish("picomqtt/foo", 33);
        publish.write((const uint8_t *) "Message delivered to clients only", 33);
        publish.send();

        last_publish = millis();
    }

}
