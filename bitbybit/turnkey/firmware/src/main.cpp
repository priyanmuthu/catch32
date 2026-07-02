#include <Arduino.h>
#include <lvgl.h>

#include "app_mode.h"
#include "board_config.h"
#include "display.h"
#include "input.h"
#include "protocol.h"

namespace {

class HomeMode final : public AppMode {
public:
  const char* id() const override { return "home"; }
  const char* label() const override { return "TurnKey"; }
  const char* icon() const override { return LV_SYMBOL_HOME; }

  void onRotate(int delta) override;
  void onPress() override;
  void draw(lv_obj_t* root) override;

  uint8_t selected = 0;
};

class CommandMode final : public AppMode {
public:
  const char* id() const override { return "command"; }
  const char* label() const override { return "Command"; }
  const char* icon() const override { return LV_SYMBOL_AUDIO; }

  void onEnter() override { sendModeEnter(id()); }
  void onRotate(int delta) override {
    value = constrain(value + delta * 2, 0, 100);
  }
  void onPress() override {
    muted = !muted;
    sendPress(id(), "toggle_mute");
  }
  void onHostMessage(JsonDocument& msg) override {
    if (strcmp(msg["type"] | "", "state.command") != 0) {
      return;
    }
    value = msg["value"] | value;
    muted = msg["muted"] | muted;
  }
  void draw(lv_obj_t* root) override;

private:
  int value = 50;
  bool muted = false;
};

class FocusMode final : public AppMode {
public:
  const char* id() const override { return "focus"; }
  const char* label() const override { return "Focus"; }
  const char* icon() const override { return LV_SYMBOL_PLAY; }

  void onEnter() override {
    sendModeEnter(id());
    publish();
  }
  void onRotate(int delta) override;
  void onPress() override;
  void onDoublePress() override;
  void tick(uint32_t nowMs) override;
  void draw(lv_obj_t* root) override;

private:
  static constexpr uint16_t PRESETS[] = {25, 5, 15};
  static constexpr const char* PRESET_LABELS[] = {"Focus", "Break", "Long"};

  void publish();

  uint8_t preset = 0;
  bool running = false;
  uint32_t remainingSeconds = PRESETS[0] * 60;
  uint32_t lastTickMs = 0;
};

constexpr uint16_t FocusMode::PRESETS[];
constexpr const char* FocusMode::PRESET_LABELS[];

class MeetingMode final : public AppMode {
public:
  const char* id() const override { return "meeting"; }
  const char* label() const override { return "Meeting"; }
  const char* icon() const override { return LV_SYMBOL_CALL; }

  void onEnter() override { sendModeEnter(id()); }
  void onRotate(int delta) override {
    selectedStatus = (selectedStatus + 3 + (delta > 0 ? 1 : -1)) % 3;
    sendPress(id(), STATUS_IDS[selectedStatus]);
  }
  void onPress() override {
    muted = !muted;
    sendPress(id(), "toggle_mute");
  }
  void onHostMessage(JsonDocument& msg) override {
    if (strcmp(msg["type"] | "", "state.meeting") != 0) {
      return;
    }
    muted = msg["muted"] | muted;
    const char* status = msg["status"] | "";
    for (uint8_t i = 0; i < 3; i++) {
      if (strcmp(status, STATUS_IDS[i]) == 0) {
        selectedStatus = i;
      }
    }
  }
  void draw(lv_obj_t* root) override;

private:
  static constexpr const char* STATUS_IDS[] = {"available", "focus", "in_meeting"};
  static constexpr const char* STATUS_LABELS[] = {"Available", "Focus", "Meeting"};

  uint8_t selectedStatus = 0;
  bool muted = false;
};

constexpr const char* MeetingMode::STATUS_IDS[];
constexpr const char* MeetingMode::STATUS_LABELS[];

class CodingMode final : public AppMode {
public:
  const char* id() const override { return "coding"; }
  const char* label() const override { return "Code"; }
  const char* icon() const override { return LV_SYMBOL_LIST; }

  void onEnter() override { sendModeEnter(id()); }
  void onRotate(int delta) override {
    selectedMacro = (selectedMacro + MACRO_COUNT + (delta > 0 ? 1 : -1)) % MACRO_COUNT;
  }
  void onPress() override {
    lastStatus = "Running";
    sendMacroRun(MACRO_IDS[selectedMacro]);
  }
  void onDoublePress() override {
    lastStatus = "Repeat";
    sendMacroRun(MACRO_IDS[selectedMacro]);
  }
  void onHostMessage(JsonDocument& msg) override {
    if (strcmp(msg["type"] | "", "state.macro") != 0) {
      return;
    }
    lastStatus = msg["label"] | msg["status"] | lastStatus;
  }
  void draw(lv_obj_t* root) override;

private:
  static constexpr uint8_t MACRO_COUNT = 4;
  static constexpr const char* MACRO_IDS[] = {"git_status", "test_project", "build_project", "open_terminal"};
  static constexpr const char* MACRO_LABELS[] = {"Git", "Test", "Build", "Terminal"};

  uint8_t selectedMacro = 0;
  const char* lastStatus = "Ready";
};

constexpr const char* CodingMode::MACRO_IDS[];
constexpr const char* CodingMode::MACRO_LABELS[];

HomeMode homeMode;
CommandMode commandMode;
FocusMode focusMode;
MeetingMode meetingMode;
CodingMode codingMode;

AppMode* modes[] = {&homeMode, &commandMode, &focusMode, &meetingMode, &codingMode};
AppMode* launcherModes[] = {&commandMode, &focusMode, &meetingMode, &codingMode};
AppMode* current = &homeMode;
bool redraw = true;
uint32_t lastHelloMs = 0;

void clearRoot(lv_obj_t* root) {
  lv_obj_clean(root);
  lv_obj_set_style_bg_color(root, lv_color_hex(0x101418), 0);
}

lv_obj_t* addLabel(lv_obj_t* parent, const char* text, lv_coord_t x, lv_coord_t y, const lv_font_t* font, uint32_t color) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  lv_obj_align(label, LV_ALIGN_CENTER, x, y);
  return label;
}

void switchTo(AppMode* next) {
  if (current == next) {
    return;
  }
  current->onExit();
  current = next;
  current->onEnter();
  redraw = true;
}

void handleHostMessage(JsonDocument& msg) {
  if (strcmp(msg["type"] | "", "hello") == 0) {
    displaySetHostConnected(true);
  }
  for (AppMode* mode : modes) {
    mode->onHostMessage(msg);
  }
  redraw = true;
}

} // namespace

const char* modeIdToString(ModeId mode) {
  switch (mode) {
    case ModeId::Home:
      return "home";
    case ModeId::Command:
      return "command";
    case ModeId::Focus:
      return "focus";
    case ModeId::Meeting:
      return "meeting";
    case ModeId::Coding:
      return "coding";
  }
  return "unknown";
}

void HomeMode::onRotate(int delta) {
  selected = (selected + 4 + (delta > 0 ? 1 : -1)) % 4;
}

void HomeMode::onPress() {
  switchTo(launcherModes[selected]);
}

void HomeMode::draw(lv_obj_t* root) {
  clearRoot(root);
  addLabel(root, "TurnKey", 0, -18, &lv_font_montserrat_22, 0xf6f2e8);

  const lv_coord_t xs[] = {116, 0, -116, 0};
  const lv_coord_t ys[] = {0, 108, 0, -108};
  for (uint8_t i = 0; i < 4; i++) {
    bool active = i == selected;
    lv_obj_t* iconLabel = addLabel(root, launcherModes[i]->icon(), xs[i], ys[i], &lv_font_montserrat_32, active ? 0x53d6a3 : 0x8e9aa3);
    if (active) {
      lv_obj_set_style_text_font(iconLabel, &lv_font_montserrat_40, 0);
    }
  }

  addLabel(root, launcherModes[selected]->label(), 0, 46, &lv_font_montserrat_18, 0xffffff);
  addLabel(root, displayHostConnected() ? LV_SYMBOL_USB : LV_SYMBOL_CLOSE, 0, 142, &lv_font_montserrat_18, displayHostConnected() ? 0x53d6a3 : 0xff6f61);
}

void CommandMode::draw(lv_obj_t* root) {
  clearRoot(root);
  addLabel(root, muted ? LV_SYMBOL_MUTE : LV_SYMBOL_AUDIO, 0, -78, &lv_font_montserrat_40, muted ? 0xff6f61 : 0x53d6a3);
  char valueText[12];
  snprintf(valueText, sizeof(valueText), "%d%%", value);
  addLabel(root, valueText, 0, 0, &lv_font_montserrat_40, 0xffffff);
  addLabel(root, muted ? "Muted" : "Audio", 0, 56, &lv_font_montserrat_18, 0xaeb8bf);
}

void FocusMode::onRotate(int delta) {
  if (running) {
    return;
  }
  preset = (preset + 3 + (delta > 0 ? 1 : -1)) % 3;
  remainingSeconds = PRESETS[preset] * 60;
  publish();
}

void FocusMode::onPress() {
  running = !running;
  lastTickMs = millis();
  publish();
}

void FocusMode::onDoublePress() {
  running = false;
  remainingSeconds = PRESETS[preset] * 60;
  publish();
}

void FocusMode::tick(uint32_t nowMs) {
  if (!running) {
    return;
  }
  if (lastTickMs == 0) {
    lastTickMs = nowMs;
    return;
  }
  while ((nowMs - lastTickMs) >= 1000 && remainingSeconds > 0) {
    remainingSeconds--;
    lastTickMs += 1000;
  }
  if (remainingSeconds == 0) {
    running = false;
    tone(board::BUZZER, 2200, 180);
    publish();
  }
}

void FocusMode::publish() {
  sendFocusUpdate(running ? "running" : "stopped", remainingSeconds);
}

void FocusMode::draw(lv_obj_t* root) {
  clearRoot(root);
  addLabel(root, running ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY, 0, -82, &lv_font_montserrat_40, running ? 0x53d6a3 : 0xf0c85a);
  char timeText[16];
  snprintf(timeText, sizeof(timeText), "%02lu:%02lu", remainingSeconds / 60, remainingSeconds % 60);
  addLabel(root, timeText, 0, -4, &lv_font_montserrat_40, 0xffffff);
  addLabel(root, PRESET_LABELS[preset], 0, 58, &lv_font_montserrat_18, 0xaeb8bf);
}

void MeetingMode::draw(lv_obj_t* root) {
  clearRoot(root);
  addLabel(root, muted ? LV_SYMBOL_MUTE : LV_SYMBOL_CALL, 0, -70, &lv_font_montserrat_40, muted ? 0xff6f61 : 0x53d6a3);
  addLabel(root, STATUS_LABELS[selectedStatus], 0, 4, &lv_font_montserrat_28, 0xffffff);
  addLabel(root, muted ? "Muted" : "Live", 0, 58, &lv_font_montserrat_18, 0xaeb8bf);
}

void CodingMode::draw(lv_obj_t* root) {
  clearRoot(root);
  addLabel(root, LV_SYMBOL_LIST, 0, -86, &lv_font_montserrat_40, 0x53d6a3);
  addLabel(root, MACRO_LABELS[selectedMacro], 0, -6, &lv_font_montserrat_32, 0xffffff);
  addLabel(root, lastStatus, 0, 52, &lv_font_montserrat_18, 0xaeb8bf);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(board::BUZZER, OUTPUT);
  inputBegin();
  displayBegin();
  protocolBegin(Serial);
  sendHello();
  lastHelloMs = millis();
}

void loop() {
  uint32_t now = millis();

  protocolPoll(handleHostMessage);

  InputEvent event = inputPoll(now);
  if (event.type != InputEvent::None) {
    if (event.type == InputEvent::LongPress && current != &homeMode) {
      switchTo(&homeMode);
    } else if (event.type == InputEvent::DoublePress && current != &homeMode) {
      current->onDoublePress();
    } else if (event.type == InputEvent::Press) {
      current->onPress();
    } else if (event.type == InputEvent::Rotate) {
      current->onRotate(event.delta);
      if (current != &homeMode) {
        sendRotate(current->id(), event.delta);
      }
    }
    redraw = true;
  }

  current->tick(now);

  if ((now - lastHelloMs) > 5000) {
    sendHello();
    lastHelloMs = now;
  }

  if (redraw) {
    current->draw(displayRoot());
    redraw = false;
  }

  displayTick();
  delay(5);
}
