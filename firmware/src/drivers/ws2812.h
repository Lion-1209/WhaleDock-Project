#pragma once
// ============================================================
// 驱动层 · WS2812 LED 驱动
// 对齐开发约定§4：WS2812 走 RMT 外设，禁止 bit-bang 关中断。
// 实现基于 Arduino-ESP32 3.x 的 rgbLedWriteOrdered()——HAL 内部即 IDF
// led_strip + RMT，每个引脚 1 像素、1 个 RMT 通道，零额外依赖。
//
// 颜色通道顺序：标准 WS2812B 为 GRB（默认）；个别灯珠/批次为 RGB 等其它
// 序——发"绿"显"红"即此问题，构造时显式传入实测顺序。
//
// 用途：水线灯 ×2（产品，两个引脚各挂 1 灯，引脚待 pins.h 启用）；
//       当前兼任开发板板载灯调机（见 app/onboard_led）。
// 若量产改为单线级联多像素，届时引入 NeoPixelBus 重写本类，调用点不变。
// ============================================================

#include <Arduino.h>  // rgb_led_color_order_t / rgbLedWriteOrdered

class Ws2812 {
 public:
  explicit Ws2812(int gpio,
                  rgb_led_color_order_t colorOrder = LED_COLOR_ORDER_GRB);

  void write(uint8_t r, uint8_t g, uint8_t b);  // 写色并立即经 RMT 刷新
  void off();                                   // 熄灭

  int gpio() const { return gpio_; }

 private:
  int gpio_;
  rgb_led_color_order_t order_;
};
