// dependencies: mlesniew/PicoWebsocket@1.1.0

#include <PicoMQTT.h>
#include <PicoWebsocket.h>

#if __has_include("config.h")
#include "config.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "WiFi SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "password"
#endif

WiFiServer server(80);
PicoWebsocket::Server<::WiFiServer> websocket_server(server);
PicoMQTT::Server mqtt(websocket_server);

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
}

void loop() {
    mqtt.loop();
}
