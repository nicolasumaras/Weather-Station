#pragma once

#include <Arduino.h>

#include "../config.h"
#include "../sensors/readings.h"
#include "../storage/settings.h"

// Bridges the MeshCore gateway (rak3112-meshcore-mqtt-gateway) to the station.
//
// Inbound:  the gateway POSTs every received mesh message to /api/mesh-hook.
//           That runs on the async_tcp task, so it only queues — no HTTP,
//           no blocking.
// Outbound: loop() drains the queue and talks to the gateway's REST API,
//           at most one HTTP request per pass so a slow or unreachable
//           gateway degrades throughput instead of stalling the sensors.

struct MeshRequest {
  char from[32];
  bool direct;
  bool used;
};

// Why a delivery did or did not produce a reply — the caller logs the reason,
// so "nothing happened" is never ambiguous.
enum class MeshAccept {
  Queued,
  NotEnabled,
  NoTrigger,
  CoolingDown,
  QueueFull,
};

struct SenderState {
  char name[32];
  uint32_t lastMs;
  bool used;
};

class MeshBridge {
 public:
  void begin(StationSettings* settings);

  // Called from the web handler; never blocks.
  MeshAccept enqueue(const char* from, const char* text, bool direct);

  // Seconds left before this sender may trigger again; 0 when ready.
  uint32_t cooldownRemaining(const char* from) const;
  // Same, but resolves the real sender first (public deliveries carry it in
  // the text rather than the from field).
  uint32_t cooldownRemainingFor(const char* from, const char* text) const;

  // Called from the main loop. Performs at most one HTTP request.
  void loop(const SensorReadings& readings);

  // Accepting a delivery only needs the bridge switched on. Replying also
  // needs somewhere to send it — keep the two separate so a missing gateway
  // host does not make inbound deliveries look rejected.
  bool inboundEnabled() const;
  bool canReply() const;

  uint32_t repliesSent() const { return sent_; }
  uint32_t failures() const { return failed_; }
  uint32_t hooksAccepted() const { return hooksAccepted_; }
  uint32_t hooksRejected() const { return hooksRejected_; }
  void noteRejected() { hooksRejected_++; }
  const char* lastError() const { return lastError_; }

 private:
  enum class Stage { Idle, ResolveContact, Send };

  bool matchesKeyword(const char* text) const;
  SenderState* findSender(const char* name);
  const SenderState* findSender(const char* name) const;
  SenderState* claimSenderSlot(const char* name);
  size_t buildReply(const SensorReadings& r, char* out, size_t outLen) const;
  bool resolveContactId(const char* name, int* idOut);
  bool postMessage(const char* text, int to);
  String baseUrl() const;

  StationSettings* settings_ = nullptr;
  MeshRequest queue_[MESH_QUEUE_SLOTS];
  size_t head_ = 0;
  size_t tail_ = 0;
  Stage stage_ = Stage::Idle;
  MeshRequest active_{};
  int activeTo_ = -1;
  SenderState senders_[MESH_SENDER_SLOTS]{};
  uint32_t sent_ = 0;
  uint32_t failed_ = 0;
  uint32_t hooksAccepted_ = 0;
  uint32_t hooksRejected_ = 0;
  char lastError_[64] = {0};
};
