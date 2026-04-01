#include <PicoMQTT.h>
#include <PicoWebsocket.h>

// regular tcp server on port 1883
::WiFiServer tcp_server(1883);

// websocket server on tcp port 80
::WiFiServer websocket_underlying_server(80);
PicoWebsocket::Server<::WiFiServer> websocket_server(
    websocket_underlying_server);

// MQTT server using the two server instances
PicoMQTT::Server mqtt(tcp_server,
                      websocket_server);  // NOTE: this constructor can take any
                                          // number of server parameters

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
