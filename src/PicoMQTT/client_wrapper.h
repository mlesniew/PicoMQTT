#pragma once

#include <WiFiClient.h>

namespace PicoMQTT {

class ClientWrapper: public ::WiFiClient {
    public:
        ClientWrapper(unsigned long socket_timeout_seconds);
        ClientWrapper(const WiFiClient & client, unsigned long socket_timeout_seconds);
        ClientWrapper(const ClientWrapper &) = default;

        virtual int peek() override;
        virtual int read() override;
        virtual int read(uint8_t * buf, size_t size) override;
        virtual size_t write(const uint8_t * buffer, size_t size) override;
        virtual size_t write(uint8_t value) override final;

#ifdef ESP32
        // these methods are only available in WiFiClient on ESP8266
        uint8_t status() { return connected(); }
        void abort() { stop(); }
#endif

        const unsigned long socket_timeout_millis;

    protected:
        int available_wait(unsigned long timeout);
};

}
