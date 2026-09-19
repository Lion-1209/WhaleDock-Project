#include "onboard_led.h"

#include <Arduino.h>

#include "config.h"

void OnboardLed::begin() {
  write(0, 0, 8);  // 上电微光，确认灯链路（RMT 通道由 HAL 首写时创建）
  Serial.println("[灯] 板载 WS2812：GPIO38（本机批次实测），rgbLedWrite/RMT");
}

void OnboardLed::update() {
  const float phase = (millis() % BREATH_PERIOD_MS) / (float)BREATH_PERIOD_MS;
  const uint8_t v = (uint8_t)(BREATH_MIN +
                              (BREATH_MAX - BREATH_MIN) * 0.5f *
                                  (1.0f - cosf(2.0f * PI * phase)));
  write(0, 0, v);
}

void OnboardLed::write(uint8_t r, uint8_t g, uint8_t b) {
  led_.write(r, g, b);
}
