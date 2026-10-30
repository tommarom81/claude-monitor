#pragma once

// Copy this file to src/mqtt_local.h (gitignored) and fill it in to enable the
// Home Assistant screen control on the mini-s3-35 board (see mqtt_ha.h).
// Use the broker's IP address: the ESP32 client does not resolve .local names.
#define MQTT_HOST  "192.168.1.10"
#define MQTT_PORT  1883
#define MQTT_USER  "mqtt-user"
#define MQTT_PASS  "change-me"

// Optional: enables flashing over WiFi (ArduinoOTA). Leave undefined to keep
// USB-only flashing. Upload with: pio run -e mini-s3-35 -t upload --upload-port <device-ip>
// after setting upload_flags = --auth=<this password> in a local override.
#define OTA_PASS   "change-me-too"
