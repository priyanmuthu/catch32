#include <Arduino.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>

namespace {

constexpr uint32_t kFocusSeconds = 25 * 60;
constexpr uint32_t kBreakSeconds = 5 * 60;

LilyGo_Class amoled;

lv_obj_t *screen;
lv_obj_t *modeLabel;
lv_obj_t *timerLabel;
lv_obj_t *statusLabel;
lv_obj_t *batteryLabel;
lv_obj_t *startPauseButton;
lv_obj_t *startPauseLabel;
lv_obj_t *resetButton;
lv_obj_t *skipButton;
lv_obj_t *progressBar;

enum class TimerMode {
  Focus,
  Break,
};

TimerMode currentMode = TimerMode::Focus;
bool running = false;
uint32_t remainingSeconds = kFocusSeconds;
uint32_t lastTickMs = 0;
uint32_t lastBatteryMs = 0;

uint32_t durationForMode(TimerMode mode) {
  return mode == TimerMode::Focus ? kFocusSeconds : kBreakSeconds;
}

const char *nameForMode(TimerMode mode) {
  return mode == TimerMode::Focus ? "Focus" : "Break";
}

const char *promptForMode(TimerMode mode) {
  return mode == TimerMode::Focus ? "Work on one thing" : "Step away and reset";
}

void setMode(TimerMode mode) {
  currentMode = mode;
  remainingSeconds = durationForMode(currentMode);
}

void formatTime(char *buffer, size_t size, uint32_t seconds) {
  const uint32_t minutes = seconds / 60;
  const uint32_t secs = seconds % 60;
  snprintf(buffer, size, "%02lu:%02lu", static_cast<unsigned long>(minutes),
           static_cast<unsigned long>(secs));
}

void refreshUi() {
  char timeText[16];
  formatTime(timeText, sizeof(timeText), remainingSeconds);

  lv_label_set_text(modeLabel, nameForMode(currentMode));
  lv_label_set_text(timerLabel, timeText);
  lv_label_set_text(statusLabel, running ? promptForMode(currentMode) : "Tap start when ready");
  lv_label_set_text(startPauseLabel, running ? "Pause" : "Start");

  const uint32_t total = durationForMode(currentMode);
  const uint32_t elapsed = total - remainingSeconds;
  const int progress = total == 0 ? 0 : static_cast<int>((elapsed * 100) / total);
  lv_bar_set_value(progressBar, progress, LV_ANIM_ON);
}

void updateBatteryLabel() {
  const uint16_t batteryMv = amoled.SY.getBattVoltage();
  const uint16_t usbMv = amoled.SY.getVbusVoltage();
  const bool usbConnected = amoled.SY.isVbusIn();

  if (batteryMv > 0) {
    lv_label_set_text_fmt(batteryLabel, "%u mV  %s", batteryMv,
                          usbConnected ? "USB" : "BAT");
  } else if (usbMv > 0) {
    lv_label_set_text_fmt(batteryLabel, "USB %u mV", usbMv);
  } else {
    lv_label_set_text(batteryLabel, "Power: unknown");
  }
}

void completeInterval() {
  running = false;
  setMode(currentMode == TimerMode::Focus ? TimerMode::Break : TimerMode::Focus);
  refreshUi();
}

void onStartPause(lv_event_t *) {
  running = !running;
  lastTickMs = millis();
  refreshUi();
}

void onReset(lv_event_t *) {
  running = false;
  remainingSeconds = durationForMode(currentMode);
  refreshUi();
}

void onSkip(lv_event_t *) {
  running = false;
  setMode(currentMode == TimerMode::Focus ? TimerMode::Break : TimerMode::Focus);
  refreshUi();
}

lv_obj_t *makeButton(lv_obj_t *parent, const char *text, lv_event_cb_t callback) {
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_size(button, 130, 58);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_center(label);
  return button;
}

void buildUi() {
  screen = lv_scr_act();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x111418), 0);
  lv_obj_set_style_text_color(screen, lv_color_hex(0xF6F1E8), 0);

  modeLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(modeLabel, &lv_font_montserrat_28, 0);
  lv_obj_align(modeLabel, LV_ALIGN_TOP_MID, 0, 34);

  batteryLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(batteryLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(batteryLabel, lv_color_hex(0xA9B4C0), 0);
  lv_obj_align(batteryLabel, LV_ALIGN_TOP_RIGHT, -18, 12);

  timerLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(timerLabel, &lv_font_montserrat_48, 0);
  lv_obj_align(timerLabel, LV_ALIGN_CENTER, 0, -62);

  statusLabel = lv_label_create(screen);
  lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xA9B4C0), 0);
  lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, 0);

  progressBar = lv_bar_create(screen);
  lv_obj_set_size(progressBar, 330, 18);
  lv_obj_align(progressBar, LV_ALIGN_CENTER, 0, 42);
  lv_obj_set_style_bg_color(progressBar, lv_color_hex(0x2A313A), LV_PART_MAIN);
  lv_obj_set_style_bg_color(progressBar, lv_color_hex(0x4FD1C5), LV_PART_INDICATOR);
  lv_bar_set_range(progressBar, 0, 100);

  startPauseButton = makeButton(screen, "Start", onStartPause);
  lv_obj_align(startPauseButton, LV_ALIGN_BOTTOM_MID, 0, -92);
  lv_obj_set_style_bg_color(startPauseButton, lv_color_hex(0x4FD1C5), 0);
  startPauseLabel = lv_obj_get_child(startPauseButton, 0);

  resetButton = makeButton(screen, "Reset", onReset);
  lv_obj_align(resetButton, LV_ALIGN_BOTTOM_LEFT, 55, -24);
  lv_obj_set_style_bg_color(resetButton, lv_color_hex(0x3A4652), 0);

  skipButton = makeButton(screen, "Skip", onSkip);
  lv_obj_align(skipButton, LV_ALIGN_BOTTOM_RIGHT, -55, -24);
  lv_obj_set_style_bg_color(skipButton, lv_color_hex(0x3A4652), 0);

  updateBatteryLabel();
  refreshUi();
}

void tickTimer() {
  if (!running) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastTickMs < 1000) {
    return;
  }

  const uint32_t elapsed = (now - lastTickMs) / 1000;
  lastTickMs += elapsed * 1000;

  if (elapsed >= remainingSeconds) {
    remainingSeconds = 0;
    completeInterval();
    return;
  }

  remainingSeconds -= elapsed;
  refreshUi();
}

} // namespace

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("FocusFrame starting...");

  if (!amoled.beginAMOLED_241()) {
    Serial.println("Failed to initialize LILYGO T4-S3 AMOLED.");
    while (true) {
      delay(1000);
    }
  }

  beginLvglHelper(amoled);
  buildUi();
}

void loop() {
  lv_task_handler();
  tickTimer();

  const uint32_t now = millis();
  if (now - lastBatteryMs > 10000) {
    lastBatteryMs = now;
    updateBatteryLabel();
  }

  delay(5);
}
