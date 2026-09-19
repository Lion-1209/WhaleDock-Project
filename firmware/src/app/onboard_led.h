#pragma once
// ============================================================
// 应用层 · 板载 RGB 灯（调机资源，非产品引脚）
// DevKitC-1 板载 WS2812 随批次接 GPIO48 或 GPIO38，本机实测为 38。
// 职责：上电微光确认灯链路 + 运行期蓝色呼吸心跳。
// 水线灯产品化（pins.h 启用产品引脚）后，本模块退役或改接水线灯。
// ============================================================

#include <stdint.h>

#include "drivers/ws2812.h"

class OnboardLed {
 public:
  void begin();   // 点微光 + 串口打印链路信息
  void update();  // 每个 loop 调用：呼吸（周期/亮度见 config.h）
  void setBreathColor(uint8_t r, uint8_t g, uint8_t b);  // 默认蓝色
  void write(uint8_t r, uint8_t g, uint8_t b);

 private:
  // 本机批次实测 GPIO38、RGB 通道序（发绿显红暴露的问题）；部分批次为 GPIO48
  Ws2812 led_{38, LED_COLOR_ORDER_RGB};
  uint8_t cr_ = 0, cg_ = 0, cb_ = 255;  // 呼吸颜色，按 BREATH_MAX 归一化调制
};
