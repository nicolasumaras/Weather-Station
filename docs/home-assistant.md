# Home Assistant setup

This station uses **MQTT discovery**. Entities appear automatically — no YAML sensors required.

## Prerequisites

1. MQTT broker running (Mosquitto add-on is fine).
2. Home Assistant **MQTT** integration connected to that broker.

## Device setup

1. Open the Weather Station web UI (station IP, or `http://192.168.4.1` in setup mode).
2. Open **Setup**.
3. Enable **MQTT discovery**.
4. Set broker host (HA / Mosquitto IP), port (`1883`), and credentials if needed.
5. Save. Leave topic prefix as `weather_station` unless you have a reason to change it.

When WiFi and MQTT are up, the firmware publishes discovery configs under `homeassistant/sensor/<device_id>/...` and state JSON to `weather_station/<device_id>/state`.

## Entities created

| Entity (name) | Unit | Notes |
|---------------|------|-------|
| PM2.5 / PM10 / PM1.0 | µg/m³ | From SSAP10 |
| Wind Speed | m/s | NPN anemometer |
| Radiation CPM | CPM | Pulse Geiger |
| Radiation Dose | µSv/h | CPM × tube factor |
| Temperature | °C | BME280 |
| Humidity | % | BME280 |
| Pressure | hPa | BME280 |
| WiFi RSSI | dBm | Link quality |

Availability topic: `weather_station/<device_id>/status` (`online` / `offline`).

## Troubleshooting

- No entities: check MQTT integration, broker IP on the device, and that **Enable MQTT** is on.
- Stale values: confirm WiFi is `sta` on the dashboard and MQTT shows connected in `/api/settings` (`mqtt_connected`).
- Re-discovery: reboot the ESP after changing the topic prefix.
