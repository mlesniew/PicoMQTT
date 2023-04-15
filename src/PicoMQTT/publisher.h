#pragma once

#include <cstring>

#include <Arduino.h>

#include "debug.h"
#include "outgoing_packet.h"
#include "print_mux.h"

namespace PicoMQTT {

class Publisher {
    public:
        class Publish: public OutgoingPacket {
            private:
                Publish(Publisher & publisher, const PrintMux & print,
                        uint8_t flags, size_t total_size,
                        const char * topic, size_t topic_size,
                        uint16_t message_id);

            public:
                Publish(Publisher & publisher, const PrintMux & print,
                        const char * topic, size_t topic_size, size_t payload_size,
                        uint8_t qos = 0, bool retain = false, bool dup = false, uint16_t message_id = 0);

                Publish(Publisher & publisher, const PrintMux & print,
                        const char * topic, size_t payload_size,
                        uint8_t qos = 0, bool retain = false, bool dup = false, uint16_t message_id = 0);

                ~Publish();

                virtual bool send() override;

                const uint8_t qos;
                const uint16_t message_id;
                PrintMux print;
                Publisher & publisher;
        };

        virtual Publish begin_publish(const char * topic, const size_t payload_size,
                                      uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) = 0;

        Publish begin_publish(const String & topic, const size_t payload_size,
                              uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) {
            return begin_publish(topic.c_str(), payload_size, qos, retain, message_id);
        }

        template <typename TopicStringType>
        bool publish(TopicStringType topic, const void * payload, const size_t payload_size,
                     uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) {
            TRACE_FUNCTION
            auto packet = begin_publish(get_c_str(topic), payload_size, qos, retain, message_id);
            packet.write((const uint8_t *) payload, payload_size);
            return packet.send();
        }

        template <typename TopicStringType>
        bool publish_P(TopicStringType topic, PGM_P payload, const size_t payload_size,
                       uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) {
            TRACE_FUNCTION;
            auto packet = begin_publish(get_c_str(topic), payload_size, qos, retain, message_id);
            packet.write_P(payload, payload_size);
            return packet.send();
        }

        template <typename TopicStringType, typename PayloadStringType>
        bool publish(TopicStringType topic, PayloadStringType payload,
                     uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) {
            return publish(topic, (const void *) get_c_str(payload), get_c_str_len(payload),
                           qos, retain, message_id);
        }

        template <typename TopicStringType>
        bool publish_P(TopicStringType topic, PGM_P payload,
                       uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) {
            return publish_P(topic, payload, strlen_P(payload),
                             qos, retain, message_id);
        }

    protected:
        virtual bool on_publish_complete(const Publish & publish) { return true; }

        static const char * get_c_str(const char * string) { return string; }
        static const char * get_c_str(const String & string) { return string.c_str(); }
        static size_t get_c_str_len(const char * string) { return strlen(string); }
        static size_t get_c_str_len(const String & string) { return string.length(); }
};

}
