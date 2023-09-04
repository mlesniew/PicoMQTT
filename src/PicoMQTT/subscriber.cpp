#include "subscriber.h"
#include "incoming_packet.h"
#include "debug.h"

namespace PicoMQTT {

String Subscriber::get_topic_element(const char * topic, size_t index) {

    while (index && topic[0]) {
        if (topic++[0] == '/') {
            --index;
        }
    }

    if (!topic[0]) {
        return "";
    }

    const char * end = topic;
    while (*end && *end != '/') {
        ++end;
    }

    String ret;
    ret.concat(topic, end - topic);
    return ret;
}

String Subscriber::get_topic_element(const String & topic, size_t index) {
    TRACE_FUNCTION
    return get_topic_element(topic.c_str(), index);
}

bool Subscriber::topic_matches(const char * p, const char * t) {
    TRACE_FUNCTION
    // TODO: Special handling of the $ prefix
    while (true) {
        switch (*p) {
            case '\0':
                // end of pattern reached
                // TODO: check for '/#' suffix
                return (*t == '\0');

            case '#':
                // multilevel wildcard
                if (*t == '\0') {
                    return false;
                }
                return true;

            case '+':
                // single level wildcard
                while (*t && *t != '/') {
                    ++t;
                }
                ++p;
                break;

            default:
                // regular match
                if (*p != *t) {
                    return false;
                }
                ++p;
                ++t;
        }
    }
}

const char * SubscribedMessageListener::get_subscription_pattern(SubscriptionId id) const {
    TRACE_FUNCTION
    for (const auto & kv : subscriptions) {
        if (kv.first.id == id) {
            return kv.first.c_str();
        }
    }
    return nullptr;
}

Subscriber::SubscriptionId SubscribedMessageListener::get_subscription(const char * topic) const {
    TRACE_FUNCTION
    for (const auto & kv : subscriptions) {
        if (topic_matches(kv.first.c_str(), topic)) {
            return kv.first.id;
        }
    }
    return 0;
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(const String & topic_filter) {
    TRACE_FUNCTION
    return subscribe(topic_filter, [this](const char * topic, IncomingPacket & packet) { on_extra_message(topic, packet); });
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(const String & topic_filter, MessageCallback callback) {
    TRACE_FUNCTION
    unsubscribe(topic_filter);
    auto pair = subscriptions.emplace(std::make_pair(Subscription(topic_filter), callback));
    return pair.first->first.id;
}

void SubscribedMessageListener::unsubscribe(const String & topic_filter) {
    TRACE_FUNCTION
    subscriptions.erase(topic_filter);
}

void SubscribedMessageListener::fire_message_callbacks(const char * topic, IncomingPacket & packet) {
    TRACE_FUNCTION
    for (const auto & kv : subscriptions) {
        if (topic_matches(kv.first.c_str(), topic)) {
            kv.second((char *) topic, packet);
            return;
        }
    }
    on_extra_message(topic, packet);
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(const String & topic_filter,
        std::function<void(char *, void *, size_t)> callback, size_t max_size) {
    TRACE_FUNCTION
    return subscribe(topic_filter, [this, callback, max_size](char * topic, IncomingPacket & packet) {
        const size_t payload_size = packet.get_remaining_size();
        if (payload_size >= max_size) {
            on_message_too_big(topic, packet);
            return;
        }
        char payload[payload_size + 1];
        if (packet.read((uint8_t *) payload, payload_size) != (int) payload_size) {
            // connection error, ignore
            return;
        }
        payload[payload_size] = '\0';
        callback(topic, payload, payload_size);
    });
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(const String & topic_filter,
        std::function<void(char *, char *)> callback, size_t max_size) {
    TRACE_FUNCTION
    return subscribe(topic_filter, [callback](char * topic, void * payload, size_t payload_size) {
        callback(topic, (char *) payload);
    });
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(const String & topic_filter,
        std::function<void(char *)> callback, size_t max_size) {
    TRACE_FUNCTION
    return subscribe(topic_filter, [callback](char * topic, void * payload, size_t payload_size) {
        callback((char *) payload);
    });
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(const String & topic_filter,
        std::function<void(void *, size_t)> callback, size_t max_size) {
    TRACE_FUNCTION
    return subscribe(topic_filter, [callback](char * topic, void * payload, size_t payload_size) {
        callback(payload, payload_size);
    });
}

};
