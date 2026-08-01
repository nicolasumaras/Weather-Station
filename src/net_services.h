#pragma once

#include "storage/settings.h"

class NetServices {
 public:
  void begin(StationSettings* settings);
  void loop(bool stationOnline);
  void applyTimezone();
  bool timeSynced() const { return synced_; }
  uint32_t unixTime() const;  // 0 if not synced

 private:
  StationSettings* settings_ = nullptr;
  bool mdnsStarted_ = false;
  bool ntpStarted_ = false;
  bool synced_ = false;
  uint32_t lastSyncCheckMs_ = 0;

  void startServices();
  void stopServices();
};
