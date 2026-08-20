/* NTP-synchronized CenterClock for WeMos D1 mini (ESP8266). */
// Author: Dr. Alan Wong (https://github.com/geeksloth)

#include <CenterClockDisplay.h>
#include <ESP8266WiFi.h>
#include <time.h>

// Replace with your own Wi-Fi network.
const char *WIFI_SSID = "your-ssid";
const char *WIFI_PASS = "your-wifi-password";

const long GMT_OFFSET_SEC = 7 * 3600; // Thailand UTC+7
const int DAYLIGHT_OFFSET_SEC = 0;
const char *NTP_1 = "pool.ntp.org";
const char *NTP_2 = "time.nist.gov";

const bool USE_24_HOUR = true;
const bool BLANK_HOUR_LEADING_ZERO = true;
const bool SHOW_INACTIVE_WEEKDAYS_WHITE = true;

CenterClockDisplay display(D7, D5, D2, D1);

bool readLocalTime(struct tm *timeInfo) {
  time_t now = time(nullptr);
  if (now < 1000000000) return false;
  localtime_r(&now, timeInfo);
  return true;
}

bool connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500);
    Serial.print('.');
    tries++;
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed");
    return false;
  }

  Serial.print("Connected, IP: ");
  Serial.println(WiFi.localIP());
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_1, NTP_2);

  Serial.print("Waiting for NTP");
  struct tm currentTime;
  int attempts = 0;
  while (!readLocalTime(&currentTime) && attempts < 20) {
    delay(500);
    Serial.print('.');
    attempts++;
  }
  Serial.println();
  return readLocalTime(&currentTime);
}

void drawClock(const struct tm &currentTime) {
  display.clear();

  int displayHour = currentTime.tm_hour;
  if (!USE_24_HOUR) {
    if (displayHour >= 12) {
      display.setBit(CenterClockSegments::PM);
    }
    displayHour %= 12;
    if (displayHour == 0) displayHour = 12;
  }

  display.putClockTime(
      displayHour,
      currentTime.tm_min,
      BLANK_HOUR_LEADING_ZERO);

  // tm_wday is Sunday=0; the display map uses Monday=0.
  int mondayBasedWeekday = (currentTime.tm_wday + 6) % 7;
  display.putWeekday(
      mondayBasedWeekday,
      SHOW_INACTIVE_WEEKDAYS_WHITE);
  display.putDate(currentTime.tm_mon + 1, currentTime.tm_mday);

  if (currentTime.tm_isdst > 0) {
    display.setBit(CenterClockSegments::DST);
  }

  display.write();
}

void selfTest() {
  display.clear();
  display.putClockTime(88, 88);
  display.write();
  delay(2000);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  display.begin();
  Serial.println("=== WeMos CenterClock NTP example ===");
  selfTest();
  connectWiFi();
}

void loop() {
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();

    struct tm currentTime;
    if (readLocalTime(&currentTime)) {
      drawClock(currentTime);
    } else {
      static bool visible = false;
      visible = !visible;
      display.clear();
      if (visible) display.putClockTime(88, 88);
      display.write();
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastRetry = 0;
    if (millis() - lastRetry >= 30000) {
      lastRetry = millis();
      connectWiFi();
    }
  }
}
