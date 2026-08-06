#include <Arduino.h>
#include <WiFi.h>

#include <ctime>

#include "config.h"
#include "mesh/mesh_bridge.h"
#include "mqtt/ha_discovery.h"
#include "net_services.h"
#include "ota_update.h"
#include "sensors/anemometer.h"
#include "sensors/bme280_sensor.h"
#include "sensors/geiger.h"
#include "sensors/history.h"
#include "sensors/readings.h"
#include "sensors/ssap10.h"
#include "storage/settings.h"
#include "web/server.h"
#include "wifi_portal.h"

namespace {
StationSettings settings;
WifiPortal wifi;
WebUi web;
HaMqtt haMqtt;
StationOta ota;
NetServices net;
Ssap10Sensor ssap10;
AnemometerSensor anemometer;
GeigerSensor geiger;
Bme280Sensor bme;
HistoryBuffer history;
MeshBridge mesh;
SensorReadings readings;
uint32_t bootMs = 0;
uint32_t lastSensorMs = 0;
bool bmeOk = false;
uint8_t ledR = 0, ledG = 0, ledB = 0;

// Red: sensor fault. Green: online. Blue blink: portal / reconnecting.
void updateStatusLed(bool online) {
  uint8_t r = 0, g = 0, b = 0;
  if (!bmeOk) {
    r = STATUS_LED_BRIGHTNESS;
  } else if (online) {
    g = STATUS_LED_BRIGHTNESS;
  } else if ((millis() / 500) % 2) {
    b = STATUS_LED_BRIGHTNESS;
  }
  if (r != ledR || g != ledG || b != ledB) {
    ledR = r;
    ledG = g;
    ledB = b;
    neopixelWrite(PIN_STATUS_LED, r, g, b);
  }
}
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.printf("[%s] %s boot\n", DEVICE_NAME, FW_VERSION);

  neopixelWrite(PIN_STATUS_LED, 0, 0, 0);

  settingsLoad(settings);
  bootMs = millis();

  ssap10.begin(PIN_SSAP10_RX, PIN_SSAP10_TX);
  anemometer.begin(PIN_ANEMOMETER);
  anemometer.setFactor(settings.windMpsPerPps);
  geiger.begin(PIN_GEIGER);
  geiger.setFactor(settings.geigerUsvPerCpm);
  bmeOk = bme.begin(PIN_BME_SDA, PIN_BME_SCL);
  if (!bmeOk) {
    Serial.println("[bme] BME280 not found (check wiring / address)");
  } else {
    Serial.println("[bme] BME280 ready");
  }

  wifi.begin(settings);
  net.begin(&settings);
  haMqtt.begin(&settings);
  ota.begin();
  mesh.begin(&settings);
  web.begin(&settings, &wifi, &history, &haMqtt, &net, &mesh);
}

void loop() {
  wifi.loop();

  // Keep calibration factors in sync after settings changes
  anemometer.setFactor(settings.windMpsPerPps);
  geiger.setFactor(settings.geigerUsvPerCpm);

  ssap10.update(readings);
  anemometer.update(readings);
  geiger.update(readings);

  uint32_t now = millis();
  if (now - lastSensorMs >= SENSOR_LOOP_MS) {
    lastSensorMs = now;
    bme.update(readings);
    readings.wifiConnected = wifi.isStationOnline();
    readings.wifiRssi = readings.wifiConnected ? WiFi.RSSI() : 0;
    readings.uptimeSec = (now - bootMs) / 1000;
    switch (wifi.mode()) {
      case WifiMode::Portal:
        strncpy(readings.mode, "ap", sizeof(readings.mode) - 1);
        break;
      case WifiMode::Station:
        strncpy(readings.mode, "sta", sizeof(readings.mode) - 1);
        break;
      case WifiMode::Reconnecting:
        strncpy(readings.mode, "reconnect", sizeof(readings.mode) - 1);
        break;
      default:
        strncpy(readings.mode, "boot", sizeof(readings.mode) - 1);
        break;
    }
    readings.ntpSynced = net.timeSynced();
    readings.unixTime = net.unixTime();
    readings.localTime[0] = '\0';
    if (readings.ntpSynced) {
      time_t raw = static_cast<time_t>(readings.unixTime);
      struct tm ti;
      localtime_r(&raw, &ti);
      strftime(readings.localTime, sizeof(readings.localTime), "%Y-%m-%d %H:%M:%S", &ti);
    }
    history.maybeSample(readings);
  }

  const bool online = wifi.isStationOnline();
  updateStatusLed(online);

  web.loop(readings);
  net.loop(online);
  ota.loop(online);
  mesh.loop(readings);
  if (online) {
    haMqtt.loop(readings);
  }
}
