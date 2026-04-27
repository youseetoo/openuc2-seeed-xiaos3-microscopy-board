# openUC2 XIAO ESP32S3 Microscopy Board — PlatformIO Project

## Project Structure

```
openuc2-seeed-xiaos3-microscopy-board/
├── platformio.ini          # Build config (seeed_xiao_esp32s3)
├── include/
│   ├── pins.h              # Pin mapping
│   └── openuc2logo.png     # Brand logo (embedded as base64 in firmware)
└── src/
    └── main.cpp            # Firmware: web server, captive portal, touch LED toggle
```

## Features

### Captive Portal
When a phone or laptop connects to the Wi-Fi access point, the OS captive-portal
detection is intercepted (DNS wildcard + HTTP redirects) and the browser opens
automatically to the openUC2 controller at `http://192.168.4.1`.

Supported platforms:
- **Android** — `/generate_204`, `/gen_204`
- **iOS / macOS** — `/hotspot-detect.html`
- **Windows** — `/connecttest.txt`, `/ncsi.txt`

### Touch LED Toggle
The two capacitive touch padsThe two capacitive touch padsThe two capacitive touch padsTad | GPIO  | Action                             |
|-----------|-------|------------------------------------|
| D0 (T1) | | | D0 (T1) | | | D0 (T1) | | | D0 (T1)es| D0 (T1) | | | D0 (T1) | | | D0 (T1) | | | D                    |

ToToToToToToToToToTturns the LED on regardless of its current state; touching T2
always turns it off. Debounce is 300 ms per pad.

### Brand-Styled Web UI
The web interface follows the openUC2 brand guidelines:
- **Colours:** Blue `#023773` (background), Green `#85b918` (accents), Teal `#1f9c7c` (secondary)
- **Logo:** served from `/logo` — decoded from PROGMEM base64 and delivered as a PNG
- **Font:** Segoe UI / Arial (web fallback for Stolzl)
- Fully self-contained — no SPIFFS, no external CDN, everything embedded in the firmware

---

## Pin Mapping

| D-Label | GPIO   | Function                 |
|---------|--------|--------------------------|
| D0      | GPIO1  | Touch Button 1 (LED ON)  | D0      | GPIO1  | Touch Button 1 (LED ON)  | D0      | GPIO1  | Touch Button 1 (LED ON)  | D0      | GPIO1  | Touch Button 1 (LED ON)        |
| D4      | GPIO5  | Motor 1 IN2              |
| D5      | GPIO6  | I2C SCL                  |
| D6      | GPIO7  | NeoPixel data            |
| D7      | GPIO8  | Servo 1 PWM              |
| D8      | GPIO9  | Servo 2 PWM              |
| D9      | GPIO10 | Motor 2 IN1              |
| D10     | GPIO11 | I2C SDA                  |

---

## Wi-Fi / Web UI

By default the board starts as an **Access Point**:

- **SSID:** `openUC2-Microscope`
- **Password:** *(open, no password)*
- **URL:** `http://192.168.4.1` — opened automatically by the captive portal

To To To To To To To To To To To To To To To To To To To true` and fill in
`WIFI_SSID` / `WIFI_PASSWORD` at the top of `main.cpp`.

---

## Web API

| Endp| Endp| Endparam| Endp| Endp| Endparam| Endp| Endp| Endparam
|--------------|---------------------|---------------------------|
| `GET /`      | —                   | Main controller UI        |
| `GET /logo`  | —                   | openUC2 logo PNG          |
| `GET /neopixel` | `r,g,b,brightness` | Set LED colour           |
| `GET /motor` | `n,speed`           | Motor speed (-255…255)    |
| `GET /servo` | `n,angle`           | Servo angle (0…180°)      |
| `GET /status`| —                   | JSON: touch, LED, motors  |
| `GET /i2c`   | —                   | JSON: I2C device addresses|

---

## Touch Pad Calibration (ESP32S3 note)

The ESPThe ESPThe ESPThe ESPThe ESPThe ESPThe ESPThe Eposite to older ESP32).
The threshold in `pins.h` (`TOUCH_THRESHOLD 40000`) may need tuning for your PCB.
Print raw values via Serial and adjust accordingly.

---

## Motor Control

Motors are driven via H-bridge (e.g. DRV8833 / TB6612). Speed range: -255 … +255:
- Positive = forward
- Negative = reverse
- 0 = stop (coast)

---

## Flash / Upload

```bash
pio run --target upload
pio device monitor   # Serial at 115200 baud
```
