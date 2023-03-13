#include "subscriber.h"
#include "debug.h"

namespace PicoMQTT {

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

void Subscriber::subscribe(const char * topic_filter) {
    TRACE_FUNCTION
    subscriptions.insert(topic_filter);
}

void Subscriber::unsubscribe(const char * topic_filter) {
    TRACE_FUNCTION
    subscriptions.erase(topic_filter);
}

const char * Subscriber::get_subscription(const char * topic) const {
    TRACE_FUNCTION
    for (const auto & pattern : subscriptions)
        if (topic_matches(pattern.c_str(), topic)) {
            return pattern.c_str();
        }
    return nullptr;
}

};
