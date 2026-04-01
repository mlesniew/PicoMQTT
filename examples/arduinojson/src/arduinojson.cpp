#include <ArduinoJson.h>
#include <PicoMQTT.h>

PicoMQTT::Client mqtt("broker.hivemq.com");

unsigned long last_publish_time = 0;
int greeting_number = 1;

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

    // Subscribe to a topic and attach a callback
    mqtt.subscribe("picomqtt/json/#", [](const char * topic, Stream & stream) {
        Serial.printf("Received message in topic '%s':\n", topic);
        JsonDocument json;
        if (deserializeJson(json, stream)) {
            Serial.println("Json parsing failed.");
            return;
        }
        serializeJsonPretty(json, Serial);
        Serial.println();
    });

    mqtt.begin();
}

void loop() {
    mqtt.loop();

    // Publish a greeting message every 3 seconds.
    if (millis() - last_publish_time >= 3000) {
        String topic = "picomqtt/json/esp-" + WiFi.macAddress();
        Serial.printf("Publishing in topic '%s'...\n", topic.c_str());

        // build JSON document
        JsonDocument json;
        json["foo"] = "bar";
        json["millis"] = millis();

        // publish using begin_publish()/send() API
        auto publish = mqtt.begin_publish(topic, measureJson(json));
        serializeJson(json, publish);
        publish.send();

        last_publish_time = millis();
    }
}
