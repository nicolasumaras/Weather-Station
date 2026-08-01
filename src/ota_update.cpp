#include "ota_update.h"

#include <ArduinoOTA.h>
#include <Update.h>
#include <WiFi.h>

#include "config.h"

void StationOta::begin() {
  ArduinoOTA.setHostname("weather-station");

  ArduinoOTA
      .onStart([]() {
        const char* type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
        Serial.printf("[ota] ArduinoOTA start (%s)\n", type);
      })
      .onEnd([]() { Serial.println("\n[ota] ArduinoOTA done"); })
      .onProgress([](unsigned int progress, unsigned int total) {
        static int last = -1;
        int pct = total ? static_cast<int>((progress * 100U) / total) : 0;
        if (pct != last && pct % 10 == 0) {
          last = pct;
          Serial.printf("[ota] %u%%\n", pct);
        }
      })
      .onError([](ota_error_t error) { Serial.printf("[ota] Error[%u]\n", error); });
}

void StationOta::loop(bool stationOnline) {
  if (!stationOnline) {
    started_ = false;
    return;
  }
  if (!started_) {
    ArduinoOTA.begin();
    started_ = true;
    Serial.println("[ota] ArduinoOTA ready (hostname: weather-station.local)");
  }
  ArduinoOTA.handle();
}
