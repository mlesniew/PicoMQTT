#pragma once

#include <Arduino.h>

#ifndef PICOMQTT_MAX_TOPIC_SIZE
#define PICOMQTT_MAX_TOPIC_SIZE 256
#endif

#ifndef PICOMQTT_MAX_MESSAGE_SIZE
#define PICOMQTT_MAX_MESSAGE_SIZE 1024
#endif

#ifndef PICOMQTT_MAX_CLIENT_ID_SIZE
/*
 * The MQTT standard requires brokers to accept client ids that are
 * 1-23 chars long, but allows longer client IDs to be accepted too.
 */
#define PICOMQTT_MAX_CLIENT_ID_SIZE 64
#endif

#ifndef PICOMQTT_MAX_USERPASS_SIZE
#define PICOMQTT_MAX_USERPASS_SIZE 256
#endif

#ifndef PICOMQTT_OUTGOING_BUFFER_SIZE
#define PICOMQTT_OUTGOING_BUFFER_SIZE 128
#endif

#ifdef ESP32
// Uncomment this define to make PicoMQTT compatible with framework variants
// which have extra Client::connect methods which accept a timeout parameter.
// #define PICOMQTT_EXTRA_CONNECT_METHODS
#endif

// #define PICOMQTT_DEBUG

// #define PICOMQTT_DEBUG_TRACE_FUNCTIONS
