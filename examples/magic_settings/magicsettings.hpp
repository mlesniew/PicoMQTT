#pragma once

#include <set>
#include <PicoMQTT.h>
#include <Preferences.h>

// These methods convert a MQTT message payload string to different types.  The 2nd param must be a non-const reference.
// TODO: Find a way to return the parsed value

static inline void load_from_string(const String & payload, String & value) {
    value = payload;
}

static inline void load_from_string(const String & payload, int & value) {
    value = payload.toInt();
}

static inline void load_from_string(const String & payload, double & value) {
    value = payload.toDouble();
}

static inline void load_from_string(const String & payload, float & value) {
    value = payload.toFloat();
}

static inline void load_from_string(const String & payload, bool & value) {
    value = payload.toInt();
}


// These methods convert different types to a MQTT payload
String store_to_string(const String & value) {
    return value;
}

String store_to_string(const int & value) {
    return String(value);
}

String store_to_string(const double & value) {
    return String(value);
}

String store_to_string(const float & value) {
    return String(value);
}

String store_to_string(const bool & value) {
    return String((int)value);
}

// NVRAM getters
static inline int nvGet(Preferences &p, const String &key, int defaultValue) {
    return  p.getInt(key.c_str(), defaultValue);
}

static inline String nvGet(Preferences &p, const String &key, const String &defaultValue) {
    return p.getString(key.c_str(), defaultValue.c_str());
}

static inline double nvGet(Preferences &p, const String &key, double defaultValue) {
    return  p.getDouble(key.c_str(), defaultValue);
}

static inline float nvGet(Preferences &p, const String &key, float defaultValue) {
    return  p.getFloat(key.c_str(), defaultValue);
}

static inline bool nvGet(Preferences &p, const String &key, bool defaultValue) {
    return  p.getBool(key.c_str(), defaultValue);
}

// NVRAM setters
static inline void nvSet(Preferences &p, const String &key, int value) {
    p.putInt(key.c_str(), value);
}

static inline void nvSet(Preferences &p, const String &key, bool value) {
    p.putBool(key.c_str(), value);
}

static inline void nvSet(Preferences &p, const String &key, float value) {
    p.putFloat(key.c_str(), value);
}

static inline void nvSet(Preferences &p, const String &key, double value) {
    p.putDouble(key.c_str(), value);
}

static inline void nvSet(Preferences &p,const String &key, const String &value) {
    p.putString(key.c_str(), value.c_str());
}


// TODO: Add more load_from_string and store_to_string variants

class MagicSettings {
  public:
    class SettingBase {
      public:
        virtual ~SettingBase() {}

        virtual void begin() = 0;
        virtual void publish() = 0;
        virtual const String &var_name() = 0;
    };

    template <typename T>
    class Setting: public SettingBase {
      public:
        Setting(MagicSettings & ns, const String & name, const T & default_value, std::function<void()> change_callback = nullptr):
            name(name), ns(ns), default_value(default_value), change_callback(change_callback) {
            value = default_value;
            ns.settings.insert(this);
        }

        ~Setting() {
            ns.settings.erase(this);
        }

        virtual void begin() override {
            if (!ns.prefs.isKey(name.c_str())) {
                nvSet(ns.prefs, name, default_value);
                value = default_value;
            } else {
                value = nvGet(ns.prefs, name, value);
            }
            if (change_callback) {
                change_callback();
            }
            ns.mqtt.subscribe(String("preferences/") + ns.name + "/" + name, [this](const String & payload) {
                load_from_string(payload, value);
                if (change_callback) {
                    change_callback();
                }
                nvSet(ns.prefs, name, value);
            });
        }

        virtual void publish() override {
            ns.mqtt.publish(String("preferences/") + ns.name + "/" + name, store_to_string(value));
        }

        const T & get() const {
            return value;
        }

        const T & get_default() const {
            return default_value;
        }

        void set(const T & new_value) {
            if (value != new_value) {
                value = new_value;
                nvSet(ns.prefs, name, value);
                publish();
                if (change_callback) {
                    change_callback();
                }
            }
        }

        // This will allow direct conversion to T
        operator T() const {
            return get();
        }

        // This will allow setting from T objects
        const T & operator=(const T & other) {
            set(other);
            if (change_callback) {
                change_callback();
            }
            return other;
        }

        const String &var_name() {
            return name;
        }

        std::function<void()> change_callback;

        const String name;

      protected:
        MagicSettings & ns;
        T value;
        const T default_value;
    };


    // PicoMQTT::Server  could be replaced by PicoMQTT::Client like so:
    // MagicSettings(PicoMQTT::Client & mqtt, const String name): mqtt(mqtt), name(name) {}
    MagicSettings(PicoMQTT::Server & mqtt, const String name = "default"): mqtt(mqtt), name(name) {}

    ~MagicSettings() {
        prefs.end();
    }

    void defaults() {
        log_i("defaults: %s", name.c_str());
        prefs.clear();
        prefs.end();
        begin();
    }

    // init all settings within this namespace
    void begin() {
        if (!prefs.begin(name.c_str(), false)) {
            log_e("opening namespace '%s' failed", name.c_str());
        };
        for (auto & setting: settings)
            setting->begin();

        // writing any value to preferences/<namespace>/reset will wipe the namespace
        // and set all of its settings to declaration-time defaults
        mqtt.subscribe(String("preferences/") + name + "/reset", [this](const String & payload) {
            defaults();
        });
    }

    // publish everything within this namespace
    void publish() {
        for (auto & setting: settings)
            setting->publish();

        mqtt.publish(String("preferences/") + name + "/reset", "0");
    }

    const String name;

  protected:
    PicoMQTT::Server & mqtt;
    Preferences prefs;

    // TODO: Perhaps a different container would work better?...
    std::set<SettingBase *> settings;
};

