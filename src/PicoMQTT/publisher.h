#pragma once

#include <Arduino.h>

#include "outgoing_packet.h"
#include "print_mux.h"

namespace PicoMQTT {

class Publisher {
    public:
        class Publish: public OutgoingPacket {
            private:
                Publish(Publisher & publisher, const PrintMux & print, Buffer & buffer,
                        uint8_t flags, size_t total_size,
                        const char * topic, size_t topic_size,
                        uint16_t message_id);

            public:
                Publish(Publisher & publisher, const PrintMux & print, Buffer & buffer,
                        const char * topic, size_t topic_size, size_t payload_size,
                        uint8_t qos = 0, bool retain = false, bool dup = false, uint16_t message_id = 0);

                Publish(Publisher & publisher, const PrintMux & print, Buffer & buffer,
                        const char * topic, size_t payload_size,
                        uint8_t qos = 0, bool retain = false, bool dup = false, uint16_t message_id = 0);

                ~Publish();

                virtual bool send() override;

                const uint8_t qos;
                const uint16_t message_id;
                PrintMux print;
                Publisher & publisher;
        };

        virtual Publish publish(const char * topic, const size_t payload_size,
                                uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) = 0;

        bool publish(const char * topic, const void * payload, const size_t payload_size,
                     uint8_t qos = 0, bool retain = false, uint16_t message_id = 0);

        bool publish(const char * topic, const char * payload,
                     uint8_t qos = 0, bool retain = false, uint16_t message_id = 0);

        bool publish(const String & topic, const String & payload,
                     uint8_t qos = 0, bool retain = false, uint16_t message_id = 0);

        bool publish_P(const char * topic, const void * payload, const size_t payload_size,
                       uint8_t qos = 0, bool retain = false, uint16_t message_id = 0);

        bool publish_P(const char * topic, const char * payload,
                       uint8_t qos = 0, bool retain = false, uint16_t message_id = 0);

    protected:
        virtual bool on_publish_complete(const Publish & publish) { return true; }
};

}
