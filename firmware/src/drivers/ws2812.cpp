#include "ws2812.h"

#include <Arduino.h>  // rgbLedWrite（esp32-hal-rgb-led.h，随 Arduino.h 引入）

Ws2812::Ws2812(int gpio) : gpio_(gpio) {}

void Ws2812::write(uint8_t r, uint8_t g, uint8_t b) {
  rgbLedWrite(gpio_, r, g, b);  // HAL 首次调用时自建 RMT 通道，此后复用
}

void Ws2812::off() {
  write(0, 0, 0);
}
