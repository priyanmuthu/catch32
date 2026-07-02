#pragma once

#include <lvgl.h>

void displayBegin();
void displayTick();
lv_obj_t* displayRoot();
void displaySetHostConnected(bool connected);
bool displayHostConnected();

