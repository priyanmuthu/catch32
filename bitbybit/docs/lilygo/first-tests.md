# T-Encoder-Pro First Tests

Run these before attempting a full display or LVGL application.

## 1. Serial Test

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.println("T-Encoder-Pro serial OK");
  delay(1000);
}
```

Expected result: serial monitor prints once per second at `115200`.

## 2. Buzzer Test

```cpp
#include <Arduino.h>

#define BUZZER_DATA 17

void setup() {
  ledcAttach(BUZZER_DATA, 2000, 8);
}

void loop() {
  ledcWriteTone(BUZZER_DATA, 1000);
  delay(150);
  ledcWriteTone(BUZZER_DATA, 0);
  delay(850);
}
```

Expected result: short beep once per second.

If this fails to compile, your Arduino-ESP32 version may use the older LEDC API. In that case, use the version-specific LEDC examples bundled with your installed ESP32 board package.

## 3. I2C Scan

```cpp
#include <Arduino.h>
#include <Wire.h>

#define IIC_SDA 5
#define IIC_SCL 6

void setup() {
  Serial.begin(115200);
  Wire.begin(IIC_SDA, IIC_SCL);
  Serial.println("I2C scan");

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found I2C device at 0x%02X\n", addr);
    }
  }
}

void loop() {}
```

Expected result: at least one I2C address should appear for the touch controller.

## 4. Encoder Test

Install the Arduino library `Ai Esp32 Rotary Encoder`, then flash:

```cpp
#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>

#define KNOB_DATA_A 1
#define KNOB_DATA_B 2
#define KNOB_KEY 0
#define ROTARY_STEPS 4

AiEsp32RotaryEncoder encoder(KNOB_DATA_A, KNOB_DATA_B, KNOB_KEY, -1, ROTARY_STEPS);

void IRAM_ATTR readEncoderISR() {
  encoder.readEncoder_ISR();
}

void setup() {
  Serial.begin(115200);
  encoder.begin();
  encoder.setup(readEncoderISR);
  encoder.setBoundaries(-100, 100, false);
  encoder.setAcceleration(0);
}

void loop() {
  if (encoder.encoderChanged()) {
    Serial.println(encoder.readEncoder());
  }

  if (encoder.isEncoderButtonClicked()) {
    Serial.println("button");
  }

  delay(10);
}
```

Expected result: turning the dial changes the count; pressing it prints `button`.

## 5. Display Power Enable Probe

Before trying a display driver, confirm that the display enable GPIO can be driven:

```cpp
#include <Arduino.h>

#define LCD_VCI_EN 3

void setup() {
  pinMode(LCD_VCI_EN, OUTPUT);
  digitalWrite(LCD_VCI_EN, HIGH);
  Serial.begin(115200);
  Serial.println("Display VCI enabled");
}

void loop() {}
```

Expected result: this does not draw anything by itself, but it is a required first step for display bring-up in the referenced Pro firmware.
