# Weather Station

An ESP32-S3 weather station with a live on-device dashboard, captive-portal WiFi setup, OTA updates, and Home Assistant via MQTT discovery.

Built for particulate matter, wind, ionizing radiation, and barometric conditions — with no cloud account required.

---

## Highlights

| | |
|---|---|
| **Live web UI** | Metrics + trend charts served from the device |
| **Easy WiFi** | Soft-AP setup on first boot; re-opens after 5 minutes offline |
| **Home Assistant** | MQTT discovery — entities appear automatically |
| **OTA** | Web upload or `pio run -e esp32-s3-ota -t upload` |
| **Local-first** | NTP, mDNS (`weather-station.local`), configurable units |

Dashboard: `http://weather-station.local` (or the station’s LAN IP)

---

## Sensors

| Sensor | Interface | Measures |
|--------|-----------|----------|
| [Makerfabs SSAP10](https://www.makerfabs.com/digital-laser-pm2-5-dust-sensor-ssap10.html) | UART 9600 | PM1.0 / PM2.5 / PM10 |
| NPN pulse anemometer | GPIO (falling edge) | Wind speed |
| uRAD-style Geiger kit | GPIO pulse | CPM → µSv/h |
| **BME280** | I2C | Temperature, humidity, pressure |

Default board: **ESP32-S3 DevKitC-1**. Pins live in [`src/config.h`](src/config.h). Full wiring: [`docs/wiring.md`](docs/wiring.md).

```text
                 ┌─────────────────────┐
  SSAP10 UART ──►│                     │──► Web UI + charts
  Anemometer  ──►│     ESP32-S3        │──► MQTT / Home Assistant
  Geiger INT  ──►│                     │──► OTA (web / ArduinoOTA)
  BME280 I2C  ──►│                     │
                 └─────────────────────┘
```

---

## Quick start

### Requirements

- [PlatformIO](https://platformio.org/) CLI or IDE
- ESP32-S3 DevKit (or compatible) + sensors above
- USB cable for the first flash

### Flash

```bash
git clone https://github.com/nicolasumaras/Weather-Station.git
cd Weather-Station
pio run -t upload
pio run -t uploadfs
pio device monitor
```

Serial: **115200** (USB CDC enabled).

### First boot

1. Join WiFi AP **`WeatherStation-XXXX`**
2. Open **`http://192.168.4.1/`** (captive portal may redirect)
3. Pick your network, set MQTT if you use Home Assistant → **Save & connect WiFi**
4. Open **`http://weather-station.local`**
5. Under **Setup → Time & display units**, set timezone and preferred units

If WiFi stays down for **5 minutes**, the setup AP comes back so you can choose another network.

---

## Features in detail

### Web dashboard
- Live PM, wind, dose rate, pressure, temperature, humidity
- On-device history (~3 h @ 15 s samples) with NTP timestamps when synced
- Units: wind **m/s · km/h · mph**, temp **°C · °F**, pressure **hPa · inHg**
- Calibration and MQTT settings in the same UI

### Home Assistant
Enable MQTT in Setup and point at your broker. Discovery publishes PM, wind, radiation, BME280, and WiFi RSSI automatically.

See [`docs/home-assistant.md`](docs/home-assistant.md).

### OTA updates
After the first USB flash (dual-OTA partitions):

```bash
# Network upload
pio run -e esp32-s3-ota -t upload

# Or: Setup → OTA update → firmware.bin / littlefs.bin
```

Details: [`docs/ota.md`](docs/ota.md).

### Calibration defaults
| Parameter | Default | Notes |
|-----------|---------|--------|
| Wind | `0.0875` m/s per pulse/s | Makerfabs-class NPN cups |
| Geiger | `0.0057` µSv/h per CPM | SBM-20; change for J305 etc. |

---

## Project layout

```text
Weather-Station/
├── platformio.ini      # ESP32-S3 + OTA env
├── partitions.csv      # Dual OTA + LittleFS
├── src/                # Firmware
│   ├── main.cpp
│   ├── sensors/        # SSAP10, wind, Geiger, BME280
│   ├── web/            # Async server + API
│   ├── mqtt/           # HA discovery
│   └── ...
├── data/www/           # Dashboard (LittleFS)
└── docs/               # Wiring, HA, OTA
```

---

## Docs

| Doc | Contents |
|-----|----------|
| [`docs/wiring.md`](docs/wiring.md) | Pins, power, level notes |
| [`docs/home-assistant.md`](docs/home-assistant.md) | MQTT discovery setup |
| [`docs/ota.md`](docs/ota.md) | Web + PlatformIO OTA |

---

## License

Use and modify freely for your own station.
