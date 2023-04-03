# PicoMQTT

This is a lightweight and easy to use MQTT library for ESP8266 devices.

Features:
* Client and broker mode supported
* MQTT 3.1.1 implementation
* Allows handling and sending arbitrary sized messages
* High performance -- the broker can deliver thousands of messages per second
* Low memory usage

Limitations:
* Client only supports MQTT QoS levels 0 and 1
* Broker only supports MQTT QoS level 0, ignores will and retained messages.
* Currently only ESP8266-based boards are supported (tested on NodeMCU and Wemos D1 Mini).


## Installation

Once you have the ESP8266 board and toolchain set up, you can install the library by adding it to your Arduino libraries folder.

If you're using PlatformIO, clone this repository into your project's `lib/` subdirectory.


## Quickstart

To get started, try compiling and running the code below or explore [examples](examples).

### Client

```
#include <Arduino.h>
#include <PicoMQTT.h>

PicoMQTT::Client mqtt("broker.hivemq.com");

void setup() {
    // Usual setup
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin("MyWiFi", "password");

    // Subscribe to a topic pattern and attach a callback
    mqtt.subscribe("#", [](const char * topic, const char * payload) {
        Serial.printf("Received message in topic '%s': %s\n", topic, payload);
    });
}

void loop() {
    // This will automatically reconnect the client if needed.  Re-subscribing to topics is never required.
    mqtt.loop();

    if (random(1000) == 0)
        mqtt.publish("picomqtt/welcome", "Hello from PicoMQTT!");
}
```

### Broker

```
#include <Arduino.h>
#include <PicoMQTT.h>

PicoMQTT::Server mqtt;

void setup() {
    // Usual setup
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin("MyWiFi", "password");

    // Subscribe to a topic pattern and attach a callback
    mqtt.subscribe("#", [](const char * topic, const char * payload) {
        Serial.printf("Received message in topic '%s': %s\n", topic, payload);
    });
}

void loop() {
    // This will automatically handle client connections.  By default, all clients are accepted.
    mqtt.loop();

    if (random(1000) == 0)
        mqtt.publish("picomqtt/welcome", "Hello from PicoMQTT!");
}
```

## Publishing messages

To publish messages, the `publish` and `publish_P` methods can be used.  The client and the broker have both the same
API for publishing.

```
#include <PicoMQTT.h>

PicoMQTT::Client mqtt("broker.hivemq.com");  // or PicoMQTT::Server mqtt;

void setup() { /* ... */ }

void loop() {
    mqtt.loop();

    mqtt.publish("picomqtt/simple_publish", "Message");
    mqtt.publish("picomqtt/another_simple_publish", F("Message"));

    const char binary_payload[] = "This string could contain binary data including a zero byte";
    size_t binary_payload_size = strlen(binary_payload);
    mqtt.publish("picomqtt/binary_payload", (const void *) binary_payload, binary_payload_size);
}
```

Notes:
* It's not required to check if the client is connected before publishing.  Calls to `publish()` will have no effect and will return immediately in such cases.
* More examples available [here](examples/advanced_publish/advanced_publish.ino)


## Subscribing and consuming messages

The `subscribe` methods can be used with client and broker to set up callbacks for specific topic patterns.

```
#include <PicoMQTT.h>

PicoMQTT::Client mqtt("broker.hivemq.com");  // or PicoMQTT::Server mqtt;

void setup() {
    /* ... */

    mqtt.subscribe("picomqtt/foo", [](const char * payload) { /* handle message here */ });
    mqtt.subscribe("picomqtt/bar", [](const char * topic, const char * payload) { /* handle message here */ });
    mqtt.subscribe("picomqtt/baz", [](const char * topic, const void * payload, size_t payload_size) { /* handle message here */ });

    // Pattern subscriptions
    mqtt.subscribe("picomqtt/+/foo/#", [](const char * topic, const char * payload) {
        // To extract individual elements from the topic use:
        String wildcard_value = mqtt.get_topic_element(topic, 1);  // second parameter is the index (zero based)
    });
}

void loop() {
    mqtt.loop();
}
```

Notes:
* New subscriptions can be added at any point, not just in the `setup()` function.
* All strings (`const char *` parameters) are guaranteed to have a null terminator.  It's safe to treat them as strings.
* Message payloads can be binary, which means they can contain a zero byte in the middle.  To handle binary data, use a callback with a `size_t` parameter to know the exact size of the message.
* The topic and the payload are both buffers allocated on the stack.  They will become invalid after the callback returns.  If you need to store the payload for later, make sure to copy it to a separate buffer.
* By default, the maximum topic and payload sizes are is 128 and 1024 bytes respectively.  This can be tuned by using `#define` directives to override values from [config.h](src/PicoMQTT/config.h).  Consider using the advanced API described in the later sections to handle bigger messages.
* If a received message's topic matches more than one pattern, then only one of the callbacks will be fired.
* `PicoMQTT::Server` will not deliver published messages locally.  This means that if you set up a `PicoMQTT::Server`
and use `subscribe`, your callback will only be called when messages from clients are received.  Messages published on
the same device will not trigger the callback.
* Try to return from message handlers quickly.  Don't call functions which may block (like reading from serial or network connections), don't use the `delay()` function.
* More examples available [here](examples/advanced_consume/advanced_consume.ino)


## Arbitrary sized messages

It is possible to send and handle messages of arbitrary size, even if they are significantly bigger than the available
memory.

### Publishing

```
auto publish = mqtt.publish(
        "picomqtt/advanced",  // topic
        1000000               // payload size
        );

// The returned publish is a Print subclass, so all Print's functions will work:
publish.println("Hello MQTT");
publish.println(2023, HEX);
publish.write('c');
publish.write((const uint8_t *) "1234567890", 10);

// We can always check how much space is left
size_t remaining_size = publish.get_remaining_size();

// ...

// Once all data is written, we have to send the message
publish.send();
```

### Consuming

```
mqtt.subscribe("picomqtt/advanced", [](const char * topic, PicoMQTT::IncomingPacket & packet) {
    // at any point we can check the remaining payload size
    size_t payload_size = packet.get_remaining_size();

    // packet is a Stram object, so we can use its methods
    int val1 = packet.read();

    char buf[100];
    packet.read(buf, 100);

    // it's OK to not read the whole content of the message
});
```

### Notes

* When consuming or producing a message using the advanced API, don't call other MQTT methods.  Don't try to publish multiple messages at a time or publish a message while consuming another.
* Even with this API, the topic size is still limited.  The limit can be increased by overriding values from [config.h](src/PicoMQTT/config.h).


## License

This library is open-source software licensed under GNU LGPLv3.
