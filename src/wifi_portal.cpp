#include "wifi_portal.h"

#include <WiFi.h>
#include <cstring>

#include "config.h"

void WifiPortal::begin(StationSettings& settings) {
  settings_ = &settings;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("weather-station");

  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(apSsid_, sizeof(apSsid_), "%s-%02X%02X", AP_SSID_PREFIX, mac[4], mac[5]);

  if (!settingsHasWifi(*settings_)) {
    startPortal("No WiFi credentials");
    return;
  }
  tryConnectStation();
}

bool WifiPortal::isStationOnline() const {
  return mode_ == WifiMode::Station && WiFi.status() == WL_CONNECTED;
}

void WifiPortal::startPortal(const char* reason) {
  strncpy(reason_, reason ? reason : "Setup required", sizeof(reason_) - 1);
  mode_ = WifiMode::Portal;
  ensurePortal();
}

void WifiPortal::ensurePortal() {
  if (portalStarted_) {
    return;
  }
  WiFi.disconnect(true, false);
  delay(100);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSsid_);
  portalStarted_ = true;
  Serial.printf("[wifi] Portal AP: %s  IP: %s (%s)\n", apSsid_,
                WiFi.softAPIP().toString().c_str(), reason_);
}

void WifiPortal::tryConnectStation() {
  if (!settings_ || !settingsHasWifi(*settings_)) {
    startPortal("No WiFi credentials");
    return;
  }

  portalStarted_ = false;
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  mode_ = WifiMode::Reconnecting;
  disconnectSinceMs_ = millis();
  strncpy(reason_, "Connecting", sizeof(reason_) - 1);
  Serial.printf("[wifi] Connecting to %s\n", settings_->wifiSsid);
  WiFi.begin(settings_->wifiSsid, settings_->wifiPass);
}

void WifiPortal::applySavedStation() {
  tryConnectStation();
}

void WifiPortal::forgetWifiAndReboot() {
  if (!settings_) {
    return;
  }
  settingsClearWifi(*settings_);
  settingsSave(*settings_);
  delay(200);
  ESP.restart();
}

void WifiPortal::loop() {
  if (!settings_) {
    return;
  }

  if (mode_ == WifiMode::Portal) {
    ensurePortal();
    return;
  }

  wl_status_t st = WiFi.status();
  if (st == WL_CONNECTED) {
    if (mode_ != WifiMode::Station) {
      mode_ = WifiMode::Station;
      disconnectSinceMs_ = 0;
      strncpy(reason_, "Connected", sizeof(reason_) - 1);
      Serial.printf("[wifi] Connected, IP %s RSSI %d\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
    }
    return;
  }

  // Not connected
  if (mode_ == WifiMode::Station) {
    mode_ = WifiMode::Reconnecting;
    disconnectSinceMs_ = millis();
    strncpy(reason_, "Connection lost", sizeof(reason_) - 1);
    Serial.println("[wifi] Lost connection, retrying...");
  }

  if (mode_ == WifiMode::Reconnecting || mode_ == WifiMode::Boot) {
    if (disconnectSinceMs_ == 0) {
      disconnectSinceMs_ = millis();
    }
    // Periodically re-issue begin
    static uint32_t lastBegin = 0;
    if (millis() - lastBegin > 15000) {
      lastBegin = millis();
      WiFi.disconnect();
      WiFi.begin(settings_->wifiSsid, settings_->wifiPass);
    }
    if (millis() - disconnectSinceMs_ >= WIFI_RETRY_MS) {
      startPortal("WiFi failed for 5 minutes");
    }
  }
}
