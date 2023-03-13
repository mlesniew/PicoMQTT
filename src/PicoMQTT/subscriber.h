#pragma once

#include <set>

#include <Arduino.h>

namespace PicoMQTT {

class Subscriber {
    public:

        static bool topic_matches(const char * topic_filter, const char * topic);

        void subscribe(const char * topic_filter);
        void unsubscribe(const char * topic_filter);

        const char * get_subscription(const char * topic) const;

    private:
        std::set<String> subscriptions;
};

}
