// Author: Dr. Alan Wong (https://github.com/geeksloth)
#include "ClockConfig.h"

#include <LittleFS.h>

namespace {

const char *CONFIG_PATH = "/clock-config.json";
const char *BACKUP_PATH = "/clock-config.bak";
const char *TEMP_PATH = "/clock-config.tmp";
const uint8_t CONFIG_SCHEMA_VERSION = 1;

template <size_t N>
void copyText(char (&destination)[N], const char *source) {
  if (source == nullptr) source = "";
  strlcpy(destination, source, N);
}

uint32_t fnvAdd(uint32_t hash, const uint8_t *bytes, size_t length) {
  for (size_t index = 0; index < length; index++) {
    hash ^= bytes[index];
    hash *= 16777619UL;
  }
  return hash;
}

template <typename T>
uint32_t fnvValue(uint32_t hash, const T &value) {
  return fnvAdd(hash, reinterpret_cast<const uint8_t *>(&value), sizeof(value));
}

uint32_t fnvText(uint32_t hash, const char *value) {
  return fnvAdd(hash, reinterpret_cast<const uint8_t *>(value), strlen(value));
}

}  // namespace

ClockConfigStore::ClockConfigStore() : _filesystemReady(false) {
  setDefaults();
}

bool ClockConfigStore::begin() {
  _filesystemReady = LittleFS.begin();
  if (!_filesystemReady) {
    Serial.println(F("LittleFS mount failed; cached configuration is unavailable"));
  }
  return _filesystemReady;
}

bool ClockConfigStore::load() {
  if (!_filesystemReady) return false;
  if (loadPath(CONFIG_PATH)) return true;
  if (loadPath(BACKUP_PATH)) {
    Serial.println(F("Recovered configuration from backup"));
    return true;
  }
  setDefaults();
  return false;
}

bool ClockConfigStore::loadPath(const char *path) {
  File file = LittleFS.open(path, "r");
  if (!file) return false;

  JsonDocument document;
  DeserializationError error = deserializeJson(document, file);
  file.close();
  if (error || document["schema_version"].as<uint8_t>() != CONFIG_SCHEMA_VERSION) {
    return false;
  }

  ClockConfigData candidate = {};
  copyText(candidate.timezone, document["timezone"] | "Asia/Bangkok");
  candidate.organizationOffsetSeconds = document["organization_offset_seconds"] | 0;
  candidate.deviceOffsetSeconds = document["device_offset_seconds"] | 0;
  candidate.hourFormat = document["hour_format"] | 24;
  candidate.brightnessPercent = document["brightness_percent"] | 100;
  const char *unit = document["temperature_unit"] | "C";
  candidate.temperatureUnit = unit[0];
  copyText(candidate.weekdayMode,
           document["weekday_mode"] | "active_green_inactive_white");
  candidate.blankHourLeadingZero = document["blank_hour_leading_zero"] | true;
  candidate.showDate = document["show_date"] | true;
  candidate.showDst = document["show_dst"] | true;
  candidate.revision = document["config_revision"] | 0;

  uint32_t expectedChecksum = document["checksum"] | 0;
  if (expectedChecksum == 0 || expectedChecksum != checksum(candidate)) return false;
  if (candidate.hourFormat != 12 && candidate.hourFormat != 24) return false;
  if (candidate.brightnessPercent > 100) return false;
  if (candidate.temperatureUnit != 'C' && candidate.temperatureUnit != 'F') return false;

  _data = candidate;
  return true;
}

bool ClockConfigStore::save() const {
  if (!_filesystemReady) return false;

  JsonDocument document;
  document["schema_version"] = CONFIG_SCHEMA_VERSION;
  document["timezone"] = _data.timezone;
  document["organization_offset_seconds"] = _data.organizationOffsetSeconds;
  document["device_offset_seconds"] = _data.deviceOffsetSeconds;
  document["hour_format"] = _data.hourFormat;
  document["brightness_percent"] = _data.brightnessPercent;
  document["temperature_unit"] = String(_data.temperatureUnit);
  document["weekday_mode"] = _data.weekdayMode;
  document["blank_hour_leading_zero"] = _data.blankHourLeadingZero;
  document["show_date"] = _data.showDate;
  document["show_dst"] = _data.showDst;
  document["config_revision"] = _data.revision;
  document["checksum"] = checksum(_data);

  File temporary = LittleFS.open(TEMP_PATH, "w");
  if (!temporary) return false;
  bool written = serializeJson(document, temporary) > 0;
  temporary.flush();
  temporary.close();
  if (!written) {
    LittleFS.remove(TEMP_PATH);
    return false;
  }

  LittleFS.remove(BACKUP_PATH);
  if (LittleFS.exists(CONFIG_PATH) && !LittleFS.rename(CONFIG_PATH, BACKUP_PATH)) {
    LittleFS.remove(TEMP_PATH);
    return false;
  }
  if (!LittleFS.rename(TEMP_PATH, CONFIG_PATH)) {
    if (LittleFS.exists(BACKUP_PATH)) LittleFS.rename(BACKUP_PATH, CONFIG_PATH);
    return false;
  }
  LittleFS.remove(BACKUP_PATH);
  return true;
}

bool ClockConfigStore::applyMqtt(const JsonDocument &document) {
  uint32_t incomingRevision = document["config_revision"] | 0;
  if (incomingRevision == 0 || incomingRevision < _data.revision) return false;
  // Retained MQTT configuration is delivered after every reconnect. A matching
  // revision only needs an acknowledgement; avoiding another flash write here
  // prevents unnecessary LittleFS wear.
  if (incomingRevision == _data.revision) return true;

  ClockConfigData candidate = _data;
  copyText(candidate.timezone, document["timezone"] | candidate.timezone);

  if (!document["organization_time_offset_seconds"].isNull()) {
    candidate.organizationOffsetSeconds = document["organization_time_offset_seconds"];
  } else if (!document["time_offset_seconds"].isNull()) {
    candidate.organizationOffsetSeconds = document["time_offset_seconds"];
    candidate.deviceOffsetSeconds = 0;
  }
  if (!document["device_time_offset_seconds"].isNull()) {
    candidate.deviceOffsetSeconds = document["device_time_offset_seconds"];
  }

  candidate.hourFormat = document["hour_format"] | candidate.hourFormat;
  candidate.brightnessPercent =
      document["brightness_percent"] | candidate.brightnessPercent;
  const char *unit = document["temperature_unit"] | nullptr;
  if (unit != nullptr && (unit[0] == 'C' || unit[0] == 'F')) {
    candidate.temperatureUnit = unit[0];
  }
  copyText(candidate.weekdayMode, document["weekday_mode"] | candidate.weekdayMode);

  JsonObjectConst options = document["display_options"].as<JsonObjectConst>();
  if (!options.isNull()) {
    candidate.blankHourLeadingZero =
        options["blank_hour_leading_zero"] | candidate.blankHourLeadingZero;
    candidate.showDate = options["show_date"] | candidate.showDate;
    candidate.showDst = options["show_dst"] | candidate.showDst;
  }
  candidate.revision = incomingRevision;

  if (candidate.hourFormat != 12 && candidate.hourFormat != 24) return false;
  if (candidate.brightnessPercent > 100) return false;
  if (candidate.organizationOffsetSeconds < -86400 ||
      candidate.organizationOffsetSeconds > 86400 ||
      candidate.deviceOffsetSeconds < -86400 || candidate.deviceOffsetSeconds > 86400) {
    return false;
  }

  _data = candidate;
  save();
  return true;
}

const ClockConfigData &ClockConfigStore::data() const {
  return _data;
}

void ClockConfigStore::setDefaults() {
  memset(&_data, 0, sizeof(_data));
  copyText(_data.timezone, "Asia/Bangkok");
  _data.hourFormat = 24;
  _data.brightnessPercent = 100;
  _data.temperatureUnit = 'C';
  copyText(_data.weekdayMode, "active_green_inactive_white");
  _data.blankHourLeadingZero = true;
  _data.showDate = true;
  _data.showDst = true;
}

uint32_t ClockConfigStore::checksum(const ClockConfigData &value) const {
  uint32_t hash = 2166136261UL;
  hash = fnvText(hash, value.timezone);
  hash = fnvValue(hash, value.organizationOffsetSeconds);
  hash = fnvValue(hash, value.deviceOffsetSeconds);
  hash = fnvValue(hash, value.hourFormat);
  hash = fnvValue(hash, value.brightnessPercent);
  hash = fnvValue(hash, value.temperatureUnit);
  hash = fnvText(hash, value.weekdayMode);
  hash = fnvValue(hash, value.blankHourLeadingZero);
  hash = fnvValue(hash, value.showDate);
  hash = fnvValue(hash, value.showDst);
  hash = fnvValue(hash, value.revision);
  return hash;
}
