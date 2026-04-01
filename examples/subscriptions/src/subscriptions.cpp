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

    {
        // Callbacks with no topic argument
        mqtt.subscribe("picomqtt/no_topic/void_ptr_payload",
                       [](void * data, size_t size) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/no_topic/const_void_ptr_payload",
                       [](const void * data, size_t size) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/no_topic/char_ptr_payload", [](char * data) {
            Serial.printf("%s:%u\n", __FILE__, __LINE__);
        });

        mqtt.subscribe("picomqtt/no_topic/const_char_ptr_payload",
                       [](const char * data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/no_topic/string_payload", [](String data) {
            Serial.printf("%s:%u\n", __FILE__, __LINE__);
        });

        mqtt.subscribe("picomqtt/no_topic/const_string_ref_payload",
                       [](const String & data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });
    }

    {
        // Callbacks with char * argument
        mqtt.subscribe("picomqtt/char_ptr_topic/void_ptr_payload",
                       [](char * topic, void * data, size_t size) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/char_ptr_topic/const_void_ptr_payload",
                       [](char * topic, const void * data, size_t size) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/char_ptr_topic/char_ptr_payload",
                       [](char * topic, char * data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/char_ptr_topic/const_char_ptr_payload",
                       [](char * topic, const char * data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/char_ptr_topic/string_payload",
                       [](char * topic, String data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/char_ptr_topic/const_string_ref_payload",
                       [](char * topic, const String & data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });
    }

    {
        // Callbacks with const char * topic argument
        mqtt.subscribe("picomqtt/const_char_ptr_topic/void_ptr_payload",
                       [](const char * topic, void * data, size_t size) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/const_char_ptr_topic/const_void_ptr_payload",
                       [](const char * topic, const void * data, size_t size) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/const_char_ptr_topic/char_ptr_payload",
                       [](const char * topic, char * data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/const_char_ptr_topic/const_char_ptr_payload",
                       [](const char * topic, const char * data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/const_char_ptr_topic/string_payload",
                       [](const char * topic, String data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/const_char_ptr_topic/const_string_ref_payload",
                       [](const char * topic, const String & data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });
    }

    {
        // Callbacks with String topic argument
        mqtt.subscribe("picomqtt/string_topic/void_ptr_payload",
                       [](String topic, void * data, size_t size) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/string_topic/const_void_ptr_payload",
                       [](String topic, const void * data, size_t size) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/string_topic/char_ptr_payload",
                       [](String topic, char * data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/string_topic/const_char_ptr_payload",
                       [](String topic, const char * data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/string_topic/string_payload",
                       [](String topic, String data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/string_topic/const_string_ref_payload",
                       [](String topic, const String & data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });
    }

    {
        // Callbacks with ref String topic argument
        mqtt.subscribe("picomqtt/const_string_ref_topic/void_ptr_payload",
                       [](const String & topic, void * data, size_t size) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe(
            "picomqtt/const_string_ref_topic/const_void_ptr_payload",
            [](const String & topic, const void * data, size_t size) {
                Serial.printf("%s:%u\n", __FILE__, __LINE__);
            });

        mqtt.subscribe("picomqtt/const_string_ref_topic/char_ptr_payload",
                       [](const String & topic, char * data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/const_string_ref_topic/const_char_ptr_payload",
                       [](const String & topic, const char * data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/const_string_ref_topic/string_payload",
                       [](const String & topic, String data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe("picomqtt/const_string_ref_topic/string_ref_payload",
                       [](const String & topic, const String & data) {
                           Serial.printf("%s:%u\n", __FILE__, __LINE__);
                       });

        mqtt.subscribe(
            "picomqtt/const_string_ref_topic/const_string_ref_payload",
            [](const String & topic, const String & data) {
                Serial.printf("%s:%u\n", __FILE__, __LINE__);
            });
    }

    mqtt.begin();
}

void loop() { mqtt.loop(); }
