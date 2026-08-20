// Author: Dr. Alan Wong (https://github.com/geeksloth)
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

struct ClockConfigData {
  char timezone[64];
  int32_t organizationOffsetSeconds;
  int32_t deviceOffsetSeconds;
  uint8_t hourFormat;
  uint8_t brightnessPercent;
  char temperatureUnit;
  char weekdayMode[40];
  bool blankHourLeadingZero;
  bool showDate;
  bool showDst;
  uint32_t revision;
};

class ClockConfigStore {
 public:
  ClockConfigStore();

  bool begin();
  bool load();
  bool save() const;
  bool applyMqtt(const JsonDocument &document);

  const ClockConfigData &data() const;

 private:
  void setDefaults();
  bool loadPath(const char *path);
  uint32_t checksum(const ClockConfigData &value) const;

  bool _filesystemReady;
  ClockConfigData _data;
};
