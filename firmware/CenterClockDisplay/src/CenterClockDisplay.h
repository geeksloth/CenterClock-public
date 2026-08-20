/*
 * CenterClockDisplay - seven-chip, 112-channel LED display driver.
 *
 * Author: Dr. Alan Wong (https://github.com/geeksloth)
 */
#pragma once

#include <Arduino.h>
#include "CenterClockSegments.h"

class CenterClockDisplay {
 public:
  static const uint8_t NO_PIN = 0xFF;

  CenterClockDisplay(uint8_t dataPin,
                     uint8_t clockPin,
                     uint8_t latchPin,
                     uint8_t outputEnablePin = NO_PIN,
                     bool outputEnableActiveLow = true);

  void begin();
  void clear();
  void fill(bool on);
  void setBit(int bit, bool on = true);
  bool getBit(int bit) const;
  void write();
  void setOutputsEnabled(bool enabled);
  bool outputsEnabled() const;
  void setBrightness(uint8_t percent);
  uint8_t brightness() const;

  void putDigit(const int8_t segments[7], int value);
  void putLimitedLeadingOne(const int8_t segments[2], int value);
  void putClockTime(int hour, int minute, bool blankLeadingZero = false);
  void putTemperature(int temperature, bool fahrenheit);
  void putDate(int month, int day);
  void putWeekday(int activeDay, bool showInactiveWhite = true);

 private:
  void shiftBit(bool value);
  void latch();
  void applyOutputEnable();

  uint8_t _dataPin;
  uint8_t _clockPin;
  uint8_t _latchPin;
  uint8_t _outputEnablePin;
  bool _outputEnableActiveLow;
  bool _outputsEnabled;
  uint8_t _brightnessPercent;
  bool _state[CenterClockSegments::BIT_COUNT];
};
