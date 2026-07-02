#pragma once

#include <Arduino.h>

namespace board {

constexpr uint8_t ENCODER_A = 1;
constexpr uint8_t ENCODER_B = 2;
constexpr uint8_t ENCODER_KEY = 0;
constexpr uint8_t BUZZER = 17;

constexpr uint8_t I2C_SDA = 5;
constexpr uint8_t I2C_SCL = 6;

constexpr uint8_t TOUCH_INT = 9;
constexpr uint8_t TOUCH_RST = 8;

constexpr uint8_t LCD_VCI_EN = 3;
constexpr uint8_t TFT_RST = 4;
constexpr uint8_t TFT_QSPI_CS = 10;
constexpr uint8_t TFT_QSPI_SCK = 12;
constexpr uint8_t TFT_QSPI_D0 = 11;
constexpr uint8_t TFT_QSPI_D1 = 13;
constexpr uint8_t TFT_QSPI_D2 = 7;
constexpr uint8_t TFT_QSPI_D3 = 14;

constexpr uint16_t SCREEN_WIDTH = 390;
constexpr uint16_t SCREEN_HEIGHT = 390;

}

