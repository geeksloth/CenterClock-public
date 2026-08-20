#pragma once

/*
 * Copy CloudNodeSecrets.example.h to CloudNodeSecrets.h and change the
 * deployment-specific values. CloudNodeSecrets.h is ignored by Git.
 *
 * Author: Dr. Alan Wong (https://github.com/geeksloth)
 */
#if __has_include("CloudNodeSecrets.h")
#include "CloudNodeSecrets.h"
#endif

#ifndef CENTER_CLOCK_MQTT_HOST
#define CENTER_CLOCK_MQTT_HOST "centerclock.local"
#endif

#ifndef CENTER_CLOCK_MQTT_PORT
#define CENTER_CLOCK_MQTT_PORT 1883
#endif

#ifndef CENTER_CLOCK_MQTT_USERNAME
#define CENTER_CLOCK_MQTT_USERNAME "bootstrap"
#endif

#ifndef CENTER_CLOCK_MQTT_PASSWORD
#define CENTER_CLOCK_MQTT_PASSWORD "change-this-bootstrap-password"
#endif

#ifndef CENTER_CLOCK_MQTT_USE_TLS
#define CENTER_CLOCK_MQTT_USE_TLS 0
#endif

#ifndef CENTER_CLOCK_MQTT_SERVER_PUBLIC_KEY_PEM
#define CENTER_CLOCK_MQTT_SERVER_PUBLIC_KEY_PEM ""
#endif

#ifndef CENTER_CLOCK_FIRMWARE_SERVER_PUBLIC_KEY_PEM
#define CENTER_CLOCK_FIRMWARE_SERVER_PUBLIC_KEY_PEM ""
#endif

#ifndef CENTER_CLOCK_OTA_SIGNING_PUBLIC_KEY_PEM
#define CENTER_CLOCK_OTA_SIGNING_PUBLIC_KEY_PEM ""
#endif

#ifndef CENTER_CLOCK_PORTAL_PASSWORD
#define CENTER_CLOCK_PORTAL_PASSWORD "configure-clock"
#endif

#ifndef CENTER_CLOCK_FIRMWARE_VERSION
#define CENTER_CLOCK_FIRMWARE_VERSION "0.1.0-dev"
#endif

#ifndef CENTER_CLOCK_HARDWARE_TARGET
#define CENTER_CLOCK_HARDWARE_TARGET "centerclock-wemos-d1-mini"
#endif

namespace CloudNodeSettings {

static const char MQTT_HOST[] = CENTER_CLOCK_MQTT_HOST;
static const uint16_t MQTT_PORT = CENTER_CLOCK_MQTT_PORT;
static const char MQTT_USERNAME[] = CENTER_CLOCK_MQTT_USERNAME;
static const char MQTT_PASSWORD[] = CENTER_CLOCK_MQTT_PASSWORD;
static const bool MQTT_USE_TLS = CENTER_CLOCK_MQTT_USE_TLS != 0;

// Pinning the server public key avoids requiring an independent NTP source
// before the MQTT TLS connection can be verified.
static const char MQTT_SERVER_PUBLIC_KEY_PEM[] =
    CENTER_CLOCK_MQTT_SERVER_PUBLIC_KEY_PEM;
static const char FIRMWARE_SERVER_PUBLIC_KEY_PEM[] =
    CENTER_CLOCK_FIRMWARE_SERVER_PUBLIC_KEY_PEM;
static const char OTA_SIGNING_PUBLIC_KEY_PEM[] =
    CENTER_CLOCK_OTA_SIGNING_PUBLIC_KEY_PEM;

static const char PORTAL_PASSWORD[] = CENTER_CLOCK_PORTAL_PASSWORD;
static const char FIRMWARE_VERSION[] = CENTER_CLOCK_FIRMWARE_VERSION;
static const char HARDWARE_TARGET[] = CENTER_CLOCK_HARDWARE_TARGET;

static const uint32_t MQTT_RETRY_MS = 5000;
static const uint32_t WIFI_RETRY_MS = 30000;
static const uint32_t ENROLL_INTERVAL_MS = 60000;
static const uint32_t TIME_SYNC_INTERVAL_MS = 60000;
static const uint32_t STATUS_INTERVAL_MS = 30000;
static const uint32_t DISPLAY_INTERVAL_MS = 200;
static const uint16_t CONFIG_PORTAL_TIMEOUT_SECONDS = 300;
static const uint16_t MQTT_PAYLOAD_LIMIT = 2048;

}  // namespace CloudNodeSettings
