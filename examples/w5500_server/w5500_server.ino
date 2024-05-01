// Platform compatibility: espressif8266

#include <Arduino.h>
#include <Ethernet.h>

#include <PicoMQTT.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

EthernetServer server(1883);
PicoMQTT::Server mqtt(server);

void setup() {
    Serial.begin(115200);

    Serial.println("Connecting to network...");
    Ethernet.init(5); // ss pin
    while (!Ethernet.begin(mac)) {
        Serial.println("Failed, retrying...");
    }
    Serial.println(Ethernet.localIP());

    mqtt.begin();
}

void loop() {
    mqtt.loop();
}
