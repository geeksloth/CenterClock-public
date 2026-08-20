/*
 * CenterClock cloud node for WeMos D1 mini (ESP8266).
 *
 * Display wiring:
 *   D7 -> SDI
 *   D5 -> CLK
 *   D2 -> LE
 *   D1 -> OE
 *
 * Old-MCU pads 17 and 20 are assumed to be permanently enabled in hardware.
 *
 * Author: Dr. Alan Wong (https://github.com/geeksloth)
 */

#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>
#include <CenterClockDisplay.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <Updater.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiManager.h>

#include "ClockConfig.h"
#include "CloudNodeSettings.h"
#include "CloudTime.h"

using namespace CloudNodeSettings;

CenterClockDisplay display(D7, D5, D2, D1);
ClockConfigStore configStore;
CloudTime cloudTime;
WiFiManager wifiManager;

WiFiClient plainTransport;
BearSSL::WiFiClientSecure secureTransport;
MqttClient *mqttClient = nullptr;
BearSSL::PublicKey *mqttServerPublicKey = nullptr;
BearSSL::PublicKey *firmwareServerPublicKey = nullptr;
BearSSL::PublicKey *otaSigningPublicKey = nullptr;
BearSSL::HashSHA256 *otaHash = nullptr;
BearSSL::SigningVerifier *otaVerifier = nullptr;

char hardwareId[9];
char accessPointName[32];
char deviceState[24] = "unconfirmed";

String topicEnrollUp;
String topicTimeRequest;
String topicStatus;
String topicConfigApplied;
String topicOtaStatus;
String topicEnrollDown;
String topicTimeResponse;
String topicConfigDown;
String topicOtaCommand;

uint32_t lastMqttAttemptMs = 0;
uint32_t lastWifiAttemptMs = 0;
uint32_t lastEnrollmentMs = 0;
uint32_t lastTimeRequestMs = 0;
uint32_t lastStatusMs = 0;
uint32_t lastDisplayMs = 0;
uint64_t lastDisplayedSecond = UINT64_MAX;

bool due(uint32_t now, uint32_t previous, uint32_t interval) {
  return static_cast<uint32_t>(now - previous) >= interval;
}

bool mqttConnected() {
  return mqttClient != nullptr && mqttClient->connected();
}

bool isActive() {
  return strcmp(deviceState, "active") == 0;
}

void buildTopics() {
  String root = String(F("centerclock/v1/devices/")) + hardwareId;
  topicEnrollUp = root + F("/up/enroll");
  topicTimeRequest = root + F("/up/time/request");
  topicStatus = root + F("/up/status");
  topicConfigApplied = root + F("/up/config/applied");
  topicOtaStatus = root + F("/up/ota/status");
  topicEnrollDown = root + F("/down/enroll");
  topicTimeResponse = root + F("/down/time/response");
  topicConfigDown = root + F("/down/config");
  topicOtaCommand = root + F("/down/ota/command");
}

bool publishJson(const String &topic, const JsonDocument &document,
                 bool retained = false) {
  if (!mqttConnected()) return false;

  String payload;
  payload.reserve(measureJson(document) + 1);
  serializeJson(document, payload);
  if (!mqttClient->beginMessage(topic, payload.length(), retained, 1)) return false;
  mqttClient->print(payload);
  return mqttClient->endMessage() == 1;
}

void publishEnrollment() {
  JsonDocument document;
  document["schema_version"] = 1;
  document["hardware_id"] = hardwareId;
  document["model"] = HARDWARE_TARGET;
  document["firmware_version"] = FIRMWARE_VERSION;
  if (publishJson(topicEnrollUp, document)) {
    lastEnrollmentMs = millis();
    Serial.println(F("Enrollment announced"));
  }
}

void publishTimeRequest() {
  char requestId[40];
  snprintf(requestId, sizeof(requestId), "%s-%08lx", hardwareId,
           static_cast<unsigned long>(millis()));

  JsonDocument document;
  document["schema_version"] = 1;
  document["request_id"] = requestId;
  document["uptime_ms"] = millis();
  document["applied_config_revision"] = configStore.data().revision;

  cloudTime.beginRequest(requestId);
  if (publishJson(topicTimeRequest, document)) {
    lastTimeRequestMs = millis();
  } else {
    cloudTime.cancelRequest();
  }
}

void publishStatus() {
  JsonDocument document;
  document["schema_version"] = 1;
  document["firmware_version"] = FIRMWARE_VERSION;
  document["uptime_seconds"] = millis() / 1000UL;
  document["wifi_rssi_dbm"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -127;
  document["free_heap_bytes"] = ESP.getFreeHeap();
  document["applied_config_revision"] = configStore.data().revision;
  if (publishJson(topicStatus, document)) lastStatusMs = millis();
}

void publishConfigApplied() {
  JsonDocument document;
  document["schema_version"] = 1;
  document["config_revision"] = configStore.data().revision;
  publishJson(topicConfigApplied, document);
}

void publishOtaStatus(const char *deploymentId, const char *state,
                      const char *errorCode = nullptr,
                      const char *errorMessage = nullptr) {
  JsonDocument document;
  document["schema_version"] = 1;
  document["deployment_id"] = deploymentId;
  document["state"] = state;
  document["firmware_version"] = FIRMWARE_VERSION;
  if (errorCode != nullptr) document["error_code"] = errorCode;
  if (errorMessage != nullptr) document["error_message"] = errorMessage;
  publishJson(topicOtaStatus, document);
}

void handleEnrollment(const JsonDocument &document) {
  const char *state = document["state"] | "unconfirmed";
  strlcpy(deviceState, state, sizeof(deviceState));
  Serial.print(F("Cloud device state: "));
  Serial.println(deviceState);
  if (isActive()) lastTimeRequestMs = millis() - TIME_SYNC_INTERVAL_MS;
}

void handleConfiguration(const JsonDocument &document) {
  if (!configStore.applyMqtt(document)) {
    Serial.println(F("Rejected invalid or stale configuration"));
    return;
  }
  display.setBrightness(configStore.data().brightnessPercent);
  publishConfigApplied();
  lastDisplayedSecond = UINT64_MAX;
  Serial.print(F("Applied configuration revision "));
  Serial.println(configStore.data().revision);
}

void otaProgress(int current, int total) {
  if (mqttConnected()) mqttClient->poll();
  static int lastPercent = -1;
  int percent = total > 0 ? (current * 100) / total : 0;
  if (percent != lastPercent && percent % 10 == 0) {
    lastPercent = percent;
    Serial.printf("OTA progress: %d%%\n", percent);
  }
}

void handleOtaCommand(const JsonDocument &document) {
  const char *deploymentId = document["deployment_id"] | "";
  const char *version = document["version"] | "";
  const char *target = document["hardware_target"] | "";
  const char *url = document["url"] | "";
  const char *signature = document["signature"] | "";

  if (deploymentId[0] == '\0') return;
  if (strcmp(target, HARDWARE_TARGET) != 0) {
    publishOtaStatus(deploymentId, "failed", "wrong_hardware",
                     "Firmware target does not match this clock");
    return;
  }
  if (strcmp(version, FIRMWARE_VERSION) == 0) {
    publishOtaStatus(deploymentId, "succeeded");
    return;
  }
  if (!String(url).startsWith("https://")) {
    publishOtaStatus(deploymentId, "failed", "https_required",
                     "Firmware URL must use HTTPS");
    return;
  }
  if (signature[0] == '\0' || otaVerifier == nullptr ||
      firmwareServerPublicKey == nullptr) {
    publishOtaStatus(deploymentId, "failed", "verification_not_configured",
                     "OTA signing and HTTPS public keys are required");
    return;
  }

  publishOtaStatus(deploymentId, "downloading");
  BearSSL::WiFiClientSecure downloadClient;
  downloadClient.setKnownKey(firmwareServerPublicKey);
  ESPhttpUpdate.rebootOnUpdate(false);
  ESPhttpUpdate.setClientTimeout(15000);
  ESPhttpUpdate.onProgress(otaProgress);

  t_httpUpdate_return result = ESPhttpUpdate.update(downloadClient, url, FIRMWARE_VERSION);
  if (result == HTTP_UPDATE_OK) {
    publishOtaStatus(deploymentId, "succeeded");
    if (mqttConnected()) mqttClient->poll();
    delay(750);
    ESP.restart();
  } else if (result == HTTP_UPDATE_NO_UPDATES) {
    publishOtaStatus(deploymentId, "succeeded");
  } else {
    String error = ESPhttpUpdate.getLastErrorString();
    publishOtaStatus(deploymentId, "failed", "http_update_failed", error.c_str());
  }
}

void onMqttMessage(int messageSize) {
  if (messageSize <= 0 || messageSize > MQTT_PAYLOAD_LIMIT) {
    while (mqttClient->available()) mqttClient->read();
    Serial.println(F("Rejected MQTT message with invalid size"));
    return;
  }

  String topic = mqttClient->messageTopic();
  String payload;
  payload.reserve(messageSize + 1);
  while (mqttClient->available()) payload += static_cast<char>(mqttClient->read());

  JsonDocument document;
  DeserializationError error = deserializeJson(document, payload);
  if (error || document["schema_version"].as<uint8_t>() != 1) {
    Serial.println(F("Rejected invalid MQTT JSON"));
    return;
  }

  if (topic == topicEnrollDown) {
    handleEnrollment(document);
  } else if (topic == topicTimeResponse) {
    if (cloudTime.applyResponse(document)) {
      lastDisplayedSecond = UINT64_MAX;
      Serial.println(F("Cloud time synchronized"));
    }
  } else if (topic == topicConfigDown) {
    handleConfiguration(document);
  } else if (topic == topicOtaCommand) {
    handleOtaCommand(document);
  }
}

bool configureTransport() {
  Client *transport = &plainTransport;
  if (MQTT_USE_TLS) {
    if (strlen(MQTT_SERVER_PUBLIC_KEY_PEM) == 0) {
      Serial.println(F("MQTT TLS enabled without a pinned server public key"));
      return false;
    }
    mqttServerPublicKey = new BearSSL::PublicKey(MQTT_SERVER_PUBLIC_KEY_PEM);
    if (mqttServerPublicKey == nullptr) return false;
    secureTransport.setKnownKey(mqttServerPublicKey);
    transport = &secureTransport;
  }

  mqttClient = new MqttClient(*transport);
  if (mqttClient == nullptr) return false;
  mqttClient->setId(hardwareId);
  mqttClient->setUsernamePassword(MQTT_USERNAME, MQTT_PASSWORD);
  mqttClient->setCleanSession(true);
  mqttClient->setKeepAliveInterval(60000);
  mqttClient->setConnectionTimeout(10000);
  mqttClient->setTxPayloadSize(MQTT_PAYLOAD_LIMIT);
  mqttClient->onMessage(onMqttMessage);
  return true;
}

void configureOtaVerification() {
  if (strlen(OTA_SIGNING_PUBLIC_KEY_PEM) == 0 ||
      strlen(FIRMWARE_SERVER_PUBLIC_KEY_PEM) == 0) {
    Serial.println(F("OTA disabled until signing and HTTPS public keys are configured"));
    return;
  }

  firmwareServerPublicKey =
      new BearSSL::PublicKey(FIRMWARE_SERVER_PUBLIC_KEY_PEM);
  otaSigningPublicKey = new BearSSL::PublicKey(OTA_SIGNING_PUBLIC_KEY_PEM);
  otaHash = new BearSSL::HashSHA256();
  otaVerifier = new BearSSL::SigningVerifier(otaSigningPublicKey);
  if (firmwareServerPublicKey == nullptr || otaSigningPublicKey == nullptr ||
      otaHash == nullptr || otaVerifier == nullptr) {
    Serial.println(F("OTA verification allocation failed"));
    return;
  }
  Update.installSignature(otaHash, otaVerifier);
}

bool connectMqtt() {
  if (mqttClient == nullptr || WiFi.status() != WL_CONNECTED) return false;
  if (mqttConnected()) return true;

  Serial.print(F("Connecting to MQTT at "));
  Serial.print(MQTT_HOST);
  Serial.print(':');
  Serial.println(MQTT_PORT);
  if (!mqttClient->connect(MQTT_HOST, MQTT_PORT)) {
    Serial.print(F("MQTT connection failed: "));
    Serial.println(mqttClient->connectError());
    return false;
  }

  mqttClient->subscribe(topicEnrollDown, 1);
  mqttClient->subscribe(topicTimeResponse, 1);
  mqttClient->subscribe(topicConfigDown, 1);
  mqttClient->subscribe(topicOtaCommand, 1);
  publishEnrollment();
  publishStatus();
  return true;
}

void startWifiPortal() {
  Serial.print(F("Starting Wi-Fi portal: "));
  Serial.println(accessPointName);
  if (mqttConnected()) mqttClient->stop();
  wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT_SECONDS);
  bool connected;
  if (strlen(PORTAL_PASSWORD) >= 8) {
    connected = wifiManager.startConfigPortal(accessPointName, PORTAL_PASSWORD);
  } else {
    connected = wifiManager.startConfigPortal(accessPointName);
  }
  Serial.println(connected ? F("Wi-Fi configured") : F("Wi-Fi portal closed"));
}

void configureWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  wifiManager.setHostname(accessPointName);
  wifiManager.setConnectTimeout(20);
  wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT_SECONDS);

  bool connected;
  if (strlen(PORTAL_PASSWORD) >= 8) {
    connected = wifiManager.autoConnect(accessPointName, PORTAL_PASSWORD);
  } else {
    connected = wifiManager.autoConnect(accessPointName);
  }
  if (connected) {
    Serial.print(F("Wi-Fi connected: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("Wi-Fi setup timed out; use serial command 'wifi' to retry"));
  }
}

void drawWeekday(int mondayBasedDay, const ClockConfigData &config) {
  using namespace CenterClockSegments;
  if (strcmp(config.weekdayMode, "all_white") == 0) {
    for (int day = 0; day < 7; day++) display.setBit(WEEKDAY_WHITE[day]);
  } else if (strcmp(config.weekdayMode, "active_green_only") == 0) {
    display.putWeekday(mondayBasedDay, false);
  } else {
    display.putWeekday(mondayBasedDay, true);
  }
}

void drawSynchronizedClock() {
  struct tm local;
  if (!cloudTime.localTime(local)) return;
  const ClockConfigData &config = configStore.data();

  display.clear();
  int displayHour = local.tm_hour;
  if (config.hourFormat == 12) {
    if (displayHour >= 12) display.setBit(CenterClockSegments::PM);
    displayHour %= 12;
    if (displayHour == 0) displayHour = 12;
  }
  display.putClockTime(displayHour, local.tm_min, config.blankHourLeadingZero);
  drawWeekday((local.tm_wday + 6) % 7, config);
  if (config.showDate) display.putDate(local.tm_mon + 1, local.tm_mday);
  if (config.showDst && cloudTime.dstActive()) display.setBit(CenterClockSegments::DST);
  display.write();
}

void drawWaitingForTime() {
  display.clear();
  if (((millis() / 750UL) % 2UL) == 0) display.putClockTime(88, 88);
  display.write();
}

void updateDisplay() {
  if (cloudTime.synchronized()) {
    uint64_t currentSecond = cloudTime.localEpochSeconds();
    if (currentSecond == lastDisplayedSecond) return;
    lastDisplayedSecond = currentSecond;
    drawSynchronizedClock();
  } else {
    drawWaitingForTime();
  }
}

void printStatus() {
  Serial.printf("id=%s wifi=%s mqtt=%s state=%s heap=%u config=%lu ", hardwareId,
                WiFi.status() == WL_CONNECTED ? "connected" : "offline",
                mqttConnected() ? "connected" : "offline", deviceState,
                ESP.getFreeHeap(),
                static_cast<unsigned long>(configStore.data().revision));
  if (cloudTime.synchronized()) {
    Serial.printf("time-sync-age=%lus\n",
                  static_cast<unsigned long>(cloudTime.lastSyncAgeSeconds()));
  } else {
    Serial.println(F("time=unsynchronized"));
  }
}

void handleSerial() {
  if (!Serial.available()) return;
  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toLowerCase();
  if (command == "status") {
    printStatus();
  } else if (command == "wifi") {
    startWifiPortal();
  } else if (command == "wifi-reset") {
    Serial.println(F("Erasing saved Wi-Fi and restarting"));
    wifiManager.resetSettings();
    delay(250);
    ESP.restart();
  } else if (command == "restart") {
    ESP.restart();
  } else if (command.length() > 0) {
    Serial.println(F("Commands: status, wifi, wifi-reset, restart"));
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  delay(250);

  snprintf(hardwareId, sizeof(hardwareId), "%06x", ESP.getChipId());
  snprintf(accessPointName, sizeof(accessPointName), "CenterClock-%s", hardwareId);
  buildTopics();

  display.begin();
  configStore.begin();
  configStore.load();
  display.setBrightness(configStore.data().brightnessPercent);

  Serial.println();
  Serial.println(F("=== CenterClock cloud node ==="));
  Serial.print(F("Hardware ID: "));
  Serial.println(hardwareId);

  configureOtaVerification();
  configureTransport();
  configureWifi();

  lastMqttAttemptMs = millis() - MQTT_RETRY_MS;
  lastWifiAttemptMs = millis();
  lastEnrollmentMs = millis() - ENROLL_INTERVAL_MS;
  lastTimeRequestMs = millis() - TIME_SYNC_INTERVAL_MS;
  lastStatusMs = millis() - STATUS_INTERVAL_MS;
}

void loop() {
  uint32_t now = millis();
  handleSerial();

  if (WiFi.status() != WL_CONNECTED) {
    if (due(now, lastWifiAttemptMs, WIFI_RETRY_MS)) {
      lastWifiAttemptMs = now;
      WiFi.reconnect();
    }
  } else {
    if (!mqttConnected() && due(now, lastMqttAttemptMs, MQTT_RETRY_MS)) {
      lastMqttAttemptMs = now;
      connectMqtt();
    }
    if (mqttConnected()) {
      mqttClient->poll();
      if (!isActive() && due(now, lastEnrollmentMs, ENROLL_INTERVAL_MS)) {
        publishEnrollment();
      }
      if (isActive() && due(now, lastTimeRequestMs, TIME_SYNC_INTERVAL_MS)) {
        publishTimeRequest();
      }
      if (due(now, lastStatusMs, STATUS_INTERVAL_MS)) publishStatus();
    }
  }

  if (due(now, lastDisplayMs, DISPLAY_INTERVAL_MS)) {
    lastDisplayMs = now;
    updateDisplay();
  }
  delay(2);
}
