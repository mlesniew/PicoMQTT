#include <PicoMQTT.h>

PicoMQTT::Client mqtt("broker.hivemq.com");

unsigned long last_subscribe_time = 0;
int resubscribe = 1;

void handler_foo(const char * topic, const char * payload) {
    Serial.printf("Handler foo received message in topic '%s': %s\n", topic,
                  payload);
}

void handler_bar(const char * topic, const char * payload) {
    Serial.printf("Handler bar received message in topic '%s': %s\n", topic,
                  payload);
}

void setup() {
    // Setup serial
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    Serial.printf("Connecting to WiFi %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
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
