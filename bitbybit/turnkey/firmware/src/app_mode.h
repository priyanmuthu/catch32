#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <lvgl.h>

class AppMode {
public:
  virtual ~AppMode() = default;

  virtual const char* id() const = 0;
  virtual const char* label() const = 0;
  virtual const char* icon() const = 0;

  virtual void onEnter() {}
  virtual void onExit() {}
  virtual void onRotate(int delta) {}
  virtual void onPress() {}
  virtual void onDoublePress() {}
  virtual void onLongPress() {}
  virtual void onTouch(int x, int y) {}
  virtual void onHostMessage(JsonDocument& msg) {}
  virtual void tick(uint32_t nowMs) {}
  virtual void draw(lv_obj_t* root) = 0;
};

enum class ModeId : uint8_t {
  Home,
  Command,
  Focus,
  Meeting,
  Coding,
};

const char* modeIdToString(ModeId mode);

