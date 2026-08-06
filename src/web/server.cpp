#include "server.h"

#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Update.h>
#include <WiFi.h>
#include <cstring>

#include "../config.h"

namespace {
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;
bool dnsActive = false;
String settingsBody;

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
               void* arg, uint8_t* data, size_t len) {
  (void)server;
  (void)arg;
  (void)data;
  (void)len;
  if (type == WS_EVT_CONNECT) {
    Serial.printf("[ws] Client #%u connected\n", client->id());
  }
}
}

void WebUi::begin(StationSettings* settings, WifiPortal* wifi, HistoryBuffer* history, HaMqtt* mqtt,
                  NetServices* net) {
  settings_ = settings;
  wifi_ = wifi;
  history_ = history;
  mqtt_ = mqtt;
  net_ = net;

  if (!LittleFS.begin(true)) {
    Serial.println("[fs] LittleFS mount failed");
  }

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  setupRoutes();
  server.begin();
  Serial.println("[web] HTTP server started");
}

String WebUi::readingsJson(const SensorReadings& r) const {
  JsonDocument doc;
  if (r.pmValid) {
    doc["pm1_0"] = r.pm1_0;
    doc["pm2_5"] = r.pm2_5;
    doc["pm10"] = r.pm10;
  } else {
    doc["pm1_0"] = nullptr;
    doc["pm2_5"] = nullptr;
    doc["pm10"] = nullptr;
  }
  doc["pm_valid"] = r.pmValid;
  if (r.windValid) {
    doc["wind_mps"] = r.windMps;
    doc["wind_kmh"] = r.windKmh;
  } else {
    doc["wind_mps"] = nullptr;
    doc["wind_kmh"] = nullptr;
  }
  doc["wind_valid"] = r.windValid;
  if (r.geigerValid) {
    doc["cpm"] = r.cpm;
    doc["usv_h"] = r.usvPerHour;
  } else {
    doc["cpm"] = nullptr;
    doc["usv_h"] = nullptr;
  }
  doc["geiger_valid"] = r.geigerValid;
  if (r.bmeValid) {
    doc["temperature"] = r.temperatureC;
    doc["humidity"] = r.humidityPct;
    doc["pressure"] = r.pressureHpa;
  } else {
    doc["temperature"] = nullptr;
    doc["humidity"] = nullptr;
    doc["pressure"] = nullptr;
  }
  doc["bme_valid"] = r.bmeValid;
  doc["rssi"] = r.wifiRssi;
  doc["wifi"] = r.wifiConnected;
  doc["uptime"] = r.uptimeSec;
  doc["mode"] = r.mode;
  doc["fw"] = FW_VERSION;
  doc["ntp"] = r.ntpSynced;
  if (r.unixTime) {
    doc["unix"] = r.unixTime;
  } else {
    doc["unix"] = nullptr;
  }
  if (r.localTime[0]) {
    doc["local_time"] = r.localTime;
  } else {
    doc["local_time"] = nullptr;
  }
  doc["mdns"] = String("http://") + MDNS_HOSTNAME + ".local";
  if (settings_) {
    doc["wind_unit"] = settings_->windUnit;
    doc["temp_unit"] = settings_->tempUnit;
    doc["pressure_unit"] = settings_->pressureUnit;
  }
  if (wifi_) {
    doc["ap_ssid"] = wifi_->apSsid();
    doc["portal"] = wifi_->isPortalActive();
    doc["wifi_reason"] = wifi_->statusReason();
    if (WiFi.status() == WL_CONNECTED) {
      doc["ip"] = WiFi.localIP().toString();
    } else if (wifi_->isPortalActive()) {
      doc["ip"] = WiFi.softAPIP().toString();
    }
  }
  String out;
  serializeJson(doc, out);
  return out;
}

void WebUi::setupRoutes() {
  server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");

  server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->send(200, "application/json", readingsJson(last_));
  });

  server.on("/api/history", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (!history_) {
      req->send(500, "application/json", "{\"error\":\"no history\"}");
      return;
    }
    // Must not live on the stack: the async_tcp task gets 16 KB
    // (CONFIG_ASYNC_TCP_STACK_SIZE) and this snapshot is 720 * 32 = 22.5 KB,
    // which overflowed it and rebooted the device on every request.
    // Request handlers all run on that one task, so a shared buffer is safe.
    static HistorySample buf[HISTORY_CAPACITY];
    size_t n = history_->copyChronological(buf, HISTORY_CAPACITY);
    JsonDocument doc;
    doc["unix"] = (n > 0) ? buf[n - 1].unix : false;
    JsonArray arr = doc["samples"].to<JsonArray>();
    for (size_t i = 0; i < n; ++i) {
      JsonObject o = arr.add<JsonObject>();
      o["t"] = buf[i].ts;
      o["unix"] = buf[i].unix;
      if (!isnan(buf[i].pm2_5)) o["pm2_5"] = buf[i].pm2_5;
      if (!isnan(buf[i].windMps)) o["wind_mps"] = buf[i].windMps;
      if (!isnan(buf[i].usvPerHour)) o["usv_h"] = buf[i].usvPerHour;
      if (!isnan(buf[i].pressureHpa)) o["pressure"] = buf[i].pressureHpa;
      if (!isnan(buf[i].temperatureC)) o["temperature"] = buf[i].temperatureC;
      if (!isnan(buf[i].humidityPct)) o["humidity"] = buf[i].humidityPct;
    }
    // Stream straight out instead of building the whole payload in a String
    // and letting the server copy it again — with 720 samples that doubled
    // an already large allocation.
    AsyncResponseStream* res = req->beginResponseStream("application/json");
    serializeJson(doc, *res);
    req->send(res);
  });

  server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["wifi_ssid"] = settings_->wifiSsid;
    doc["mqtt_enabled"] = settings_->mqttEnabled;
    doc["mqtt_host"] = settings_->mqttHost;
    doc["mqtt_port"] = settings_->mqttPort;
    doc["mqtt_user"] = settings_->mqttUser;
    doc["mqtt_prefix"] = settings_->mqttPrefix;
    doc["has_mqtt_pass"] = settings_->mqttPass[0] != '\0';
    doc["wind_factor"] = settings_->windMpsPerPps;
    doc["geiger_factor"] = settings_->geigerUsvPerCpm;
    doc["mqtt_connected"] = mqtt_ && mqtt_->connected();
    doc["timezone"] = settings_->timezone;
    doc["wind_unit"] = settings_->windUnit;
    doc["temp_unit"] = settings_->tempUnit;
    doc["pressure_unit"] = settings_->pressureUnit;
    doc["mdns"] = String("http://") + MDNS_HOSTNAME + ".local";
    doc["ntp"] = net_ && net_->timeSynced();
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest* req) {
    int n = WiFi.scanNetworks(false, true);
    JsonDocument doc;
    JsonArray arr = doc["networks"].to<JsonArray>();
    for (int i = 0; i < n; ++i) {
      JsonObject o = arr.add<JsonObject>();
      o["ssid"] = WiFi.SSID(i);
      o["rssi"] = WiFi.RSSI(i);
      o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  auto handleSettingsPost = [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
                                   size_t index, size_t total) {
    if (index == 0) {
      settingsBody = "";
      settingsBody.reserve(total);
    }
    settingsBody.concat(reinterpret_cast<const char*>(data), len);
    if (index + len < total) {
      return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, settingsBody)) {
      req->send(400, "application/json", "{\"ok\":false,\"error\":\"bad json\"}");
      return;
    }

    if (doc["wifi_ssid"].is<const char*>()) {
      strncpy(settings_->wifiSsid, doc["wifi_ssid"] | "", sizeof(settings_->wifiSsid) - 1);
    }
    if (doc["wifi_pass"].is<const char*>()) {
      const char* pass = doc["wifi_pass"] | "";
      if (pass[0] != '\0') {
        strncpy(settings_->wifiPass, pass, sizeof(settings_->wifiPass) - 1);
      }
    }
    if (!doc["mqtt_enabled"].isNull()) {
      settings_->mqttEnabled = doc["mqtt_enabled"].as<bool>();
    }
    if (doc["mqtt_host"].is<const char*>()) {
      strncpy(settings_->mqttHost, doc["mqtt_host"] | "", sizeof(settings_->mqttHost) - 1);
    }
    if (!doc["mqtt_port"].isNull()) {
      settings_->mqttPort = doc["mqtt_port"] | DEFAULT_MQTT_PORT;
    }
    if (doc["mqtt_user"].is<const char*>()) {
      strncpy(settings_->mqttUser, doc["mqtt_user"] | "", sizeof(settings_->mqttUser) - 1);
    }
    if (doc["mqtt_pass"].is<const char*>()) {
      const char* mp = doc["mqtt_pass"] | "";
      if (mp[0] != '\0') {
        strncpy(settings_->mqttPass, mp, sizeof(settings_->mqttPass) - 1);
      }
    }
    if (doc["mqtt_prefix"].is<const char*>()) {
      strncpy(settings_->mqttPrefix, doc["mqtt_prefix"] | DEFAULT_MQTT_PREFIX,
              sizeof(settings_->mqttPrefix) - 1);
    }
    if (!doc["wind_factor"].isNull()) {
      settings_->windMpsPerPps = doc["wind_factor"].as<float>();
    }
    if (!doc["geiger_factor"].isNull()) {
      settings_->geigerUsvPerCpm = doc["geiger_factor"].as<float>();
    }
    if (doc["timezone"].is<const char*>()) {
      strncpy(settings_->timezone, doc["timezone"] | DEFAULT_TIMEZONE,
              sizeof(settings_->timezone) - 1);
    }
    if (doc["wind_unit"].is<const char*>()) {
      strncpy(settings_->windUnit, doc["wind_unit"] | DEFAULT_WIND_UNIT,
              sizeof(settings_->windUnit) - 1);
    }
    if (doc["temp_unit"].is<const char*>()) {
      strncpy(settings_->tempUnit, doc["temp_unit"] | DEFAULT_TEMP_UNIT,
              sizeof(settings_->tempUnit) - 1);
    }
    if (doc["pressure_unit"].is<const char*>()) {
      strncpy(settings_->pressureUnit, doc["pressure_unit"] | DEFAULT_PRESSURE_UNIT,
              sizeof(settings_->pressureUnit) - 1);
    }

    settingsSave(*settings_);
    if (net_) {
      net_->applyTimezone();
    }
    bool reconnect = doc["apply_wifi"] | false;
    req->send(200, "application/json", "{\"ok\":true}");
    if (reconnect && settingsHasWifi(*settings_)) {
      delay(300);
      wifi_->applySavedStation();
    }
  };

  server.on(
      "/api/settings", HTTP_POST,
      [](AsyncWebServerRequest* req) { (void)req; }, nullptr,
      [handleSettingsPost](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index,
                           size_t total) { handleSettingsPost(req, data, len, index, total); });

  server.on("/api/forget-wifi", HTTP_POST, [this](AsyncWebServerRequest* req) {
    req->send(200, "application/json", "{\"ok\":true}");
    delay(200);
    wifi_->forgetWifiAndReboot();
  });

  // Web OTA: multipart file field "firmware", optional query ?mode=fs for LittleFS image
  server.on(
      "/api/ota", HTTP_POST,
      [](AsyncWebServerRequest* req) {
        const bool ok = !Update.hasError();
        AsyncWebServerResponse* res = req->beginResponse(
            ok ? 200 : 500, "application/json",
            ok ? "{\"ok\":true,\"rebooting\":true}" : "{\"ok\":false,\"error\":\"update failed\"}");
        res->addHeader("Connection", "close");
        req->send(res);
        if (ok) {
          delay(500);
          ESP.restart();
        }
      },
      [](AsyncWebServerRequest* req, const String& filename, size_t index, uint8_t* data, size_t len,
         bool final) {
        if (index == 0) {
          int cmd = U_FLASH;
          if (req->hasParam("mode", true)) {
            if (req->getParam("mode", true)->value() == "fs") {
              cmd = U_SPIFFS;
            }
          } else if (req->hasParam("mode")) {
            if (req->getParam("mode")->value() == "fs") {
              cmd = U_SPIFFS;
            }
          } else if (filename.indexOf("littlefs") >= 0 || filename.indexOf("spiffs") >= 0) {
            cmd = U_SPIFFS;
          }
          Serial.printf("[ota] Web upload start: %s (%s)\n", filename.c_str(),
                        cmd == U_FLASH ? "firmware" : "filesystem");
          if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
            Update.printError(Serial);
          }
        }
        if (len && Update.write(data, len) != len) {
          Update.printError(Serial);
        }
        if (final) {
          if (Update.end(true)) {
            Serial.printf("[ota] Web upload success (%u bytes)\n", index + len);
          } else {
            Update.printError(Serial);
          }
        }
      });

  // Captive portal helpers
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->redirect("http://192.168.4.1/");
  });
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->redirect("http://192.168.4.1/");
  });
  server.on("/canonical.html", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->redirect("http://192.168.4.1/");
  });
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->redirect("http://192.168.4.1/");
  });
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->redirect("http://192.168.4.1/");
  });

  server.onNotFound([](AsyncWebServerRequest* req) {
    if (req->method() == HTTP_OPTIONS) {
      req->send(200);
      return;
    }
    if (LittleFS.exists("/www/index.html")) {
      req->redirect("/");
      return;
    }
    req->send(404, "text/plain", "Not found");
  });
}

void WebUi::notifyClients(const SensorReadings& readings) {
  last_ = readings;
  if (millis() - lastWsMs_ < 1000) {
    return;
  }
  lastWsMs_ = millis();
  if (ws.count() == 0) {
    return;
  }
  String json = readingsJson(readings);
  ws.textAll(json);
}

void WebUi::loop(const SensorReadings& readings) {
  notifyClients(readings);
  ws.cleanupClients();

  if (wifi_ && wifi_->isPortalActive()) {
    if (!dnsActive) {
      dnsServer.start(53, "*", WiFi.softAPIP());
      dnsActive = true;
      Serial.println("[dns] Captive DNS started");
    }
    dnsServer.processNextRequest();
  } else if (dnsActive) {
    dnsServer.stop();
    dnsActive = false;
  }
}
