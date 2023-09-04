#pragma once

#include <functional>
#include <map>

#include <Arduino.h>

#include "autoid.h"
#include "config.h"

namespace PicoMQTT {

class IncomingPacket;

class Subscriber {
    public:
        typedef AutoId::Id SubscriptionId;

        static bool topic_matches(const char * topic_filter, const char * topic);
        static String get_topic_element(const char * topic, size_t index);
        static String get_topic_element(const String & topic, size_t index);

        virtual const char * get_subscription_pattern(SubscriptionId id) const = 0;
        virtual SubscriptionId get_subscription(const char * topic) const = 0;

        virtual SubscriptionId subscribe(const String & topic_filter) = 0;

        virtual void unsubscribe(const String & topic_filter) = 0;
        void unsubscribe(SubscriptionId id) { unsubscribe(get_subscription_pattern(id)); }

    protected:
        class Subscription: public String, public AutoId {
            public:
                using String::String;
                Subscription(const String & str): Subscription(str.c_str()) {}
        };

};

class SubscribedMessageListener: public Subscriber {
    public:
        // NOTE: None of the callback functions use const arguments for wider compatibility.  It's still OK (and
        // recommended) to use callbacks which take const arguments.  Similarly with Strings.
        typedef std::function<void(char * topic, IncomingPacket & packet)> MessageCallback;

        virtual const char * get_subscription_pattern(SubscriptionId id) const override;
        virtual SubscriptionId get_subscription(const char * topic) const override;

        virtual SubscriptionId subscribe(const String & topic_filter) override;
        virtual SubscriptionId subscribe(const String & topic_filter, MessageCallback callback);

        SubscriptionId subscribe(const String & topic_filter, std::function<void(char *, void *, size_t)> callback,
                                 size_t max_size = PICOMQTT_MAX_MESSAGE_SIZE);

        SubscriptionId subscribe(const String & topic_filter, std::function<void(char *, char *)> callback,
                                 size_t max_size = PICOMQTT_MAX_MESSAGE_SIZE);
        SubscriptionId subscribe(const String & topic_filter, std::function<void(void *, size_t)> callback,
                                 size_t max_size = PICOMQTT_MAX_MESSAGE_SIZE);
        SubscriptionId subscribe(const String & topic_filter, std::function<void(char *)> callback,
                                 size_t max_size = PICOMQTT_MAX_MESSAGE_SIZE);

        virtual void unsubscribe(const String & topic_filter) override;

        virtual void on_extra_message(const char * topic, IncomingPacket & packet) {}
        virtual void on_message_too_big(const char * topic, IncomingPacket & packet) {}

    protected:
        void fire_message_callbacks(const char * topic, IncomingPacket & packet);

        std::map<Subscription, MessageCallback> subscriptions;
};

}
