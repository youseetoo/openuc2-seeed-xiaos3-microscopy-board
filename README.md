# XIAO ESP32S3 Robot Board — PlatformIO Project

## Project Structure

```
xiao_robot/
├── platformio.ini          # Build config (seeed_xiao_esp32s3)
├── include/
│   └── pins.h              # Corrected pin mapping
└── src/
    └── main.cpp            # Webserver + all peripherals
```

## Pin Mapping (corrected from PCB table)

| D-Label | GPIO   | Function         |
|---------|--------|-----------------|
| D0      | GPIO1  | Touch Button 1  |
| D1      | GPIO2  | Touch Button 2  |
| D2      | GPIO3  | Motor 2 IN2     |
| D3      | GPIO4  | Motor 1 IN1     |
| D4      | GPIO5  | Motor 1 IN2     |
| D5      | GPIO6  | I2C SCL         |
| D6      | GPIO7  | NeoPixel data   |
| D7      | GPIO8  | Servo 1 PWM     |
| D8      | GPIO9  | Servo 2 PWM     |
| D9      | GPIO10 | Motor 2 IN1     |
| D10     | GPIO11 | I2C SDA         |

## Corrections vs. Original Draft

The original draft had several pin assignment errors:
- `I2C_SDA` was `D4` → corrected to `D10` (GPIO11)
- `I2C_SCL` was `D5` → correct ✓ (GPIO6)
- `MOTOR_2_IN2` was `D10` → corrected to `D2` (GPIO3)
- `MOTOR_1_IN1` was `D3` → correct ✓ (GPIO4)
- `MOTOR_1_IN2` was `D4` (GPIO9??) → corrected to `D4` (GPIO5)
- `MOTOR_2_IN1` was `D9` → correct ✓ (GPIO10)

## WiFi / Web UI

By default the board starts as an **Access Point**:
- SSID: `XiaoRobot`
- Password: `robot1234`
- URL: `http://192.168.4.1`

To connect to your home network instead, set `STA_MODE true` and fill in credentials at the top of `main.cpp`.

## Touch Pads (ESP32S3 note)

The ESP32S3 uses capacitive touch with **higher values when touched** (opposite to older ESP32). The threshold in `pins.h` (`TOUCH_THRESHOLD 40000`) may need tuning for your specific PCB. Print raw values via Serial and adjust.

## Motor Control

Motors are driven via H-bridge (e.g. DRV8833 / TB6612). Speed range is -255 to +255:
- Positive = forward
- Negative = reverse
- 0 = stop (coast)

## Flash / Upload

```bash
pio run --target upload
pio device monitor   # Serial at 115200 baud
```
