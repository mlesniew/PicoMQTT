#pragma once

#include "server.h"
#include <Arduino.h>
#include <functional>
#include <unordered_map>
#include <vector>

namespace std {
template <>
struct hash<String> {
    size_t operator()(const String& key) const {
        return hash<std::string>{}(std::string(key.c_str(), key.length()));
    }
};
}

namespace PicoMQTT {

template <typename BaseServer>
class RetainedMessagesServer : public BaseServer {
public:
    using BaseServer::BaseServer;

    // Override publish to capture retained messages from local publishes
    using BaseServer::publish;
    bool publish(const char * topic, const void * payload, const size_t payload_size,
                 uint8_t qos = 0, bool retain = false, uint16_t message_id = 0) override {
        TRACE_FUNCTION
        if (retain) {
            if (payload_size == 0) {
                // Empty payload clears the retained message
                retained_messages.erase(topic);
            } else {
                // Store the retained message
                auto data = static_cast<const uint8_t *>(payload);
                std::vector<uint8_t> payload_vec(data, data + payload_size);
                retained_messages.insert_or_assign(topic, RetainedMessage(std::move(payload_vec), qos));
            }
        }
        return BaseServer::publish(topic, payload, payload_size, qos, retain, message_id);
    }

    // Override subscribe to deliver retained messages for local subscriptions
    // This works with ServerLocalSubscribe which fires callbacks via publish()
    using BaseServer::subscribe;
    Subscriber::SubscriptionId subscribe(const String & topic_filter, typename SubscribedMessageListener::MessageCallback callback) override {
        auto id = BaseServer::subscribe(topic_filter, callback);

        // Deliver retained messages matching this subscription via publish()
        // For ServerLocalSubscribe, this will fire the local callback
        // For plain Server, this sends to network clients (local callbacks don't work anyway)
        for (const auto & retained : retained_messages) {
            const char * ret_topic = retained.first.c_str();
            const auto & retained_msg = retained.second;
            if (Subscriber::topic_matches(topic_filter.c_str(), ret_topic)) {
                BaseServer::publish(ret_topic, (const void*)retained_msg.payload.data(), retained_msg.payload.size(), (uint8_t)0, true, (uint16_t)0);
            }
        }

        return id;
    }

    // Retained message struct
    struct RetainedMessage {
        RetainedMessage() = delete;
        RetainedMessage(std::vector<uint8_t> && payload_data, const uint8_t qos_level)
            : payload(std::move(payload_data)), qos(qos_level) {}

        std::vector<uint8_t> payload;
        uint8_t qos;
    };

    std::unordered_map<String, RetainedMessage> retained_messages; // topic → retained message

    // Client class that has access to retained messages
    class Client: public BaseServer::Client {
    public:
        Client(RetainedMessagesServer & server, ::Client * client)
            : BaseServer::Client(server, client), retained_server(server) {
            TRACE_FUNCTION
        }

        // Wrapper that captures payload while passing it through
        class CapturingIncomingPublish : public IncomingPacket {
        public:
            CapturingIncomingPublish(IncomingPacket & packet, Publisher::Publish & publish, std::vector<uint8_t> & capture_buffer)
                : IncomingPacket(std::move(packet)), publish(publish), capture_buffer(capture_buffer) {
                TRACE_FUNCTION
                capture_buffer.clear();
                capture_buffer.reserve(get_remaining_size());
            }

            ~CapturingIncomingPublish() noexcept {
                TRACE_FUNCTION
                // Copy any remaining bytes
                const size_t remaining = get_remaining_size();
                for (size_t i = 0; i < remaining; ++i) {
                    const int byte = IncomingPacket::read();
                    if (byte >= 0) {
                        capture_buffer.push_back(static_cast<uint8_t>(byte));
                        publish.write(static_cast<uint8_t>(byte));
                    }
                }
            }

            int read(uint8_t * buf, size_t size) override {
                TRACE_FUNCTION
                const int ret = IncomingPacket::read(buf, size);
                if (ret > 0) {
                    capture_buffer.insert(capture_buffer.end(), buf, buf + ret);
                    publish.write(buf, ret);
                }
                return ret;
            }

            int read() override {
                TRACE_FUNCTION
                const int ret = IncomingPacket::read();
                if (ret >= 0) {
                    capture_buffer.push_back(static_cast<uint8_t>(ret));
                    publish.write(static_cast<uint8_t>(ret));
                }
                return ret;
            }

        private:
            Publisher::Publish & publish;
            std::vector<uint8_t> & capture_buffer;
        };

        // Override on_message to capture retained message payloads from clients
        void on_message(const char * topic, IncomingPacket & packet) override {
            TRACE_FUNCTION
            const uint8_t flags = packet.get_flags();
            const bool retain = flags & 0b1;
            const uint8_t qos = (flags >> 1) & 0b11;
            const size_t payload_size = packet.get_remaining_size();
            auto publish = this->server.begin_publish(topic, payload_size, qos, retain);

            if (retain) {
                // Use capturing wrapper to intercept the payload
                std::vector<uint8_t> captured_payload;
                {
                    CapturingIncomingPublish capturing_packet(packet, publish, captured_payload);
                    retained_server.on_message(topic, capturing_packet);
                }

                // Store or clear the retained message
                if (captured_payload.empty()) {
                    retained_server.retained_messages.erase(topic);
                } else {
                    retained_server.retained_messages.insert_or_assign(
                        topic,
                        RetainedMessage(std::move(captured_payload), qos)
                    );
                }
            } else {
                // Non-retained message, use the standard wrapper from base class
                typename BaseServer::IncomingPublish incoming_publish(packet, publish);
                retained_server.on_message(topic, incoming_publish);
            }

            publish.send();
        }

        // Override on_subscribed to send retained messages on subscribe
        void on_subscribed(const String & topic_filter) override {
            TRACE_FUNCTION
            BaseServer::Client::on_subscribed(topic_filter);

            // After subscribing, send retained messages for matched topics
            // Unlike other brokers, we leave deduplicating messages for overlapping
            // subscriptions to the client and send one message back for every subscription.
            // Both behaviors are spec-compliant.
            for (const auto & retained : retained_server.retained_messages) {
                const char * ret_topic = retained.first.c_str();
                const auto & retained_msg = retained.second;
                if (this->topic_matches(topic_filter.c_str(), ret_topic)) {
                    // Create a publish going only to this specific client
                    auto pub = Publisher::Publish(retained_server, PrintMux(this->get_print()), ret_topic,
                                                   retained_msg.payload.size(), 0, true, 0);
                    pub.write(retained_msg.payload.data(), retained_msg.payload.size());
                    pub.send();
                }
            }
        }

    private:
        RetainedMessagesServer & retained_server;
    };

    // Override loop() to create our custom Client instances
    void loop() override {
        TRACE_FUNCTION

        ::Client * client_ptr = this->server->accept_client();
        if (client_ptr) {
            this->clients.push_back(std::unique_ptr<typename BaseServer::Client>(new Client(*this, client_ptr)));
            this->on_connected(this->clients.back()->get_client_id());
        }

        for (auto it = this->clients.begin(); it != this->clients.end();) {
            auto & client = **it;
            client.loop();

            if (!client.connected()) {
                this->on_disconnected(client.get_client_id());
                this->clients.erase(it++);
            } else {
                ++it;
            }
        }
    }
};

} // namespace PicoMQTT
