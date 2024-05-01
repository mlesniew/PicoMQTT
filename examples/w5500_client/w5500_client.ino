// Platform compatibility: espressif8266

#include <Arduino.h>
#include <Ethernet.h>

#include <PicoMQTT.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

EthernetClient client;
PicoMQTT::Client mqtt(client, "broker.hivemq.com");

unsigned long last_publish_time = 0;
int greeting_number = 1;

void setup() {
    Serial.begin(115200);

    Serial.println("Connecting to network...");
    Ethernet.init(5); // ss pin
    while (!Ethernet.begin(mac)) {
        Serial.println("Failed, retrying...");
    }
    Serial.println(Ethernet.localIP());

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
