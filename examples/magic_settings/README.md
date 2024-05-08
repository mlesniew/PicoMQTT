# MagicSettings
This is an example how to use PicoMQTT together with the [ESP32 Preferences](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html) library for retrieving and changing configuration values backed by non-volatile storage.

## Example
An instance of this class associates an MQTT server with a Preferences namespace,
exporting variables in this namespace to topics like `preferences/<namespace>/<variable>`


````c++

// keep variables under the "testns" namespace:
MagicSettings settings(mqtt, "testns");

// this variable will appear under the topic preferences/testns/bar
MagicSettings::Setting<int> bar(settings, "bar", 42);

````

## Setting a new value via assignment

To permanently change a Settings variable, just assign its value - the new value will be recorded in NVS:

````c++
bar = 4711;
````
On next reboot, `bar` will be set to 4711.

## Setting a value via MQTT
To change the value of the `bar` setting, publish a new value to `preferences/testns/bar`.

The new value will be stored permanently in NVS, hence will retain the new value after a reboot.

## Inspecting values via MQTT
For conveniently making all variables and their values visible via MQTT, periodically call `settings.publish()` .

## Getting a value
A Setting can be used like a variable of the underlying type, for example:

````c++
Serial.printf("bar = %d\n", bar);
````

## Resetting all variables to their compile-time defaults

To reset all variables in a namespace to their compile-time defaults, use the `defaults()` method:

````c++
setting.defaults();
````
After reboot, `bar` will again have the value 42.

## Resetting all variables in a namespace via MQTT
To achieve the same effect via MQTT, publish any value to the special topic `preferences/<namespace>/reset` .

With this example, the reset topic would be `preferences/testns/reset` .

## Value change notification

A value change callback may be associate with a setting, to cause some action in user code.

Example  using a lambda:
````c++
MagicSettings::Setting<bool> flag(settings, "flag", true, [] {
    log_i("flag=%d", flag.get());
});
````
Example using a callback function:

````c++
void on_fparam_change(void) {
    log_i("fparam changed to %f, default value: %f",
          fparam.get(), fparam.get_default());
}

MagicSettings::Setting<float> fparam(settings, "fparam", 2.718, on_fparam_change);
````


