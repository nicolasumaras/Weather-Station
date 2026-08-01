#include "settings.h"

#include <Preferences.h>
#include <cstring>

namespace {
Preferences prefs;
constexpr const char* NS = "wstation";

void ensureDefaults(StationSettings& out) {
  if (out.mqttPrefix[0] == '\0') {
    strncpy(out.mqttPrefix, DEFAULT_MQTT_PREFIX, sizeof(out.mqttPrefix) - 1);
  }
  if (out.timezone[0] == '\0') {
    strncpy(out.timezone, DEFAULT_TIMEZONE, sizeof(out.timezone) - 1);
  }
  if (out.windUnit[0] == '\0') {
    strncpy(out.windUnit, DEFAULT_WIND_UNIT, sizeof(out.windUnit) - 1);
  }
  if (out.tempUnit[0] == '\0') {
    strncpy(out.tempUnit, DEFAULT_TEMP_UNIT, sizeof(out.tempUnit) - 1);
  }
  if (out.pressureUnit[0] == '\0') {
    strncpy(out.pressureUnit, DEFAULT_PRESSURE_UNIT, sizeof(out.pressureUnit) - 1);
  }
}
}

bool settingsLoad(StationSettings& out) {
  if (!prefs.begin(NS, true)) {
    StationSettings defaults;
    out = defaults;
    ensureDefaults(out);
    return false;
  }

  StationSettings defaults;
  out = defaults;

  prefs.getString("wifiSsid", out.wifiSsid, sizeof(out.wifiSsid));
  prefs.getString("wifiPass", out.wifiPass, sizeof(out.wifiPass));
  prefs.getString("mqttHost", out.mqttHost, sizeof(out.mqttHost));
  out.mqttPort = prefs.getUShort("mqttPort", DEFAULT_MQTT_PORT);
  prefs.getString("mqttUser", out.mqttUser, sizeof(out.mqttUser));
  prefs.getString("mqttPass", out.mqttPass, sizeof(out.mqttPass));
  prefs.getString("mqttPrefix", out.mqttPrefix, sizeof(out.mqttPrefix));
  out.windMpsPerPps = prefs.getFloat("windFactor", DEFAULT_WIND_MPS_PER_PPS);
  out.geigerUsvPerCpm = prefs.getFloat("geigerFactor", DEFAULT_GEIGER_USV_PER_CPM);
  out.mqttEnabled = prefs.getBool("mqttEnabled", false);
  prefs.getString("timezone", out.timezone, sizeof(out.timezone));
  prefs.getString("windUnit", out.windUnit, sizeof(out.windUnit));
  prefs.getString("tempUnit", out.tempUnit, sizeof(out.tempUnit));
  prefs.getString("pressureUnit", out.pressureUnit, sizeof(out.pressureUnit));
  prefs.end();
  ensureDefaults(out);
  return true;
}

bool settingsSave(const StationSettings& in) {
  if (!prefs.begin(NS, false)) {
    return false;
  }
  prefs.putString("wifiSsid", in.wifiSsid);
  prefs.putString("wifiPass", in.wifiPass);
  prefs.putString("mqttHost", in.mqttHost);
  prefs.putUShort("mqttPort", in.mqttPort);
  prefs.putString("mqttUser", in.mqttUser);
  prefs.putString("mqttPass", in.mqttPass);
  prefs.putString("mqttPrefix", in.mqttPrefix);
  prefs.putFloat("windFactor", in.windMpsPerPps);
  prefs.putFloat("geigerFactor", in.geigerUsvPerCpm);
  prefs.putBool("mqttEnabled", in.mqttEnabled);
  prefs.putString("timezone", in.timezone);
  prefs.putString("windUnit", in.windUnit);
  prefs.putString("tempUnit", in.tempUnit);
  prefs.putString("pressureUnit", in.pressureUnit);
  prefs.end();
  return true;
}

void settingsClearWifi(StationSettings& s) {
  s.wifiSsid[0] = '\0';
  s.wifiPass[0] = '\0';
}

bool settingsHasWifi(const StationSettings& s) {
  return s.wifiSsid[0] != '\0';
}
