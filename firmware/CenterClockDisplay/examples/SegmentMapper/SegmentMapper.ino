/*
 * Map one raw channel at a time when pads 17 and 20 are hard-enabled.
 * Open Serial Monitor at 115200 baud with Newline enabled.
 *
 * Author: Dr. Alan Wong (https://github.com/geeksloth)
 */

#include <CenterClockDisplay.h>

CenterClockDisplay display(D7, D5, D2, D1);

int selectedChannel = 0;
bool autoRun = false;
unsigned long lastAutoStep = 0;
const unsigned long AUTO_STEP_MS = 1000;

void showSelectedChannel() {
  display.clear();
  display.setBit(selectedChannel);
  display.write();

  Serial.print("MAP,bit=");
  Serial.print(selectedChannel);
  Serial.print(",shiftGroup=");
  Serial.print((selectedChannel / 16) + 1);
  Serial.print(",groupBit=");
  Serial.println(selectedChannel % 16);
}

void moveChannel(int change) {
  selectedChannel += change;
  if (selectedChannel >= CenterClockSegments::BIT_COUNT) selectedChannel = 0;
  if (selectedChannel < 0) selectedChannel = CenterClockSegments::BIT_COUNT - 1;
  showSelectedChannel();
}

void printHelp() {
  Serial.println("Commands: n=next, p=previous, r=auto, x=blank, 0..111=jump, ?=help");
}

void processCommand(String command) {
  command.trim();
  command.toLowerCase();
  if (command.length() == 0) return;

  if (command == "n") {
    autoRun = false;
    moveChannel(1);
  } else if (command == "p") {
    autoRun = false;
    moveChannel(-1);
  } else if (command == "r") {
    autoRun = !autoRun;
    lastAutoStep = millis();
    Serial.println(autoRun ? "Automatic scan: ON" : "Automatic scan: OFF");
  } else if (command == "x") {
    autoRun = false;
    display.clear();
    display.write();
    Serial.println("Outputs blanked");
  } else if (command == "?") {
    printHelp();
  } else {
    bool numeric = true;
    for (unsigned int i = 0; i < command.length(); i++) {
      if (!isDigit(command[i])) numeric = false;
    }

    int requested = command.toInt();
    if (numeric && requested >= 0 && requested < CenterClockSegments::BIT_COUNT) {
      autoRun = false;
      selectedChannel = requested;
      showSelectedChannel();
    } else {
      Serial.println("Enter a channel from 0 to 111, or ? for help.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  delay(500);

  display.begin();
  printHelp();
  showSelectedChannel();
}

void loop() {
  if (Serial.available()) processCommand(Serial.readStringUntil('\n'));

  if (autoRun && millis() - lastAutoStep >= AUTO_STEP_MS) {
    lastAutoStep = millis();
    moveChannel(1);
  }
}
