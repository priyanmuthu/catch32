#include "input.h"

#include "board_config.h"

namespace {
constexpr uint32_t DEBOUNCE_MS = 25;
constexpr uint32_t DOUBLE_PRESS_MS = 350;
constexpr uint32_t LONG_PRESS_MS = 800;

uint8_t lastEncoded = 0;
bool lastButton = true;
bool stableButton = true;
bool longPressSent = false;
uint32_t lastButtonChange = 0;
uint32_t pressStart = 0;
uint32_t lastRelease = 0;
}

void inputBegin() {
  pinMode(board::ENCODER_A, INPUT_PULLUP);
  pinMode(board::ENCODER_B, INPUT_PULLUP);
  pinMode(board::ENCODER_KEY, INPUT_PULLUP);
  lastEncoded = (digitalRead(board::ENCODER_A) << 1) | digitalRead(board::ENCODER_B);
}

InputEvent inputPoll(uint32_t nowMs) {
  InputEvent event;

  uint8_t encoded = (digitalRead(board::ENCODER_A) << 1) | digitalRead(board::ENCODER_B);
  uint8_t sum = (lastEncoded << 2) | encoded;
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    event.type = InputEvent::Rotate;
    event.delta = 1;
    lastEncoded = encoded;
    return event;
  }
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    event.type = InputEvent::Rotate;
    event.delta = -1;
    lastEncoded = encoded;
    return event;
  }
  lastEncoded = encoded;

  bool rawButton = digitalRead(board::ENCODER_KEY);
  if (rawButton != lastButton) {
    lastButton = rawButton;
    lastButtonChange = nowMs;
  }

  if ((nowMs - lastButtonChange) < DEBOUNCE_MS || rawButton == stableButton) {
    if (!stableButton && !longPressSent && (nowMs - pressStart) >= LONG_PRESS_MS) {
      longPressSent = true;
      event.type = InputEvent::LongPress;
    }
    return event;
  }

  stableButton = rawButton;
  if (!stableButton) {
    pressStart = nowMs;
    longPressSent = false;
    return event;
  }

  if (longPressSent) {
    return event;
  }

  if ((nowMs - lastRelease) <= DOUBLE_PRESS_MS) {
    lastRelease = 0;
    event.type = InputEvent::DoublePress;
  } else {
    lastRelease = nowMs;
    event.type = InputEvent::Press;
  }

  return event;
}

