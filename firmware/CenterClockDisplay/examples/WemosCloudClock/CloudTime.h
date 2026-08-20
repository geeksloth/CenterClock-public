// Author: Dr. Alan Wong (https://github.com/geeksloth)
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>

class CloudTime {
 public:
  CloudTime();

  void beginRequest(const char *requestId);
  void cancelRequest();
  bool applyResponse(const JsonDocument &document);

  bool synchronized() const;
  bool localTime(struct tm &result) const;
  uint64_t localEpochSeconds() const;
  bool dstActive() const;
  uint32_t lastSyncAgeSeconds() const;

 private:
  uint64_t utcNowMs() const;

  bool _synchronized;
  bool _requestPending;
  bool _dstActive;
  char _requestId[65];
  uint32_t _requestStartedMs;
  uint32_t _baseMonotonicMs;
  uint64_t _baseUtcMs;
  int32_t _utcOffsetSeconds;
  int32_t _organizationOffsetSeconds;
  int32_t _deviceOffsetSeconds;
};
