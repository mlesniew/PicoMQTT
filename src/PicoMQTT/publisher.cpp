#include "publisher.h"
#include "debug.h"

namespace PicoMQTT {

Publisher::Publish::Publish(Publisher & publisher, const PrintMux & print, Buffer & buffer,
                            uint8_t flags, size_t total_size,
                            const char * topic, size_t topic_size,
                            uint16_t message_id)
    :
    OutgoingPacket(this->print, buffer, Packet::PUBLISH, flags, total_size),
    qos((flags >> 1) & 0b11),
    message_id(message_id),
    print(print),
    publisher(publisher) {
    TRACE_FUNCTION

    OutgoingPacket::write_header();
    write_string(topic, topic_size);
    if (qos) {
        write_u16(message_id);
    }
}

Publisher::Publish::Publish(Publisher & publisher, const PrintMux & print, Buffer & buffer,
                            const char * topic, size_t topic_size, size_t payload_size,
                            uint8_t qos, bool retain, bool dup, uint16_t message_id)
    : Publish(
          publisher, print, buffer,
          (dup ? 0b1000 : 0) | ((qos & 0b11) << 1) | (retain ? 1 : 0),  // flags
          2 + topic_size + (qos ? 2 : 0) + payload_size,  // total size
          topic, topic_size,  // topic
          message_id) {
    TRACE_FUNCTION
}

Publisher::Publish::Publish(Publisher & publisher, const PrintMux & print, Buffer & buffer,
                            const char * topic, size_t payload_size,
                            uint8_t qos, bool retain, bool dup, uint16_t message_id)
    : Publish(
          publisher, print, buffer,
          topic, strlen(topic), payload_size,
          qos, retain, dup, message_id) {
    TRACE_FUNCTION
}

bool Publisher::Publish::send() {
    TRACE_FUNCTION
    return OutgoingPacket::send() && publisher.on_publish_complete(*this);
}

bool Publisher::publish(const char * topic, const void * payload, const size_t payload_size,
                        uint8_t qos, bool retain, uint16_t message_id) {
    TRACE_FUNCTION
    auto packet = publish(topic, payload_size, qos, retain, message_id);
    packet.write(payload, payload_size);
    return packet.send();
}

bool Publisher::publish(const char * topic, const char * payload,
                        uint8_t qos, bool retain, uint16_t message_id) {
    TRACE_FUNCTION
    return publish(topic, payload, strlen(payload), qos, retain, message_id);
}

bool Publisher::publish(const String & topic, const String & payload,
                        uint8_t qos, bool retain, uint16_t message_id) {
    TRACE_FUNCTION
    return publish(topic.c_str(), payload.c_str(), qos, retain, message_id);
}

bool Publisher::publish_P(const char * topic, const void * payload, const size_t payload_size,
                          uint8_t qos, bool retain, uint16_t message_id) {
    TRACE_FUNCTION;
    auto packet = publish(topic, payload_size, qos, retain, message_id);
    packet.write_P(payload, payload_size);
    return packet.send();
}

bool Publisher::publish_P(const char * topic, const char * payload,
                          uint8_t qos, bool retain, uint16_t message_id) {
    TRACE_FUNCTION
    return publish_P(topic, payload, strlen_P(payload), qos, retain, message_id);
}

}
