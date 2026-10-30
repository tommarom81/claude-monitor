#pragma once
#include <stdint.h>

// Home Assistant control of the mini-s3-35 backlight over MQTT: the screen shows
// up in HA (MQTT discovery) as a light with on/off and a 0..255 brightness
// slider. Enabled only on BOARD_MINI_S3_35 and only when src/mqtt_local.h exists
// — that file holds the broker address and login, is gitignored, and is copied
// from src/mqtt_local.example.h. Without it every call below is a no-op. The
// same file can also turn on network (OTA) flashing — see OTA_PASS there.
#if defined(BOARD_MINI_S3_35) && __has_include("mqtt_local.h")
#define MQTT_HA_ENABLED 1

void mqttHaBegin();                       // after WiFi is up; applies the saved brightness
void mqttHaLoop();                        // per-loop: reconnect (backed off) + client.loop()
bool mqttHaScreenOff();                   // HA switched the backlight fully off
void mqttHaWake();                        // a tap while off: back on, HA told
void mqttHaOnPresetBrightness(uint8_t level);   // on-device long-press cycled a preset

#else

inline void mqttHaBegin() {}
inline void mqttHaLoop() {}
inline bool mqttHaScreenOff() { return false; }
inline void mqttHaWake() {}
inline void mqttHaOnPresetBrightness(uint8_t) {}

#endif
