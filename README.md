# CenterClock-public

**Author:** Dr. Alan Wong ([GitHub](https://github.com/geeksloth))

This is the public version of the CenterClock project: firmware for a
cloud-connected LED clock built on the WeMos D1 mini (ESP8266).

## Executive summary

CenterClock is a digital clock whose face is a custom display driven by seven
serially chained LED driver chips — 112 segments in total. This repository
contains the Arduino library that drives that display and the complete firmware
for the clock node. Highlights:

- **`CenterClockDisplay` library** — board-agnostic (ESP8266/ESP32) raw
  112-channel control, shared segment maps, and seven-segment, weekday,
  date, temperature, and clock-face rendering helpers.
- **Cloud time, not NTP** — the node enrolls into the CenterClock cloud over
  MQTT (QoS 1, password-protected) and syncs its time from the cloud. The
  cloud returns UTC, the current numeric timezone offset, and a DST flag, so
  the ESP8266 can display an IANA timezone without carrying a timezone
  database.
- **Robust offline behavior** — between successful syncs the clock keeps
  monotonic time from `millis()`, reports health every 30 seconds, and
  reconnects Wi-Fi/MQTT automatically.
- **Secure first boot and updates** — hardware identity from
  `ESP.getChipId()`, a password-protected captive portal on first boot, and
  signed HTTPS OTA that stays disabled unless the required public keys are
  compiled into the device (the signing private key never lives in this
  repository).

The cloud/dashboard side of the system is **not** included here; this
repository is firmware only. See `firmware/README.md` for the full technical
documentation.

## Repository layout

```
firmware/
└── CenterClockDisplay/
    ├── library.properties
    ├── src/                  # display driver + segment maps
    └── examples/
        ├── AllLedsOn/        # hardware diagnostic
        ├── SegmentMapper/    # segment layout tooling
        ├── SerialLedToggle/  # minimal serial-driven test
        ├── WemosNtpClock/    # NTP experiment
        └── WemosCloudClock/  # the complete cloud-connected clock node
```

## Quick start

1. **Hardware.** WeMos D1 mini wired to the CenterClock display:

   ```
   D7 -> SDI     D5 -> CLK     D2 -> LE     D1 -> OE
   GND -> GND    5V -> circuit VCC
   ```

   Old-MCU pads 17 and 20 are assumed to be permanently pulled high or
   connected to the circuit VCC.

2. **Toolchain.** `arduino-cli` with the ESP8266 Arduino core (3.1.2 or
   newer). WiFiManager, ArduinoMqttClient, and ArduinoJson are declared in
   `library.properties` and installed automatically.

3. **Configure cloud access.** Copy the example secrets file next to the
   sketch and edit it with your broker address and credentials:

   ```sh
   cp firmware/CenterClockDisplay/examples/WemosCloudClock/CloudNodeSecrets.example.h \
      firmware/CenterClockDisplay/examples/WemosCloudClock/CloudNodeSecrets.h
   ```

   `CloudNodeSecrets.h` is git-ignored; never commit real credentials.
   Local development uses plaintext MQTT on port 1883; production must set
   `CENTER_CLOCK_MQTT_USE_TLS` and pin the broker public key.

4. **Build and flash.**

   ```sh
   arduino-cli compile \
     --fqbn esp8266:esp8266:d1_mini \
     --library firmware/CenterClockDisplay \
     firmware/CenterClockDisplay/examples/WemosCloudClock

   arduino-cli upload \
     --fqbn esp8266:esp8266:d1_mini \
     --port /dev/cu.usbserial-XXXX \
     firmware/CenterClockDisplay/examples/WemosCloudClock
   ```

5. **First boot.**

   1. Power the clock.
   2. Join the `CenterClock-<hardware-id>` access point using the portal
      password from your configuration.
   3. Choose your local Wi-Fi network in the captive portal.
   4. The clock connects to MQTT and appears as `unconfirmed` in the
      dashboard; a platform super-admin then binds it to an organization.
   5. It receives its retained configuration and begins cloud-time sync.

   Until time is received the display blinks `88:88`. If the network later
   fails, the clock continues from its last synchronized UTC timestamp; it
   cannot recover correct time after losing both power and network without
   an external RTC.

## Serial monitor

Open the serial monitor at 115200 baud. Available commands:

```
status      print Wi-Fi, MQTT, enrollment, memory, and time-sync status
wifi        open the captive portal for five minutes
wifi-reset  erase stored Wi-Fi credentials and restart
restart     restart the ESP8266
```

## License

Apache License 2.0 — see [LICENSE](LICENSE).
