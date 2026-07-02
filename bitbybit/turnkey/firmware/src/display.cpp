#include "display.h"

#include <Arduino.h>
#include "board_config.h"

namespace {
bool hostConnected = false;
lv_obj_t* root = nullptr;
}

void displayBegin() {
  pinMode(board::LCD_VCI_EN, OUTPUT);
  digitalWrite(board::LCD_VCI_EN, HIGH);

  lv_init();

  // TODO: Attach the SH8601 QSPI display driver and CHSC5816 touch driver here.
  // The app UI is deliberately isolated from panel setup so board bring-up can
  // start from the known-good LilyGO/community driver example.
  root = lv_obj_create(nullptr);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
  lv_scr_load(root);
}

void displayTick() {
  lv_timer_handler();
}

lv_obj_t* displayRoot() {
  return root ? root : lv_scr_act();
}

void displaySetHostConnected(bool connected) {
  hostConnected = connected;
}

bool displayHostConnected() {
  return hostConnected;
}

