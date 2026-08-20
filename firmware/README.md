# CenterClock firmware

**Author:** Dr. Alan Wong ([GitHub](https://github.com/geeksloth))

## Arduino library

`CenterClockDisplay` contains the shared 112-bit segment map and rendering
helpers. Its examples include hardware diagnostics, field demonstrations, NTP
experiments, and the complete cloud-connected clock node.

## Cloud-connected Wemos clock

The deployable sketch is:

```text
CenterClockDisplay/examples/WemosCloudClock/WemosCloudClock.ino
```

It targets WeMos D1 mini / ESP8266 using this wiring:

```text
D7 -> SDI
D5 -> CLK
D2 -> LE
D1 -> OE
GND -> GND
5V -> circuit VCC
```

Old-MCU pads 17 and 20 are assumed to be permanently pulled high or connected
to the circuit VCC as established during hardware testing.

### Implemented behavior

- hardware ID generated from `ESP.getChipId()`
- WiFiManager captive portal on first boot
- password-protected AP named `CenterClock-<hardware-id>`
- MQTT 3.1.1 with QoS 1 messages
- automatic enrollment as an unconfirmed device
- cloud-time request every 60 seconds after binding
- round-trip delay compensation using server receive/send timestamps
- monotonic offline timekeeping between successful synchronizations
- retained organization and per-device configuration
- atomic LittleFS configuration cache with checksum and backup recovery
- 12/24-hour mode, weekday style, date visibility, DST icon, and brightness
- 30-second health/status reports
- reconnect handling for Wi-Fi and MQTT
- signed HTTPS OTA command handling

The clock intentionally does not use NTP. The cloud returns UTC, the current
numeric timezone offset, and the DST flag. This allows the ESP8266 to display an
IANA timezone without carrying a timezone database.

Temperature remains blank because its source—local sensor, cloud weather, or a
dashboard value—is still an open product decision.

### Configure cloud access

Copy the example secrets file next to the sketch:

```sh
cp firmware/CenterClockDisplay/examples/WemosCloudClock/CloudNodeSecrets.example.h \
   firmware/CenterClockDisplay/examples/WemosCloudClock/CloudNodeSecrets.h
```

Edit `CloudNodeSecrets.h` with the computer or cloud broker address and the
matching values from `.env`. This file is ignored by Git.

For local Compose development, MQTT is plaintext on port 1883. Production must
set `CENTER_CLOCK_MQTT_USE_TLS` and pin the broker public key. Public-key pinning
is used because certificate-date validation would otherwise require a second
time source before the clock could contact the authoritative CenterClock cloud.

### Arduino dependencies

- ESP8266 Arduino core 3.1.2 or newer compatible release
- WiFiManager 2.0.17 or newer
- ArduinoMqttClient 0.1.8 or newer
- ArduinoJson 7.4.3 or newer

They are declared in `CenterClockDisplay/library.properties`.

### Build with Arduino CLI

```sh
arduino-cli compile \
  --fqbn esp8266:esp8266:d1_mini \
  --library firmware/CenterClockDisplay \
  firmware/CenterClockDisplay/examples/WemosCloudClock
```

Upload with the WeMos serial port selected:

```sh
arduino-cli upload \
  --fqbn esp8266:esp8266:d1_mini \
  --port /dev/cu.usbserial-XXXX \
  firmware/CenterClockDisplay/examples/WemosCloudClock
```

### First boot

1. Power the clock.
2. Join `CenterClock-<hardware-id>` using the configured portal password.
3. Choose the local Wi-Fi network in the captive portal.
4. The clock connects to MQTT and appears as `unconfirmed` in the dashboard.
5. A platform super-admin binds it to an organization.
6. The clock receives retained configuration and begins cloud-time sync.

Until time is received, the display blinks `88:88`. If the network later fails,
the clock continues from its last synchronized UTC timestamp using `millis()`.
It cannot recover correct time after losing both power and network without an
external RTC.

### Serial commands

Open the serial monitor at 115200 baud:

```text
status      print Wi-Fi, MQTT, enrollment, memory, and time-sync status
wifi        open the captive portal for five minutes
wifi-reset  erase stored Wi-Fi credentials and restart
restart     restart the ESP8266
```

### Signed OTA

OTA remains disabled unless all required public keys are compiled into the
device settings. The HTTPS server key authenticates the download server. The OTA
signing key is installed into the ESP8266 `Update` verifier and validates the
signature appended to the firmware binary. A command cannot bypass that binary
signature check merely by supplying metadata in MQTT.

The private signing key must remain outside this repository and outside the
cloud runtime that serves firmware files.
