# Bluetooth Disconnect Tester

ESP32-C3 firmware and a Web Bluetooth test page for Chromium issue 40502943: `BluetoothRemoteGATTServer.disconnect()` should cancel a pending `gatt.connect()`.

## Hardware

- ESP32-C3 development board
- Chrome on Windows for the first test pass

## Firmware

The firmware advertises a BLE GATT server named `dino c(h)ancler` with one test service.

Build with PlatformIO:

```bash
cd firmware/arduino-esp32c3
pio run -t upload
pio device monitor
```

Serial commands:

- `a` - start advertising
- `s` - stop advertising
- `d` - disconnect connected central
- `z` - stop advertising, disconnect, and enter deep sleep
- `r` - restart the board

## Manual Windows repro loop

1. Flash the ESP32-C3 firmware.
2. Open `web/index.html` from a secure origin, for example with a local HTTPS server or GitHub Pages.
3. Click `Request device` while the ESP32-C3 is advertising.
4. Click `Connect` once to verify the device works.
5. Disconnect, then make the ESP32-C3 unavailable (`z` over serial, reset without advertising, or power off).
6. Click `Connect`, then click `Disconnect / cancel pending connect` while `connect()` is pending.
7. Expected after the Chromium fix: the pending `connect()` promise rejects promptly with `AbortError`, and a subsequent `requestDevice()` or connection to another BLE device still works without reloading the page.

## Chromium bug

- https://issues.chromium.org/issues/40502943
