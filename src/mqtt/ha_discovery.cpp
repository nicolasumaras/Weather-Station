#include "ha_discovery.h"

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "../config.h"

namespace {
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
}

void HaMqtt::begin(StationSettings* settings) {
  settings_ = settings;
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char id[20];
  snprintf(id, sizeof(id), "ws%02x%02x%02x", mac[3], mac[4], mac[5]);
  deviceId_ = id;
  statePrefix_ = String(settings_->mqttPrefix[0] ? settings_->mqttPrefix : DEFAULT_MQTT_PREFIX) +
                 "/" + deviceId_;
  availTopic_ = statePrefix_ + "/status";
  mqtt.setBufferSize(1024);
}

bool HaMqtt::connected() const {
  return mqtt.connected();
}

String HaMqtt::stateTopic() const {
  return statePrefix_ + "/state";
}

void HaMqtt::ensureConnected() {
  if (!settings_ || !settings_->mqttEnabled || settings_->mqttHost[0] == '\0') {
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  if (mqtt.connected()) {
    return;
  }
  if (millis() - lastReconnectMs_ < MQTT_RECONNECT_MS) {
    return;
  }
  lastReconnectMs_ = millis();

  mqtt.setServer(settings_->mqttHost, settings_->mqttPort);
  String clientId = "weather-station-" + deviceId_;
  bool ok;
  if (settings_->mqttUser[0] != '\0') {
    ok = mqtt.connect(clientId.c_str(), settings_->mqttUser, settings_->mqttPass,
                      availTopic_.c_str(), 0, true, "offline");
  } else {
    ok = mqtt.connect(clientId.c_str(), nullptr, nullptr, availTopic_.c_str(), 0, true,
                      "offline");
  }
  if (ok) {
    Serial.println("[mqtt] Connected");
    mqtt.publish(availTopic_.c_str(), "online", true);
    discoverySent_ = false;
    publishDiscovery();
  } else {
    Serial.printf("[mqtt] Connect failed rc=%d\n", mqtt.state());
  }
}

void HaMqtt::publishSensorDiscovery(const char* objectId, const char* name,
                                    const char* deviceClass, const char* unit,
                                    const char* stateKey, int decimals) {
  String topic = "homeassistant/sensor/" + deviceId_ + "/" + objectId + "/config";
  JsonDocument doc;
  doc["name"] = name;
  doc["uniq_id"] = deviceId_ + "_" + objectId;
  doc["stat_t"] = stateTopic();
  doc["avty_t"] = availTopic_;
  doc["pl_avail"] = "online";
  doc["pl_not_avail"] = "offline";
  doc["val_tpl"] = String("{{ value_json.") + stateKey + " }}";
  if (unit && unit[0]) {
    doc["unit_of_meas"] = unit;
  }
  if (deviceClass && deviceClass[0]) {
    doc["dev_cla"] = deviceClass;
  }
  doc["stat_cla"] = "measurement";
  if (decimals >= 0) {
    // force float display hints via suggested display precision when supported
  }
  JsonObject dev = doc["dev"].to<JsonObject>();
  dev["ids"][0] = deviceId_;
  dev["name"] = DEVICE_NAME;
  dev["mdl"] = DEVICE_MODEL;
  dev["mf"] = "Weather-Station";
  dev["sw"] = FW_VERSION;

  String payload;
  serializeJson(doc, payload);
  mqtt.publish(topic.c_str(), payload.c_str(), true);
}

void HaMqtt::publishDiscovery() {
  if (!mqtt.connected()) {
    return;
  }
  publishSensorDiscovery("pm25", "PM2.5", "pm25", "µg/m³", "pm2_5", 0);
  publishSensorDiscovery("pm10", "PM10", "pm10", "µg/m³", "pm10", 0);
  publishSensorDiscovery("pm1", "PM1.0", "pm1", "µg/m³", "pm1_0", 0);
  publishSensorDiscovery("wind", "Wind Speed", "wind_speed", "m/s", "wind_mps", 2);
  publishSensorDiscovery("cpm", "Radiation CPM", "", "CPM", "cpm", 1);
  publishSensorDiscovery("dose", "Radiation Dose", "", "µSv/h", "usv_h", 3);
  publishSensorDiscovery("temperature", "Temperature", "temperature", "°C", "temperature", 1);
  publishSensorDiscovery("humidity", "Humidity", "humidity", "%", "humidity", 1);
  publishSensorDiscovery("pressure", "Pressure", "atmospheric_pressure", "hPa", "pressure", 1);
  publishSensorDiscovery("rssi", "WiFi RSSI", "signal_strength", "dBm", "rssi", 0);
  discoverySent_ = true;
  Serial.println("[mqtt] HA discovery published");
}

void HaMqtt::loop(const SensorReadings& readings) {
  if (!settings_ || !settings_->mqttEnabled || settings_->mqttHost[0] == '\0') {
    return;
  }
  ensureConnected();
  if (!mqtt.connected()) {
    return;
  }
  mqtt.loop();

  if (!discoverySent_) {
    publishDiscovery();
  }

  if (millis() - lastPublishMs_ < 5000) {
    return;
  }
  lastPublishMs_ = millis();

  JsonDocument doc;
  if (readings.pmValid) {
    doc["pm1_0"] = readings.pm1_0;
    doc["pm2_5"] = readings.pm2_5;
    doc["pm10"] = readings.pm10;
  }
  if (readings.windValid) {
    doc["wind_mps"] = readings.windMps;
  }
  if (readings.geigerValid) {
    doc["cpm"] = readings.cpm;
    doc["usv_h"] = readings.usvPerHour;
  }
  if (readings.bmeValid) {
    doc["temperature"] = readings.temperatureC;
    doc["humidity"] = readings.humidityPct;
    doc["pressure"] = readings.pressureHpa;
  }
  doc["rssi"] = readings.wifiRssi;

  String payload;
  serializeJson(doc, payload);
  mqtt.publish(stateTopic().c_str(), payload.c_str(), false);
  mqtt.publish(availTopic_.c_str(), "online", true);
}
