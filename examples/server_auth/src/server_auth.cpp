#include <PicoMQTT.h>

class MQTT : public PicoMQTT::Server {
protected:
    PicoMQTT::ConnectReturnCode auth(const char * client_id,
                                     const char * username,
                                     const char * password) override {
        // only accept client IDs which are 3 chars or longer
        if (String(client_id).length() < 3) {  // client_id is never NULL
            return PicoMQTT::CRC_IDENTIFIER_REJECTED;
        }

        // only accept connections if username and password are provided
        if (!username || !password) {  // username and password can be NULL
            // no username or password supplied
            return PicoMQTT::CRC_NOT_AUTHORIZED;
        }

        // accept two user/password combinations
        if (((String(username) == "alice") && (String(password) == "secret")) ||
            ((String(username) == "bob") && (String(password) == "password"))) {
            return PicoMQTT::CRC_ACCEPTED;
        }

        // reject all other credentials
        return PicoMQTT::CRC_BAD_USERNAME_OR_PASSWORD;
    }
} mqtt;

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
