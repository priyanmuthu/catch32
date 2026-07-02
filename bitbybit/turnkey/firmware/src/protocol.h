#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

using HostMessageHandler = void (*)(JsonDocument& msg);

void protocolBegin(Stream& stream);
void protocolPoll(HostMessageHandler handler);
void sendHello();
void sendModeEnter(const char* mode);
void sendRotate(const char* mode, int delta);
void sendPress(const char* mode, const char* action);
void sendFocusUpdate(const char* state, uint32_t remainingSeconds);
void sendMacroRun(const char* id);

