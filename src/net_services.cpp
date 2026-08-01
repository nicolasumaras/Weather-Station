#include "net_services.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <ctime>

#include "config.h"

void NetServices::begin(StationSettings* settings) {
  settings_ = settings;
  applyTimezone();
}

void NetServices::applyTimezone() {
  if (!settings_) {
    return;
  }
  const char* tz = settings_->timezone[0] ? settings_->timezone : DEFAULT_TIMEZONE;
  setenv("TZ", tz, 1);
  tzset();
  Serial.printf("[ntp] TZ=%s\n", tz);
}

void NetServices::startServices() {
  if (!ntpStarted_) {
    applyTimezone();
    const char* tz = settings_->timezone[0] ? settings_->timezone : DEFAULT_TIMEZONE;
    configTzTime(tz, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
    ntpStarted_ = true;
    Serial.println("[ntp] Sync started");
  }

  if (!mdnsStarted_) {
    if (MDNS.begin(MDNS_HOSTNAME)) {
      MDNS.addService("http", "tcp", 80);
      MDNS.addService("arduino", "tcp", 3232);
      mdnsStarted_ = true;
      Serial.printf("[mdns] http://%s.local\n", MDNS_HOSTNAME);
    } else {
      Serial.println("[mdns] begin failed");
    }
  }
}

void NetServices::stopServices() {
  if (mdnsStarted_) {
    MDNS.end();
    mdnsStarted_ = false;
  }
  ntpStarted_ = false;
  synced_ = false;
}

uint32_t NetServices::unixTime() const {
  time_t now = time(nullptr);
  if (now < 1700000000) {
    return 0;
  }
  return static_cast<uint32_t>(now);
}

void NetServices::loop(bool stationOnline) {
  if (!stationOnline) {
    if (mdnsStarted_ || ntpStarted_) {
      stopServices();
    }
    return;
  }

  startServices();

  if (!synced_ && millis() - lastSyncCheckMs_ > 2000) {
    lastSyncCheckMs_ = millis();
    if (unixTime() > 0) {
      synced_ = true;
      time_t raw = time(nullptr);
      struct tm ti;
      localtime_r(&raw, &ti);
      char buf[32];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ti);
      Serial.printf("[ntp] Synced: %s\n", buf);
    }
  }
}
