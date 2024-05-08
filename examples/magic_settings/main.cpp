// dependencies: https://github.com/mlesniew/PicoWebsocket.git bblanchon/ArduinoJson m5stack/M5Unified
// platform compatibility: espressif32

#include <M5Unified.h>
#include <PicoMQTT.h>
#include <PicoWebsocket.h>
#include "magicsettings.hpp"

#if __has_include("config.h")
#include "config.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "WiFi SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "password"
#endif

void on_fparam_change(void);

PicoMQTT::Server mqtt;
MagicSettings settings(mqtt, "testns");

MagicSettings::Setting<int> bar(settings, "bar", 42);
MagicSettings::Setting<String> baz(settings, "baz", "The Answer.");
MagicSettings::Setting<double> dparam(settings, "dparam", 3.14);
MagicSettings::Setting<float> fparam(settings, "fparam", 2.718, on_fparam_change);
MagicSettings::Setting<bool> flag(settings, "flag", true, [] {
    log_i("flag=%d", flag.get());
});

// value change callback
void on_fparam_change(void) {
    log_i("fparam changed to %f, default value: %f",
          fparam.get(), fparam.get_default());
}

void setup() {

    delay(3000);
    M5.begin();

    Serial.begin(115200);

    // Connect to WiFi
    Serial.printf("Connecting to WiFi %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
    delay(1000);
    Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());

    mqtt.begin();
    settings.begin();
    settings.publish();
    bar.change_callback = [] {
        log_i("bar changed to %d", bar.get());
    };
}

uint32_t last;

void loop() {
    mqtt.loop();

    // periodically publish the namespace
    // and the reset special topic
    if (millis() - last > 3000) {
        settings.publish();
        last = millis();
    }
    yield();
}
