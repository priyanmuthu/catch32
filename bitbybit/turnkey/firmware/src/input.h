#pragma once

#include <Arduino.h>

struct InputEvent {
  enum Type : uint8_t {
    None,
    Rotate,
    Press,
    DoublePress,
    LongPress,
  } type = None;

  int delta = 0;
};

void inputBegin();
InputEvent inputPoll(uint32_t nowMs);

