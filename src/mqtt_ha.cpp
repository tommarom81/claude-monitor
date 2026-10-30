#include "mqtt_ha.h"

#ifdef MQTT_HA_ENABLED
#include "mqtt_local.h"
#include "hal.h"
#include "config.h"
#include "settings.h"
#include "app_state.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// One retained JSON-schema light. Topics hang off claude_monitor/<mac6>/ so two
// boards on one broker never collide; the entity id is pinned with
// default_entity_id so it doesn't follow the header label (dev_name).
//
// State model: s_on + s_pwm. s_pwm is the brightness to come back to (1..255,
// persisted as "bl_pwm"); off only drops the backlight to 0 and never touches
// s_pwm, so ON restores the last level. A reboot always comes up ON — a dark
// panel after a power cut with HA unreachable would look like a dead device.

static WiFiClient    s_net;
static PubSubClient  s_mqtt(s_net);
static char          s_id[7];              // last 3 MAC bytes, hex
static char          s_base[32];           // claude_monitor/<id>
static bool          s_on   = true;
static uint8_t       s_pwm  = 160;
static unsigned long s_lastTryMs = 0;
static unsigned long s_retryMs   = 0;      // 0 = try immediately

static const unsigned long RETRY_MIN_MS = 5000;
static const unsigned long RETRY_MAX_MS = 60000;

static void topic(char* out, size_t n, const char* leaf) {
    snprintf(out, n, "%s/%s", s_base, leaf);
}

static void applyBacklight() {
    halSetBacklightPwm(s_on ? s_pwm : 0);
}

static void savePwm() {
    settingsPutU8("bl_pwm", s_pwm);
}

static void publishState() {
    if (!s_mqtt.connected()) return;
    JsonDocument d;
    d["state"] = s_on ? "ON" : "OFF";
    d["brightness"] = s_pwm;
    d["color_mode"] = "brightness";
    char buf[96], t[48];
    serializeJson(d, buf, sizeof(buf));
    topic(t, sizeof(t), "screen/state");
    s_mqtt.publish(t, buf, true);
}

static void publishDiscovery() {
    char cmd[48], st[48], avail[48], cfg[80], uid[40], devId[32];
    topic(cmd, sizeof(cmd), "screen/set");
    topic(st, sizeof(st), "screen/state");
    topic(avail, sizeof(avail), "status");
    snprintf(cfg, sizeof(cfg), "homeassistant/light/claude_monitor_%s/screen/config", s_id);
    snprintf(uid, sizeof(uid), "claude_monitor_%s_screen", s_id);
    snprintf(devId, sizeof(devId), "claude_monitor_%s", s_id);

    JsonDocument d;
    d["name"] = "Screen";
    d["unique_id"] = uid;
    d["default_entity_id"] = "light.claude_monitor_screen";
    d["schema"] = "json";
    d["command_topic"] = cmd;
    d["state_topic"] = st;
    d["availability_topic"] = avail;
    d["brightness"] = true;
    d["brightness_scale"] = 255;
    d["supported_color_modes"].to<JsonArray>().add("brightness");
    d["icon"] = "mdi:monitor";
    JsonObject dev = d["device"].to<JsonObject>();
    dev["identifiers"].to<JsonArray>().add(devId);
    dev["name"] = "Claude Monitor";
    dev["manufacturer"] = "Guition";
    dev["model"] = "Mini ESP32-S3 3.5\" (JC3248W535)";
    dev["sw_version"] = FW_VERSION;

    String out;
    serializeJson(d, out);
    s_mqtt.publish(cfg, out.c_str(), true);
}

static void onMessage(char* t, byte* payload, unsigned int len) {
    // HA restarted: discovery and state are retained, but republishing keeps the
    // entity correct even if the broker lost its retained store too.
    if (strcmp(t, "homeassistant/status") == 0) {
        if (len == 6 && memcmp(payload, "online", 6) == 0) {
            publishDiscovery();
            publishState();
        }
        return;
    }

    JsonDocument d;
    if (deserializeJson(d, payload, len)) return;

    if (d["brightness"].is<int>()) {
        // HA never sends 0 with ON; clamp anyway so ON can't mean "dark".
        uint8_t b = constrain(d["brightness"].as<int>(), 1, 255);
        if (b != s_pwm) { s_pwm = b; savePwm(); }
    }
    const char* state = d["state"] | "";
    if (strcmp(state, "ON") == 0)  s_on = true;
    if (strcmp(state, "OFF") == 0) s_on = false;

    applyBacklight();
    publishState();
}

static bool connectBroker() {
    char avail[48], cmd[48];
    topic(avail, sizeof(avail), "status");
    topic(cmd, sizeof(cmd), "screen/set");

    char clientId[32];
    snprintf(clientId, sizeof(clientId), "claude-monitor-%s", s_id);
    if (!s_mqtt.connect(clientId, MQTT_USER, MQTT_PASS, avail, 1, true, "offline")) {
        Serial.printf("[MQTT] connect failed, rc=%d\n", s_mqtt.state());
        return false;
    }
    s_mqtt.publish(avail, "online", true);
    publishDiscovery();
    publishState();
    s_mqtt.subscribe(cmd);
    s_mqtt.subscribe("homeassistant/status");
    Serial.println("[MQTT] connected");
    return true;
}

void mqttHaBegin() {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(s_id, sizeof(s_id), "%02x%02x%02x", mac[3], mac[4], mac[5]);
    snprintf(s_base, sizeof(s_base), "claude_monitor/%s", s_id);

    // No saved HA level yet: start from the on-device preset already in use.
    Preferences p;
    p.begin(NVS_NAMESPACE, true);
    s_pwm = p.getUChar("bl_pwm", 0);
    p.end();
    if (s_pwm == 0) s_pwm = halBrightnessPwm(g_settings.brightness);
    s_on = true;
    applyBacklight();

    s_mqtt.setServer(MQTT_HOST, MQTT_PORT);
    s_mqtt.setCallback(onMessage);
    s_mqtt.setBufferSize(768);          // discovery JSON is ~600 bytes; default is 256
    s_mqtt.setSocketTimeout(3);         // a dead broker stalls the UI for 3s, not 15s

#ifdef OTA_PASS
    // Network flashing (pio run -t upload --upload-port <ip>), so later builds
    // don't need the USB cable. Rides on mqtt_local.h because it needs a
    // password that must stay out of the public repo too.
    char host[33];
    slugifyHostname(g_settings.devName, host, sizeof(host));
    ArduinoOTA.setHostname(host);
    ArduinoOTA.setPassword(OTA_PASS);
    ArduinoOTA.begin();
#endif
}

void mqttHaLoop() {
    if (WiFi.status() != WL_CONNECTED) return;
#ifdef OTA_PASS
    ArduinoOTA.handle();
#endif
    if (s_mqtt.connected()) {
        s_mqtt.loop();
        return;
    }
    if (millis() - s_lastTryMs < s_retryMs) return;
    s_lastTryMs = millis();
    if (connectBroker()) {
        s_retryMs = 0;
    } else {
        s_retryMs = s_retryMs ? min(s_retryMs * 2, RETRY_MAX_MS) : RETRY_MIN_MS;
    }
}

bool mqttHaScreenOff() { return !s_on; }

void mqttHaWake() {
    s_on = true;
    applyBacklight();
    publishState();
}

void mqttHaOnPresetBrightness(uint8_t level) {
    s_pwm = halBrightnessPwm(level);
    s_on = true;
    savePwm();
    applyBacklight();
    publishState();
}

#endif // MQTT_HA_ENABLED
