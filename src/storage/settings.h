#pragma once

#include <Arduino.h>
#include "../config.h"

struct StationSettings {
  char wifiSsid[33] = {0};
  char wifiPass[65] = {0};
  char mqttHost[64] = {0};
  uint16_t mqttPort = DEFAULT_MQTT_PORT;
  char mqttUser[32] = {0};
  char mqttPass[64] = {0};
  char mqttPrefix[32] = {0};
  float windMpsPerPps = DEFAULT_WIND_MPS_PER_PPS;
  float geigerUsvPerCpm = DEFAULT_GEIGER_USV_PER_CPM;
  bool mqttEnabled = false;
  char timezone[48] = {0};
  char windUnit[8] = {0};      // mps | kmh | mph
  char tempUnit[4] = {0};      // C | F
  char pressureUnit[8] = {0};  // hPa | inHg

  // MeshCore gateway bridge (rak3112-meshcore-mqtt-gateway)
  bool meshEnabled = false;
  char meshHost[64] = {0};  // gateway IP or hostname, no scheme
  // Sized for generated secrets, not typed passwords: a 64-hex-char token is
  // the natural thing to paste here and must fit with room for the terminator.
  char meshAdminPass[129] = {0};  // gateway HTTP Basic auth password for user "admin"
  char meshHookToken[129] = {0};  // shared secret the gateway sends as "Authorization: Bearer"
  char meshKeyword[24] = {0};  // trigger word, matched case-insensitively
  // Per-sender reply window. 0 disables rate limiting entirely.
  uint16_t meshCooldownSec = DEFAULT_MESH_COOLDOWN_SEC;
};

bool settingsLoad(StationSettings& out);
bool settingsSave(const StationSettings& in);
void settingsClearWifi(StationSettings& s);
bool settingsHasWifi(const StationSettings& s);
