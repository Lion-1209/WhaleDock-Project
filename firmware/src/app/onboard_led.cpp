#include "onboard_led.h"

#include <Arduino.h>

#include "config.h"

void OnboardLed::begin() {
  write(0, 0, 2);  // 上电微光（亮度见 config.h 注释），确认灯链路（RMT 通道由 HAL 首写时创建）
  Serial.println("[灯] 板载 WS2812：GPIO38（本机批次实测），rgbLedWrite/RMT");
}

void OnboardLed::update() {
  const float phase = (millis() % BREATH_PERIOD_MS) / (float)BREATH_PERIOD_MS;
  // v ∈ [BREATH_MIN, BREATH_MAX]，是实际的 PWM 驱动值（0-255）。
  // 颜色按 v/255 等比调制——此前误除 BREATH_MAX 会把峰值拉回 255（全亮）
  const float v = BREATH_MIN +
                  (BREATH_MAX - BREATH_MIN) * 0.5f *
                      (1.0f - cosf(2.0f * PI * phase));
  write((uint8_t)(cr_ * v / 255.0f), (uint8_t)(cg_ * v / 255.0f),
        (uint8_t)(cb_ * v / 255.0f));
}

void OnboardLed::setBreathColor(uint8_t r, uint8_t g, uint8_t b) {
  cr_ = r;
  cg_ = g;
  cb_ = b;
}

void OnboardLed::write(uint8_t r, uint8_t g, uint8_t b) {
  led_.write(r, g, b);
}
