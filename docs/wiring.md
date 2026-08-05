# Wiring — ESP32-S3 Weather Station

Default pin map for a generic **ESP32-S3 DevKitC-1**. Change pins in [`src/config.h`](../src/config.h) if needed.

## Power

| Rail | Use |
|------|-----|
| **5 V** | SSAP10 VCC, anemometer VCC (5–30 V; 5 V OK), Geiger breakout VCC (if rated 5 V) |
| **3.3 V** | BME280 VCC, ESP32-S3 logic |
| **GND** | Common ground for all modules |

Do not feed 5 V UART or open-collector pulled to 5 V into ESP32-S3 pins without level shifting.

## Connections

| Device | Module pin | ESP32-S3 GPIO | Notes |
|--------|------------|---------------|-------|
| SSAP10 | VCC | 5 V | Fan draws ~85 mA |
| SSAP10 | GND | GND | |
| SSAP10 | TX | **GPIO 13** (RX) | 3.3 V TTL, 9600 8N1 |
| SSAP10 | RX | **GPIO 12** (TX) | Optional for passive mode |
| SSAP10 | SET / RESET | NC | Internal pull-ups |
| Anemometer | Brown VCC | 5 V | Confirm your cable colors |
| Anemometer | Black GND | GND | |
| Anemometer | Blue NPN out | **GPIO 14** | Open-collector: use ESP pull-up (INPUT_PULLUP). If output swings to 5 V, use a divider |
| Geiger kit | VCC | 3.3–5 V per board | HV present on tube terminals — careful |
| Geiger kit | GND | GND | |
| Geiger kit | INT / OUT | **GPIO 11** | Active-low pulse, falling edge |
| BME280 | VCC | 3.3 V | |
| BME280 | GND | GND | |
| BME280 | SDA | **GPIO 5** | Address 0x76 or 0x77 |
| BME280 | SCL | **GPIO 4** | |
| Status LED | onboard RGB (WS2812) | **GPIO 48** (`RGB_BUILTIN`) | No wiring needed. Red = sensor fault, green = online, blue blink = setup/reconnect |

## Sensor notes

### SSAP10 (Makerfabs)
- UART frames start with `0x42 0x4D`, 32 bytes, ~1 Hz active mode.
- Atmospheric PM1.0 / PM2.5 / PM10 used for display and Home Assistant.

### NPN anemometer
- Default calibration: **0.0875 m/s per pulse per second** (Makerfabs-class).
- Adjust under Setup → Calibration if your cups differ.

### Geiger (uRAD-style pulse kit)
- Firmware counts falling edges, computes CPM over 10 s windows.
- Default dose factor **0.0057 µSv/h per CPM** (SBM-20). Typical J305 ≈ 0.00812 — set in Setup.

### BME280
- Provides temperature, humidity, and barometric pressure (hPa).
