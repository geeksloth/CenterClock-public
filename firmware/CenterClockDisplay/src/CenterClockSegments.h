#pragma once

#include <Arduino.h>

/*
 * Physical output mapping for the CenterClock face.
 *
 * Author: Dr. Alan Wong (https://github.com/geeksloth)
 *
 * Seven-segment arrays always use this order:
 *   { a, b, c, d, e, f, g }
 *
 *   --a--
 *  f     b
 *   --g--
 *  e     c
 *   --d--
 *
 * Edit this file when a newly mapped PCB output needs to be corrected.
 */
namespace CenterClockSegments {

static const uint16_t BIT_COUNT = 112;
static const int8_t UNKNOWN_BIT = -1;

// Four large green clock digits: HHMM.
static const int8_t CLOCK_H_TENS[7] = { 42, 40, 43, 45, 46, 47, 44 };
static const int8_t CLOCK_H_ONES[7] = { 34, 35, 38, 41, 39, 36, 37 };
static const int8_t CLOCK_M_TENS[7] = {  9,  7, 13, 10, 11, 12,  8 };
static const int8_t CLOCK_M_ONES[7] = {  0,  1,  3,  4,  5,  6,  2 };

// Temperature: a limited hundreds position followed by two full digits.
static const int8_t TEMP_HUNDREDS_ONE[2] = { 111, 110 }; // b, c
static const int8_t TEMP_TENS[7] = { 105, 96, 104, 107, 109, 108, 106 };
static const int8_t TEMP_ONES[7] = {  99, 97,  98, 101, 102, 103, 100 };

// Month: a limited tens position followed by one full digit.
static const int8_t MONTH_TENS_ONE[2] = { 78, 76 }; // b, c
static const int8_t MONTH_ONES[7] = { 73, 79, 71, 74, 75, 77, 72 };

// The day-tens upper-left segment was not labelled in bits-segments.png.
// Leave it at UNKNOWN_BIT until it is measured instead of guessing.
static const int8_t DAY_TENS[7] = { 58, 56, 57, 60, 61, UNKNOWN_BIT, 59 };
static const int8_t DAY_ONES[7] = { 51, 49, 50, 53, 54, 55, 52 };

// Weekday labels, Monday through Sunday.
static const int8_t WEEKDAY_GREEN[7] = { 24, 26, 29, 16, 17, 21, 23 };
static const int8_t WEEKDAY_WHITE[7] = { 25, 27, 28, 31, 18, 19, 22 };

// Icons and unit labels.
static const int8_t ALARM_TOP_LEFT  = 82;
static const int8_t PM              = 83;
static const int8_t DST             = 85;
static const int8_t ALARM_TOP_RIGHT = 90;
static const int8_t ALARM_1         = 81;
static const int8_t ALARM_2         = 84;
static const int8_t UNIT_C          = 94;
static const int8_t UNIT_F          = 95;
static const int8_t MONTH_LABEL     = 88;
static const int8_t DAY_LABEL       = 92;

} // namespace CenterClockSegments
