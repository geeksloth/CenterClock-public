// Author: Dr. Alan Wong (https://github.com/geeksloth)
#include "CenterClockDisplay.h"

namespace {

// bit0=a bit1=b bit2=c bit3=d bit4=e bit5=f bit6=g
const uint8_t DIGIT_FONT[10] = {
  0b0111111, // 0
  0b0000110, // 1
  0b1011011, // 2
  0b1001111, // 3
  0b1100110, // 4
  0b1101101, // 5
  0b1111101, // 6
  0b0000111, // 7
  0b1111111, // 8
  0b1101111, // 9
};

} // namespace

CenterClockDisplay::CenterClockDisplay(uint8_t dataPin,
                                       uint8_t clockPin,
                                       uint8_t latchPin,
                                       uint8_t outputEnablePin,
                                       bool outputEnableActiveLow)
    : _dataPin(dataPin),
      _clockPin(clockPin),
      _latchPin(latchPin),
      _outputEnablePin(outputEnablePin),
      _outputEnableActiveLow(outputEnableActiveLow),
      _outputsEnabled(true),
      _brightnessPercent(100) {
  clear();
}

void CenterClockDisplay::begin() {
  pinMode(_dataPin, OUTPUT);
  pinMode(_clockPin, OUTPUT);
  pinMode(_latchPin, OUTPUT);

  digitalWrite(_dataPin, LOW);
  digitalWrite(_clockPin, LOW);
  digitalWrite(_latchPin, LOW);

  if (_outputEnablePin != NO_PIN) {
    pinMode(_outputEnablePin, OUTPUT);
    digitalWrite(_outputEnablePin, _outputEnableActiveLow ? HIGH : LOW);
  }

  clear();
  write();
}

void CenterClockDisplay::clear() {
  fill(false);
}

void CenterClockDisplay::fill(bool on) {
  for (uint16_t bit = 0; bit < CenterClockSegments::BIT_COUNT; bit++) {
    _state[bit] = on;
  }
}

void CenterClockDisplay::setBit(int bit, bool on) {
  if (bit >= 0 && bit < CenterClockSegments::BIT_COUNT) {
    _state[bit] = on;
  }
}

bool CenterClockDisplay::getBit(int bit) const {
  if (bit < 0 || bit >= CenterClockSegments::BIT_COUNT) return false;
  return _state[bit];
}

void CenterClockDisplay::shiftBit(bool value) {
  digitalWrite(_dataPin, value ? HIGH : LOW);
  delayMicroseconds(2);
  digitalWrite(_clockPin, HIGH);
  delayMicroseconds(2);
  digitalWrite(_clockPin, LOW);
  delayMicroseconds(2);
}

void CenterClockDisplay::latch() {
  digitalWrite(_latchPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(_latchPin, LOW);
  delayMicroseconds(5);
}

void CenterClockDisplay::applyOutputEnable() {
  if (_outputEnablePin == NO_PIN) return;

  uint8_t activeLevel = _outputEnableActiveLow ? LOW : HIGH;
  uint8_t inactiveLevel = _outputEnableActiveLow ? HIGH : LOW;
  if (!_outputsEnabled || _brightnessPercent == 0) {
    digitalWrite(_outputEnablePin, inactiveLevel);
    return;
  }
  if (_brightnessPercent >= 100) {
    digitalWrite(_outputEnablePin, activeLevel);
    return;
  }

  const int pwmMaximum = 255;
  int activeDuty = (pwmMaximum * _brightnessPercent) / 100;
  int pinDuty = _outputEnableActiveLow ? pwmMaximum - activeDuty : activeDuty;
  analogWrite(_outputEnablePin, pinDuty);
}

void CenterClockDisplay::write() {
  if (_outputEnablePin != NO_PIN) {
    digitalWrite(_outputEnablePin, _outputEnableActiveLow ? HIGH : LOW);
  }

  for (uint16_t bit = 0; bit < CenterClockSegments::BIT_COUNT; bit++) {
    shiftBit(_state[bit]);
  }
  latch();
  applyOutputEnable();
}

void CenterClockDisplay::setOutputsEnabled(bool enabled) {
  _outputsEnabled = enabled;
  applyOutputEnable();
}

bool CenterClockDisplay::outputsEnabled() const {
  return _outputsEnabled;
}

void CenterClockDisplay::setBrightness(uint8_t percent) {
  _brightnessPercent = constrain(percent, 0, 100);
  applyOutputEnable();
}

uint8_t CenterClockDisplay::brightness() const {
  return _brightnessPercent;
}

void CenterClockDisplay::putDigit(const int8_t segments[7], int value) {
  if (value < 0 || value > 9) return;

  uint8_t pattern = DIGIT_FONT[value];
  for (uint8_t segment = 0; segment < 7; segment++) {
    if ((pattern >> segment) & 1) setBit(segments[segment]);
  }
}

void CenterClockDisplay::putLimitedLeadingOne(const int8_t segments[2], int value) {
  if (value != 1) return;
  setBit(segments[0]);
  setBit(segments[1]);
}

void CenterClockDisplay::putClockTime(int hour, int minute, bool blankLeadingZero) {
  using namespace CenterClockSegments;

  int hourTens = hour / 10;
  if (blankLeadingZero && hourTens == 0) hourTens = -1;

  putDigit(CLOCK_H_TENS, hourTens);
  putDigit(CLOCK_H_ONES, hour % 10);
  putDigit(CLOCK_M_TENS, minute / 10);
  putDigit(CLOCK_M_ONES, minute % 10);
}

void CenterClockDisplay::putTemperature(int temperature, bool fahrenheit) {
  using namespace CenterClockSegments;

  int value = constrain(temperature, 0, 199);
  putLimitedLeadingOne(TEMP_HUNDREDS_ONE, value / 100);

  if (value >= 10) {
    putDigit(TEMP_TENS, (value / 10) % 10);
  }
  putDigit(TEMP_ONES, value % 10);
  setBit(fahrenheit ? UNIT_F : UNIT_C);
}

void CenterClockDisplay::putDate(int month, int day) {
  using namespace CenterClockSegments;

  putLimitedLeadingOne(MONTH_TENS_ONE, month / 10);
  putDigit(MONTH_ONES, month % 10);

  // The physical day-tens position should be blank for days 1-9.
  // Drawing a leading zero also requires the one segment that remains
  // unconfirmed in the PCB map.
  if (day >= 10) {
    putDigit(DAY_TENS, day / 10);
  }
  putDigit(DAY_ONES, day % 10);
  setBit(MONTH_LABEL);
  setBit(DAY_LABEL);
}

void CenterClockDisplay::putWeekday(int activeDay, bool showInactiveWhite) {
  using namespace CenterClockSegments;

  for (int day = 0; day < 7; day++) {
    if (day == activeDay) {
      setBit(WEEKDAY_GREEN[day]);
    } else if (showInactiveWhite) {
      setBit(WEEKDAY_WHITE[day]);
    }
  }
}
