/* Turn all LED channels on and off through Serial Monitor. */
// Author: Dr. Alan Wong (https://github.com/geeksloth)

#include <CenterClockDisplay.h>

CenterClockDisplay display(D7, D5, D2, D1);
bool ledsAreOn = false;

void setAllLeds(bool on) {
  ledsAreOn = on;
  display.fill(on);
  display.write();
  Serial.println(on ? "All LEDs: ON" : "All LEDs: OFF");
}

void printHelp() {
  Serial.println("Commands: on, off, toggle, status, help");
}

void processCommand(String command) {
  command.trim();
  command.toLowerCase();

  if (command == "on" || command == "1") {
    setAllLeds(true);
  } else if (command == "off" || command == "0") {
    setAllLeds(false);
  } else if (command == "toggle" || command == "t") {
    setAllLeds(!ledsAreOn);
  } else if (command == "status" || command == "s") {
    Serial.println(ledsAreOn ? "All LEDs: ON" : "All LEDs: OFF");
  } else if (command == "help" || command == "?") {
    printHelp();
  } else if (command.length() > 0) {
    Serial.println("Unknown command. Enter help for the command list.");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  delay(500);

  display.begin();
  setAllLeds(false);
  printHelp();
}

void loop() {
  if (Serial.available()) {
    processCommand(Serial.readStringUntil('\n'));
  }
}
