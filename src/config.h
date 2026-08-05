#pragma once

#include <Arduino.h>

// ---- Pins (ESP32-S3 DevKitC-1 defaults) ----
static constexpr int PIN_SSAP10_RX = 13;  // ESP RX  <- sensor TX
static constexpr int PIN_SSAP10_TX = 12;  // ESP TX  -> sensor RX
static constexpr int PIN_ANEMOMETER = 14;
static constexpr int PIN_GEIGER = 11;
static constexpr int PIN_BME_SDA = 5;
static constexpr int PIN_BME_SCL = 4;
static constexpr int PIN_STATUS_LED = RGB_BUILTIN;  // onboard WS2812 (GPIO 48 on DevKitC-1)
static constexpr uint8_t STATUS_LED_BRIGHTNESS = 40;

// ---- Timing ----
static constexpr uint32_t WIFI_RETRY_MS = 5UL * 60UL * 1000UL;  // 5 minutes
static constexpr uint32_t SENSOR_LOOP_MS = 1000;
static constexpr uint32_t HISTORY_SAMPLE_MS = 15000;
static constexpr size_t HISTORY_CAPACITY = 720;  // 3 hours @ 15s
static constexpr uint32_t GEIGER_WINDOW_MS = 10000;
static constexpr uint32_t MQTT_RECONNECT_MS = 10000;
static constexpr uint32_t WIND_SAMPLE_MS = 1000;

// ---- Defaults ----
static constexpr float DEFAULT_WIND_MPS_PER_PPS = 0.0875f;
static constexpr float DEFAULT_GEIGER_USV_PER_CPM = 0.0057f;  // SBM-20
static constexpr uint16_t DEFAULT_MQTT_PORT = 1883;
static constexpr const char* DEFAULT_MQTT_PREFIX = "weather_station";
static constexpr const char* AP_SSID_PREFIX = "WeatherStation";
static constexpr const char* DEVICE_NAME = "Weather Station";
static constexpr const char* DEVICE_MODEL = "ESP32-S3 Weather Station";
static constexpr const char* FW_VERSION = "1.2.0";
static constexpr const char* MDNS_HOSTNAME = "weather-station";
// POSIX TZ: Brazil (UTC-3, no DST). Examples: "UTC0", "EST5EDT,M3.2.0,M11.1.0"
static constexpr const char* DEFAULT_TIMEZONE = "<-03>3";
static constexpr const char* DEFAULT_WIND_UNIT = "mps";   // mps | kmh | mph
static constexpr const char* DEFAULT_TEMP_UNIT = "C";     // C | F
static constexpr const char* DEFAULT_PRESSURE_UNIT = "hPa";  // hPa | inHg

// ---- UART ----
static constexpr uint32_t SSAP10_BAUD = 9600;
