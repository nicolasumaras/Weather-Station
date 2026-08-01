#pragma once

#include <Arduino.h>

struct SensorReadings {
  // PM (µg/m³ atmospheric)
  float pm1_0 = NAN;
  float pm2_5 = NAN;
  float pm10 = NAN;
  bool pmValid = false;

  // Wind
  float windMps = 0.0f;
  float windKmh = 0.0f;
  uint32_t windPulsesPerSec = 0;
  bool windValid = false;

  // Radiation
  float cpm = 0.0f;
  float usvPerHour = 0.0f;
  bool geigerValid = false;

  // BME280
  float temperatureC = NAN;
  float humidityPct = NAN;
  float pressureHpa = NAN;
  bool bmeValid = false;

  // Meta
  int8_t wifiRssi = 0;
  bool wifiConnected = false;
  uint32_t uptimeSec = 0;
  uint32_t unixTime = 0;  // 0 if NTP not synced
  bool ntpSynced = false;
  char mode[16] = "boot";  // ap | sta | reconnect
  char localTime[24] = {0};
};

struct HistorySample {
  uint32_t ts = 0;  // unix epoch when NTP synced, else millis/1000
  bool unix = false;
  float pm2_5 = NAN;
  float windMps = NAN;
  float usvPerHour = NAN;
  float pressureHpa = NAN;
  float temperatureC = NAN;
  float humidityPct = NAN;
};
