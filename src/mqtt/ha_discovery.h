#pragma once

#include "../sensors/readings.h"
#include "../storage/settings.h"

class HaMqtt {
 public:
  void begin(StationSettings* settings);
  void loop(const SensorReadings& readings);
  void publishDiscovery();
  bool connected() const;

 private:
  StationSettings* settings_ = nullptr;
  uint32_t lastReconnectMs_ = 0;
  uint32_t lastPublishMs_ = 0;
  bool discoverySent_ = false;
  String deviceId_;
  String availTopic_;
  String statePrefix_;

  void ensureConnected();
  void publishSensorDiscovery(const char* objectId, const char* name,
                              const char* deviceClass, const char* unit,
                              const char* stateKey, int decimals);
  String stateTopic() const;
};
