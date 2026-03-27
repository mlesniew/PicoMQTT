#pragma once

#include <Arduino.h>

#include <functional>

#include "config.h"

namespace PicoMQTT {

class IncomingPacket;

class Subscriber {
protected:
    struct Subscription {
        Subscription(const String & topic) : topic(topic), next(nullptr) {}
        Subscription(const Subscription &) = delete;
        Subscription & operator=(const Subscription &) = delete;

        virtual ~Subscription() {}

        const String topic;
        Subscription * next;
    };

    void insert_subscription(Subscription * subscription);

    Subscription * subscriptions;

public:
    typedef const Subscription * SubscriptionId;

    Subscriber();

    virtual ~Subscriber();

    static bool topic_matches(const char * topic_filter, const char * topic);
    static String get_topic_element(const char * topic, size_t index);
    static String get_topic_element(const String & topic, size_t index);

    const char * get_subscription_pattern(SubscriptionId id) const;

    SubscriptionId get_subscription(const char * topic) const;

    virtual SubscriptionId subscribe(const String & topic_filter) = 0;

    virtual bool unsubscribe(const String & topic_filter);

    virtual bool unsubscribe(SubscriptionId id);
};

class SubscribedMessageListener : public Subscriber {
public:
    // NOTE: None of the callback functions use const arguments for wider
    // compatibility.  It's still OK (and recommended) to use callbacks which
    // take const arguments.  Similarly with Strings.
    typedef std::function<void(char * topic, IncomingPacket & packet)>
        MessageCallback;

    virtual SubscriptionId subscribe(const String & topic_filter) override;
    virtual SubscriptionId subscribe(const String & topic_filter,
                                     MessageCallback callback);

    SubscriptionId subscribe(
        const String & topic_filter,
        std::function<void(char *, void *, size_t)> callback,
        size_t max_size = PICOMQTT_MAX_MESSAGE_SIZE);

    SubscriptionId subscribe(const String & topic_filter,
                             std::function<void(char *, char *)> callback,
                             size_t max_size = PICOMQTT_MAX_MESSAGE_SIZE);
    SubscriptionId subscribe(const String & topic_filter,
                             std::function<void(void *, size_t)> callback,
                             size_t max_size = PICOMQTT_MAX_MESSAGE_SIZE);
    SubscriptionId subscribe(const String & topic_filter,
                             std::function<void(char *)> callback,
                             size_t max_size = PICOMQTT_MAX_MESSAGE_SIZE);

    virtual void on_extra_message(const char * topic, IncomingPacket & packet) {
    }
    virtual void on_message_too_big(const char * topic,
                                    IncomingPacket & packet) {}

protected:
    void fire_message_callbacks(const char * topic, IncomingPacket & packet);

    class SubscriptionWithCallback : public Subscriber::Subscription {
    public:
        SubscriptionWithCallback(const String & topic, MessageCallback callback)
            : Subscription(topic), callback(std::move(callback)) {}

        const MessageCallback callback;
    };
};

}  // namespace PicoMQTT
