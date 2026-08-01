#pragma once

#include <Arduino.h>

#include "../mqtt/ha_discovery.h"
#include "../net_services.h"
#include "../sensors/history.h"
#include "../sensors/readings.h"
#include "../storage/settings.h"
#include "../wifi_portal.h"

class WebUi {
 public:
  void begin(StationSettings* settings, WifiPortal* wifi, HistoryBuffer* history, HaMqtt* mqtt,
             NetServices* net);
  void loop(const SensorReadings& readings);
  void notifyClients(const SensorReadings& readings);

 private:
  StationSettings* settings_ = nullptr;
  WifiPortal* wifi_ = nullptr;
  HistoryBuffer* history_ = nullptr;
  HaMqtt* mqtt_ = nullptr;
  NetServices* net_ = nullptr;
  SensorReadings last_{};
  uint32_t lastWsMs_ = 0;

  void setupRoutes();
  String readingsJson(const SensorReadings& r) const;
};
