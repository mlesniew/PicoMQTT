#pragma once

#include "config.h"

#ifdef PICOMQTT_DEBUG_TRACE_FUNCTIONS

#include <Arduino.h>

namespace PicoMQTT {

class FunctionTracer {
    public:
        FunctionTracer(const char * function_name) : function_name(function_name) {
            indent(1);
            Serial.print(F("CALL "));
            Serial.println(function_name);
        }

        ~FunctionTracer() {
            indent(-1);
            Serial.print(F("RETURN "));
            Serial.println(function_name);
        }

        const char * const function_name;

    protected:
        void indent(int delta) {
            static int depth = 0;
            if (delta < 0) {
                depth += delta;
            }
            for (int i = 0; i < depth; ++i) {
                Serial.print("  ");
            }
            if (delta > 0) {
                depth += delta;
            }
        }
};

}

#define TRACE_FUNCTION PicoMQTT::FunctionTracer _function_tracer(__PRETTY_FUNCTION__);

#else

#define TRACE_FUNCTION

#endif
