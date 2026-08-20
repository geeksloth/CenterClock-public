/* Keep all 112 LED outputs on continuously. */
// Author: Dr. Alan Wong (https://github.com/geeksloth)

#include <CenterClockDisplay.h>

CenterClockDisplay display(D7, D5, D2, D1);

void setup() {
  Serial.begin(115200);
  delay(500);

  display.begin();
  display.fill(true);
  display.write();

  Serial.println("All 112 LED channels are ON.");
}

void loop() {
  // Reload periodically in case electrical noise changes a driver bit.
  display.write();
  delay(1000);
}
