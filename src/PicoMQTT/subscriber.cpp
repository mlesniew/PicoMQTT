#include "subscriber.h"

#include "debug.h"
#include "incoming_packet.h"

namespace PicoMQTT {

Subscriber::Subscriber() : subscriptions(nullptr) {}

Subscriber::~Subscriber() {
    Subscription * current = subscriptions;
    while (current) {
        Subscription * next = current->next;
        delete current;
        current = next;
    }
}

void Subscriber::insert_subscription(Subscription * subscription) {
    subscription->next = subscriptions;
    subscriptions = subscription;
}

const char * Subscriber::get_subscription_pattern(SubscriptionId id) const {
    return id ? id->topic.c_str() : nullptr;
}

Subscriber::SubscriptionId Subscriber::get_subscription(
    const char * topic) const {
    for (const Subscription * s = subscriptions; s; s = s->next) {
        if (topic_matches(s->topic.c_str(), topic)) {
            return s;
        }
    }
    return nullptr;
}

bool Subscriber::unsubscribe(const String & topic_filter) {
    Subscription ** current = &subscriptions;
    while (*current) {
        if ((*current)->topic == topic_filter) {
            Subscription * to_delete = *current;
            *current = to_delete->next;
            delete to_delete;
            return true;
        }
        current = &(*current)->next;
    }
    return false;
}

bool Subscriber::unsubscribe(SubscriptionId id) {
    if (!id) {
        return false;
    }
    Subscription ** current = &subscriptions;
    while (*current) {
        if (*current == id) {
            Subscription * to_delete = *current;
            *current = to_delete->next;
            delete to_delete;
            return true;
        }
        current = &(*current)->next;
    }
    return false;
}

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
    TRACE_FUNCTION;
    return get_topic_element(topic.c_str(), index);
}

bool Subscriber::topic_matches(const char * p, const char * t) {
    while (true) {
        if (*p == '#') {
            return true;  // valid filter => '#' is terminal and matches
                          // remainder
        }

        if (*p == '\0') {
            return *t == '\0';
        }

        if (*t == '\0') {
            // allow parent-topic match: "home/#" matches "home"
            return p[0] == '/' && p[1] == '#' && p[2] == '\0';
        }

        if (*p == '+') {
            while (*t && *t != '/') {
                ++t;
            }
            ++p;
            continue;
        }

        if (*p != *t) {
            return false;
        }

        ++p;
        ++t;
    }
}

bool Subscriber::is_valid_topic_filter(const char * topic_filter) {
    TRACE_FUNCTION;
    if (!topic_filter || topic_filter[0] == '\0') {
        return false;
    }

    enum class State {
        segment_start,
        segment_cont,
        separator,
        end,
    } state = State::segment_start;

    for (;; ++topic_filter) {
        switch (state) {
            case State::segment_start:
                switch (*topic_filter) {
                    case '+':
                        state = State::separator;
                        break;
                    case '#':
                        state = State::end;
                        break;
                    case '/':
                        // empty topic segment
                        state = State::segment_start;
                        break;
                    case '\0':
                        return true;
                    default:
                        state = State::segment_cont;
                }
                break;
            case State::segment_cont:
                switch (*topic_filter) {
                    case '/':
                        state = State::segment_start;
                        break;
                    case '\0':
                        return true;
                    case '+':
                    case '#':
                        // wildcards must occupy entire topic segment
                        return false;
                    default:
                        break;
                }
                break;
            case State::separator:
                switch (*topic_filter) {
                    case '/':
                        state = State::segment_start;
                        break;
                    case '\0':
                        return true;
                    default:
                        // single-level wildcard must be followed by topic
                        // separator
                        return false;
                }
                break;
            case State::end:
                return *topic_filter == '\0';
        }
    }
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(
    const String & topic_filter) {
    TRACE_FUNCTION;
    return subscribe(topic_filter,
                     [this](const char * topic, IncomingPacket & packet) {
                         on_extra_message(topic, packet);
                     });
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(
    const String & topic_filter, MessageCallback callback) {
    TRACE_FUNCTION;

    if (!is_valid_topic_filter(topic_filter.c_str())) {
        return nullptr;
    }

    unsubscribe(topic_filter);

    SubscriptionWithCallback * node =
        new SubscriptionWithCallback(topic_filter, std::move(callback));

    insert_subscription(node);
    return node;
}

void SubscribedMessageListener::fire_message_callbacks(
    const char * topic, IncomingPacket & packet) {
    TRACE_FUNCTION;
    for (Subscription * s = subscriptions; s; s = s->next) {
        if (topic_matches(s->topic.c_str(), topic)) {
            static_cast<SubscriptionWithCallback *>(s)->callback(
                const_cast<char *>(topic), packet);
            return;
        }
    }
    on_extra_message(topic, packet);
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(
    const String & topic_filter,
    std::function<void(char *, void *, size_t)> callback, size_t max_size) {
    TRACE_FUNCTION;
    return subscribe(topic_filter, [this, callback, max_size](
                                       char * topic, IncomingPacket & packet) {
        const size_t payload_size = packet.get_remaining_size();
        if (payload_size >= max_size) {
            on_message_too_big(topic, packet);
            return;
        }
        char payload[payload_size + 1];
        if (packet.read((uint8_t *)payload, payload_size) !=
            (int)payload_size) {
            // connection error, ignore
            return;
        }
        payload[payload_size] = '\0';
        callback(topic, payload, payload_size);
    });
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(
    const String & topic_filter, std::function<void(char *, char *)> callback,
    size_t max_size) {
    TRACE_FUNCTION;
    return subscribe(
        topic_filter,
        [callback](char * topic, void * payload, size_t payload_size) {
            callback(topic, (char *)payload);
        },
        max_size);
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(
    const String & topic_filter, std::function<void(char *)> callback,
    size_t max_size) {
    TRACE_FUNCTION;
    return subscribe(
        topic_filter,
        [callback](char * topic, void * payload, size_t payload_size) {
            callback((char *)payload);
        },
        max_size);
}

Subscriber::SubscriptionId SubscribedMessageListener::subscribe(
    const String & topic_filter, std::function<void(void *, size_t)> callback,
    size_t max_size) {
    TRACE_FUNCTION;
    return subscribe(
        topic_filter,
        [callback](char * topic, void * payload, size_t payload_size) {
            callback(payload, payload_size);
        },
        max_size);
}

}  // namespace PicoMQTT
