#pragma once

#include <Arduino.h>
#include <math.h>

// US EPA Air Quality Index.
//
// Mirrors the calculation in data/www/app.js so the mesh reply and the
// dashboard never disagree. PM2.5 uses the 2024 revised breakpoints (the
// revision that moved the top of "Good" from 12.0 to 9.0); PM10 is unchanged.
//
// Breakpoints are defined on 24-hour averages. We feed them a live reading,
// which is the best available here but swings more than a published AQI.

struct AqiBreakpoint {
  float cLo;
  float cHi;
  int iLo;
  int iHi;
};

static const AqiBreakpoint AQI_PM25_TABLE[] = {
    {0.0f, 9.0f, 0, 50},       {9.1f, 35.4f, 51, 100},    {35.5f, 55.4f, 101, 150},
    {55.5f, 125.4f, 151, 200}, {125.5f, 225.4f, 201, 300}, {225.5f, 325.4f, 301, 500},
};

static const AqiBreakpoint AQI_PM10_TABLE[] = {
    {0.0f, 54.0f, 0, 50},       {55.0f, 154.0f, 51, 100},   {155.0f, 254.0f, 101, 150},
    {255.0f, 354.0f, 151, 200}, {355.0f, 424.0f, 201, 300}, {425.0f, 604.0f, 301, 500},
};

// Returns -1 when the concentration is unusable.
inline int aqiSubIndex(float c, const AqiBreakpoint* table, size_t n) {
  if (isnan(c) || c < 0.0f) {
    return -1;
  }
  for (size_t i = 0; i < n; ++i) {
    if (c <= table[i].cHi) {
      const AqiBreakpoint& b = table[i];
      const float span = b.cHi - b.cLo;
      if (span <= 0.0f) {
        return b.iLo;
      }
      return static_cast<int>(lroundf((static_cast<float>(b.iHi - b.iLo) / span) * (c - b.cLo) +
                                      static_cast<float>(b.iLo)));
    }
  }
  return 500;  // Above the top breakpoint.
}

inline int aqiFromPm25(float pm25) {
  return aqiSubIndex(pm25, AQI_PM25_TABLE, sizeof(AQI_PM25_TABLE) / sizeof(AQI_PM25_TABLE[0]));
}

inline int aqiFromPm10(float pm10) {
  return aqiSubIndex(pm10, AQI_PM10_TABLE, sizeof(AQI_PM10_TABLE) / sizeof(AQI_PM10_TABLE[0]));
}

// Overall AQI is the worst of the pollutant sub-indices. -1 if none are valid.
inline int aqiOverall(float pm25, float pm10) {
  const int a = aqiFromPm25(pm25);
  const int b = aqiFromPm10(pm10);
  return (a > b) ? a : b;
}

inline const char* aqiCategory(int aqi) {
  if (aqi < 0) return "n/a";
  if (aqi <= 50) return "Good";
  if (aqi <= 100) return "Moderate";
  if (aqi <= 150) return "Unhealthy(SG)";
  if (aqi <= 200) return "Unhealthy";
  if (aqi <= 300) return "Very unhealthy";
  return "Hazardous";
}
