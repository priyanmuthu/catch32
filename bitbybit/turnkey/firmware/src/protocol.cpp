#include "protocol.h"

namespace {
Stream* io = nullptr;
String line;

void sendJson(JsonDocument& doc) {
  if (!io) {
    return;
  }
  serializeJson(doc, *io);
  io->println();
}
}

void protocolBegin(Stream& stream) {
  io = &stream;
  line.reserve(256);
}

void protocolPoll(HostMessageHandler handler) {
  if (!io) {
    return;
  }

  while (io->available()) {
    char c = static_cast<char>(io->read());
    if (c == '\r') {
      continue;
    }
    if (c != '\n') {
      if (line.length() < 512) {
        line += c;
      }
      continue;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, line);
    line = "";
    if (!err && handler) {
      handler(doc);
    }
  }
}

void sendHello() {
  JsonDocument doc;
  doc["type"] = "hello";
  doc["device"] = "t-encoder-pro";
  doc["fw"] = "0.1.0";
  sendJson(doc);
}

void sendModeEnter(const char* mode) {
  JsonDocument doc;
  doc["type"] = "mode.enter";
  doc["mode"] = mode;
  sendJson(doc);
}

void sendRotate(const char* mode, int delta) {
  JsonDocument doc;
  doc["type"] = "input.rotate";
  doc["mode"] = mode;
  doc["delta"] = delta;
  sendJson(doc);
}

void sendPress(const char* mode, const char* action) {
  JsonDocument doc;
  doc["type"] = "input.press";
  doc["mode"] = mode;
  doc["action"] = action;
  sendJson(doc);
}

void sendFocusUpdate(const char* state, uint32_t remainingSeconds) {
  JsonDocument doc;
  doc["type"] = "focus.update";
  doc["state"] = state;
  doc["remaining_s"] = remainingSeconds;
  sendJson(doc);
}

void sendMacroRun(const char* id) {
  JsonDocument doc;
  doc["type"] = "macro.run";
  doc["id"] = id;
  sendJson(doc);
}

