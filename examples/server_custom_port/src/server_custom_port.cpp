#include <PicoMQTT.h>

PicoMQTT::Server mqtt(9000);  // listening port: TCP 9000

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
    Serial.printf("WiFi connected, IP: %s\n",
                  WiFi.localIP().toString().c_str());

    mqtt.begin();
}

void loop() { mqtt.loop(); }
