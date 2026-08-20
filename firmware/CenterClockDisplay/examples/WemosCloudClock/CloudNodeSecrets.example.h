#pragma once

// Author: Dr. Alan Wong (https://github.com/geeksloth)
//
// Local Docker Compose development settings. Replace the host with the IP
// address of the computer running CenterClock. Do not use .local unless mDNS
// resolution has been confirmed on the ESP8266.
#define CENTER_CLOCK_MQTT_HOST "192.168.1.10"
#define CENTER_CLOCK_MQTT_PORT 1883
#define CENTER_CLOCK_MQTT_USERNAME "bootstrap"
#define CENTER_CLOCK_MQTT_PASSWORD "replace-with-the-value-from-dot-env"
#define CENTER_CLOCK_MQTT_USE_TLS 0

// Production MQTT uses TLS with the broker public key pinned. Supply the PEM
// public key, not the private key and not the complete certificate.
// #define CENTER_CLOCK_MQTT_PORT 8883
// #define CENTER_CLOCK_MQTT_USE_TLS 1
// #define CENTER_CLOCK_MQTT_SERVER_PUBLIC_KEY_PEM R"KEY(
// -----BEGIN PUBLIC KEY-----
// ...
// -----END PUBLIC KEY-----
// )KEY"

// OTA requires two independent public keys: one pins the HTTPS server and one
// verifies the signature appended to the ESP8266 firmware binary.
// #define CENTER_CLOCK_FIRMWARE_SERVER_PUBLIC_KEY_PEM R"KEY(
// -----BEGIN PUBLIC KEY-----
// ...
// -----END PUBLIC KEY-----
// )KEY"
// #define CENTER_CLOCK_OTA_SIGNING_PUBLIC_KEY_PEM R"KEY(
// -----BEGIN PUBLIC KEY-----
// ...
// -----END PUBLIC KEY-----
// )KEY"

// Use a unique 8-63 character value printed on each production device.
#define CENTER_CLOCK_PORTAL_PASSWORD "replace-this-portal-password"

#define CENTER_CLOCK_FIRMWARE_VERSION "0.1.0"
