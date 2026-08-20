// Author: Dr. Alan Wong (https://github.com/geeksloth)
#include "CloudTime.h"

CloudTime::CloudTime()
    : _synchronized(false),
      _requestPending(false),
      _dstActive(false),
      _requestStartedMs(0),
      _baseMonotonicMs(0),
      _baseUtcMs(0),
      _utcOffsetSeconds(0),
      _organizationOffsetSeconds(0),
      _deviceOffsetSeconds(0) {
  _requestId[0] = '\0';
}

void CloudTime::beginRequest(const char *requestId) {
  strlcpy(_requestId, requestId, sizeof(_requestId));
  _requestStartedMs = millis();
  _requestPending = true;
}

void CloudTime::cancelRequest() {
  _requestPending = false;
  _requestId[0] = '\0';
}

bool CloudTime::applyResponse(const JsonDocument &document) {
  const char *responseId = document["request_id"] | "";
  if (!_requestPending) {
    Serial.println(F("Rejected cloud time: no request is pending"));
    return false;
  }
  if (strcmp(responseId, _requestId) != 0) {
    Serial.println(F("Rejected cloud time: request ID does not match"));
    return false;
  }

  // Cloud timestamps are currently 13 decimal digits. Using `| 0` here makes
  // ArduinoJson infer a 32-bit int and return the default because the value is
  // out of range. Decode the variants explicitly as uint64_t instead.
  uint64_t receivedUtcMs = document["server_received_utc_ms"].as<uint64_t>();
  uint64_t sentUtcMs = document["server_sent_utc_ms"].as<uint64_t>();
  if (receivedUtcMs == 0 || sentUtcMs < receivedUtcMs) {
    Serial.println(F("Rejected cloud time: invalid 64-bit timestamps"));
    return false;
  }

  uint32_t localRoundTripMs = millis() - _requestStartedMs;
  uint64_t serverProcessingMs64 = sentUtcMs - receivedUtcMs;
  uint32_t serverProcessingMs =
      serverProcessingMs64 > UINT32_MAX ? UINT32_MAX : serverProcessingMs64;
  uint32_t networkRoundTripMs =
      localRoundTripMs > serverProcessingMs ? localRoundTripMs - serverProcessingMs : 0;

  _baseUtcMs = sentUtcMs + networkRoundTripMs / 2;
  _baseMonotonicMs = millis();
  _utcOffsetSeconds = document["utc_offset_seconds"] | 0;
  _organizationOffsetSeconds = document["organization_offset_seconds"] | 0;
  _deviceOffsetSeconds = document["device_offset_seconds"] | 0;
  _dstActive = document["dst_active"] | false;
  _synchronized = true;
  cancelRequest();
  return true;
}

bool CloudTime::synchronized() const {
  return _synchronized;
}

bool CloudTime::localTime(struct tm &result) const {
  if (!_synchronized) return false;
  time_t seconds = static_cast<time_t>(localEpochSeconds());
  gmtime_r(&seconds, &result);
  return true;
}

uint64_t CloudTime::localEpochSeconds() const {
  if (!_synchronized) return 0;
  int64_t adjustedMs =
      static_cast<int64_t>(utcNowMs()) +
      static_cast<int64_t>(_utcOffsetSeconds + _organizationOffsetSeconds +
                           _deviceOffsetSeconds) *
          1000LL;
  return adjustedMs > 0 ? static_cast<uint64_t>(adjustedMs / 1000LL) : 0;
}

bool CloudTime::dstActive() const {
  return _dstActive;
}

uint32_t CloudTime::lastSyncAgeSeconds() const {
  if (!_synchronized) return UINT32_MAX;
  return (millis() - _baseMonotonicMs) / 1000UL;
}

uint64_t CloudTime::utcNowMs() const {
  return _baseUtcMs + static_cast<uint32_t>(millis() - _baseMonotonicMs);
}
