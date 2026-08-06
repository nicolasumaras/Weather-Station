#include "mesh_bridge.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include <cctype>
#include <cstring>

#include "../sensors/aqi.h"
#include "event_log.h"

namespace {

// Case-insensitive substring search; avoids pulling in strcasestr.
bool containsFold(const char* haystack, const char* needle) {
  if (!haystack || !needle || !needle[0]) {
    return false;
  }
  const size_t nlen = strlen(needle);
  for (const char* p = haystack; *p; ++p) {
    size_t i = 0;
    while (i < nlen && p[i] && tolower(static_cast<unsigned char>(p[i])) ==
                                   tolower(static_cast<unsigned char>(needle[i]))) {
      ++i;
    }
    if (i == nlen) {
      return true;
    }
  }
  return false;
}

// Public-channel deliveries all arrive with from="(public)" and the real
// sender prefixed into the text as "Name: message" (confirmed by the
// gateway's own API docs and by live traffic). Without unwrapping that, every
// public asker shares one rate-limit bucket and starves the others.
void deriveSender(const char* from, const char* text, char* out, size_t outLen) {
  out[0] = '\0';
  if (from && from[0] && from[0] != '(') {
    strncpy(out, from, outLen - 1);
    out[outLen - 1] = '\0';
    return;
  }
  const char* sep = text ? strstr(text, ": ") : nullptr;
  if (sep && sep != text) {
    size_t len = static_cast<size_t>(sep - text);
    if (len > outLen - 1) {
      len = outLen - 1;
    }
    memcpy(out, text, len);
    out[len] = '\0';
    return;
  }
  strncpy(out, from ? from : "", outLen - 1);
  out[outLen - 1] = '\0';
}

}  // namespace

void MeshBridge::begin(StationSettings* settings) {
  settings_ = settings;
  head_ = tail_ = 0;
  stage_ = Stage::Idle;
}

bool MeshBridge::inboundEnabled() const {
  return settings_ && settings_->meshEnabled;
}

bool MeshBridge::canReply() const {
  return inboundEnabled() && settings_->meshHost[0] != '\0';
}

String MeshBridge::baseUrl() const {
  return String("http://") + settings_->meshHost;
}

bool MeshBridge::matchesKeyword(const char* text) const {
  return containsFold(text, settings_->meshKeyword);
}

SenderState* MeshBridge::findSender(const char* name) {
  for (auto& s : senders_) {
    if (s.used && strcmp(s.name, name) == 0) {
      return &s;
    }
  }
  return nullptr;
}

const SenderState* MeshBridge::findSender(const char* name) const {
  for (const auto& s : senders_) {
    if (s.used && strcmp(s.name, name) == 0) {
      return &s;
    }
  }
  return nullptr;
}

// Returns the slot for this sender, reusing a free one or evicting whoever we
// heard from longest ago. The table only bounds memory; evicting simply means
// that sender's next request is treated as their first.
SenderState* MeshBridge::claimSenderSlot(const char* name) {
  if (SenderState* existing = findSender(name)) {
    return existing;
  }
  SenderState* victim = nullptr;
  for (auto& s : senders_) {
    if (!s.used) {
      victim = &s;
      break;
    }
    if (!victim || static_cast<int32_t>(s.lastMs - victim->lastMs) < 0) {
      victim = &s;
    }
  }
  strncpy(victim->name, name, sizeof(victim->name) - 1);
  victim->name[sizeof(victim->name) - 1] = '\0';
  victim->lastMs = 0;
  victim->used = true;
  return victim;
}

uint32_t MeshBridge::cooldownRemaining(const char* from) const {
  if (!settings_ || settings_->meshCooldownSec == 0) {
    return 0;
  }
  const SenderState* s = findSender(from ? from : "");
  if (!s || s->lastMs == 0) {
    return 0;
  }
  const uint32_t windowMs = static_cast<uint32_t>(settings_->meshCooldownSec) * 1000UL;
  const uint32_t elapsed = millis() - s->lastMs;
  return (elapsed >= windowMs) ? 0 : ((windowMs - elapsed) + 999) / 1000;
}

uint32_t MeshBridge::cooldownRemainingFor(const char* from, const char* text) const {
  char name[32];
  deriveSender(from, text, name, sizeof(name));
  return cooldownRemaining(name);
}

MeshAccept MeshBridge::enqueue(const char* from, const char* text, bool direct) {
  if (!inboundEnabled()) {
    return MeshAccept::NotEnabled;
  }
  hooksAccepted_++;
  if (!matchesKeyword(text)) {
    return MeshAccept::NoTrigger;
  }

  char name[32];
  deriveSender(from, text, name, sizeof(name));
  // Rate limit per sender: one person asking repeatedly must not deny everyone
  // else an answer, which a single global window did.
  if (cooldownRemaining(name) > 0) {
    return MeshAccept::CoolingDown;
  }

  const size_t next = (head_ + 1) % MESH_QUEUE_SLOTS;
  if (next == tail_) {
    return MeshAccept::QueueFull;
  }
  MeshRequest& e = queue_[head_];
  strncpy(e.from, name, sizeof(e.from) - 1);
  e.from[sizeof(e.from) - 1] = '\0';
  e.direct = direct;
  e.used = true;
  head_ = next;

  // Start this sender's window at acceptance, not delivery, so a burst
  // collapses into one reply instead of queueing several.
  SenderState* slot = claimSenderSlot(name);
  slot->lastMs = millis();
  if (slot->lastMs == 0) {
    slot->lastMs = 1;  // 0 is the "never replied" sentinel
  }
  return MeshAccept::Queued;
}

size_t MeshBridge::buildReply(const SensorReadings& r, char* out, size_t outLen) const {
  // Deliberately avoids the trigger keyword: the reply travels the same mesh,
  // and another responder echoing it must not start a loop.
  const bool fahrenheit = strcmp(settings_->tempUnit, "F") == 0;
  const float temp = fahrenheit ? (r.temperatureC * 9.0f / 5.0f + 32.0f) : r.temperatureC;
  const char* tempUnit = fahrenheit ? "F" : "C";

  float wind = r.windMps;
  const char* windUnit = "m/s";
  if (strcmp(settings_->windUnit, "kmh") == 0) {
    wind = r.windMps * 3.6f;
    windUnit = "km/h";
  } else if (strcmp(settings_->windUnit, "mph") == 0) {
    wind = r.windMps * 2.23694f;
    windUnit = "mph";
  }

  const int aqi = r.pmValid ? aqiOverall(r.pm2_5, r.pm10) : -1;

  size_t n = 0;
  n += snprintf(out + n, outLen - n, "Station:");
  if (r.bmeValid) {
    n += snprintf(out + n, outLen - n, " %.1f%s %.0f%%", temp, tempUnit, r.humidityPct);
  } else {
    n += snprintf(out + n, outLen - n, " temp n/a");
  }
  if (aqi >= 0) {
    n += snprintf(out + n, outLen - n, " AQI %d %s", aqi, aqiCategory(aqi));
  } else {
    n += snprintf(out + n, outLen - n, " AQI n/a");
  }
  if (r.geigerValid) {
    n += snprintf(out + n, outLen - n, " %.3fuSv/h", r.usvPerHour);
  } else {
    n += snprintf(out + n, outLen - n, " rad n/a");
  }
  if (r.windValid) {
    n += snprintf(out + n, outLen - n, " wind %.1f%s", wind, windUnit);
  }
  return n;
}

bool MeshBridge::resolveContactId(const char* name, int* idOut) {
  HTTPClient http;
  http.setTimeout(MESH_HTTP_TIMEOUT_MS);
  http.setConnectTimeout(MESH_HTTP_TIMEOUT_MS);
  if (!http.begin(baseUrl() + "/api/contacts")) {
    strncpy(lastError_, "contacts: begin failed", sizeof(lastError_) - 1);
    return false;
  }
  http.setAuthorization("admin", settings_->meshAdminPass);
  const int code = http.GET();
  if (code != 200) {
    snprintf(lastError_, sizeof(lastError_), "contacts: HTTP %d", code);
    meshLog.add('>', "GET /api/contacts", code, "lookup for \"%s\" failed", name);
    http.end();
    return false;
  }

  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    strncpy(lastError_, "contacts: bad json", sizeof(lastError_) - 1);
    return false;
  }

  for (JsonObject c : doc.as<JsonArray>()) {
    const char* cname = c["name"] | "";
    if (strcmp(cname, name) == 0) {
      *idOut = c["id"] | -1;
      meshLog.add('>', "GET /api/contacts", 200, "\"%s\" -> id %d", name, *idOut);
      return *idOut >= 0;
    }
  }
  strncpy(lastError_, "contacts: sender not known", sizeof(lastError_) - 1);
  meshLog.add('>', "GET /api/contacts", 200, "\"%s\" not in contact list", name);
  return false;
}

bool MeshBridge::postMessage(const char* text, int to) {
  HTTPClient http;
  http.setTimeout(MESH_HTTP_TIMEOUT_MS);
  http.setConnectTimeout(MESH_HTTP_TIMEOUT_MS);
  if (!http.begin(baseUrl() + "/api/messages")) {
    strncpy(lastError_, "send: begin failed", sizeof(lastError_) - 1);
    return false;
  }
  http.setAuthorization("admin", settings_->meshAdminPass);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["text"] = text;
  doc["to"] = to;
  String body;
  serializeJson(doc, body);

  const int code = http.POST(body);
  http.end();
  if (code < 200 || code >= 300) {
    snprintf(lastError_, sizeof(lastError_), "send: HTTP %d", code);
    meshLog.add('>', "POST /api/messages", code, "to=%d failed", to);
    return false;
  }
  meshLog.add('>', "POST /api/messages", code, "to=%d \"%s\"", to, text);
  return true;
}

void MeshBridge::loop(const SensorReadings& readings) {
  if (!inboundEnabled() || WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!canReply()) {
    // Accepted the trigger but have nowhere to send the answer. Say so once
    // per queued item rather than silently holding them forever.
    while (head_ != tail_) {
      tail_ = (tail_ + 1) % MESH_QUEUE_SLOTS;
      failed_++;
      strncpy(lastError_, "no gateway host configured", sizeof(lastError_) - 1);
      Serial.println("[mesh] trigger received but no gateway host is set");
    }
    return;
  }

  if (stage_ == Stage::Idle) {
    if (head_ == tail_) {
      return;
    }
    active_ = queue_[tail_];
    tail_ = (tail_ + 1) % MESH_QUEUE_SLOTS;
    // Public replies need no lookup; direct ones need the sender's contact id.
    activeTo_ = -1;
    stage_ = active_.direct ? Stage::ResolveContact : Stage::Send;
    return;  // one HTTP request per pass, starting next time round
  }

  if (stage_ == Stage::ResolveContact) {
    int id = -1;
    if (resolveContactId(active_.from, &id)) {
      activeTo_ = id;
    } else {
      // Fall back to the public channel rather than dropping the answer.
      Serial.printf("[mesh] %s, replying on public channel\n", lastError_);
      activeTo_ = -1;
    }
    stage_ = Stage::Send;
    return;
  }

  char reply[MESH_REPLY_MAX];
  buildReply(readings, reply, sizeof(reply));
  if (postMessage(reply, activeTo_)) {
    sent_++;
    Serial.printf("[mesh] replied to %s (to=%d): %s\n", active_.from, activeTo_, reply);
  } else {
    failed_++;
    Serial.printf("[mesh] reply failed: %s\n", lastError_);
  }
  stage_ = Stage::Idle;
}
