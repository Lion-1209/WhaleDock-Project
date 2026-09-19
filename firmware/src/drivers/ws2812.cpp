#include "ws2812.h"

Ws2812::Ws2812(int gpio, rgb_led_color_order_t colorOrder)
    : gpio_(gpio), order_(colorOrder) {}

void Ws2812::write(uint8_t r, uint8_t g, uint8_t b) {
  // 按构造时声明的通道顺序编码发送；HAL 首次调用时自建 RMT 通道，此后复用
  rgbLedWriteOrdered(gpio_, order_, r, g, b);
}

void Ws2812::off() {
  write(0, 0, 0);
}
