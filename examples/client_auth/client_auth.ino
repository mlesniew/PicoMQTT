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

PicoMQTT::Client mqtt(
    "broker.hivemq.com",    // broker address (or IP)
    1883,                   // broker port (defaults to 1883)
    "esp-client",           // Client ID
    "username",             // MQTT username
    "password"              // MQTT password
);

// PicoMQTT::Client mqtt;       // This will work too, but configuration will have to be set later (e.g. in setup())

void setup() {
    // Setup serial
    Serial.begin(115200);

    // Connect to WiFi
    Serial.printf("Connecting to WiFi %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) { delay(1000); }
    Serial.println("WiFi connected.");

    // MQTT settings can be changed or set here instead
    mqtt.host = "broker.hivemq.com";
    mqtt.port = 1883;
    mqtt.client_id = "esp-" + WiFi.macAddress();
    mqtt.username = "username";
    mqtt.password = "secret";

    // Subscribe to a topic and attach a callback
    mqtt.subscribe("picomqtt/#", [](const char * topic, const char * payload) {
        // payload might be binary, but PicoMQTT guarantees that it's zero-terminated
        Serial.printf("Received message in topic '%s': %s\n", topic, payload);
    });

    mqtt.begin();
}

void loop() {
    mqtt.loop();

    // Changing host, port, client_id, username or password here is OK too.  Changes will take effect after next
    // reconnect.  To force reconnection, explicitly disconnect the client by calling mqtt.disconnect().
}
