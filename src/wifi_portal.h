#pragma once

#include "storage/settings.h"

enum class WifiMode {
  Boot,
  Portal,
  Station,
  Reconnecting,
};

class WifiPortal {
 public:
  void begin(StationSettings& settings);
  void loop();
  WifiMode mode() const { return mode_; }
  bool isPortalActive() const { return mode_ == WifiMode::Portal; }
  bool isStationOnline() const;
  void startPortal(const char* reason);
  void forgetWifiAndReboot();
  void applySavedStation();
  const char* apSsid() const { return apSsid_; }
  const char* statusReason() const { return reason_; }

 private:
  StationSettings* settings_ = nullptr;
  WifiMode mode_ = WifiMode::Boot;
  uint32_t disconnectSinceMs_ = 0;
  char apSsid_[32] = {0};
  char reason_[48] = {0};
  bool portalStarted_ = false;

  void ensurePortal();
  void tryConnectStation();
};
