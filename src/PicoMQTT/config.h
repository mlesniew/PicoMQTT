#pragma once

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

// #define PICOMQTT_DEBUG

// #define PICOMQTT_DEBUG_TRACE_FUNCTIONS
