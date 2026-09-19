// ============================================================
// 鲸屿 WhaleDock · 主程序（装配与编排）
// 分层（对齐设计文档总体架构）：
//   src/drivers/   驱动层：外设封装（epaper / ws2812 / …）
//   src/services/  服务层：M2 起的数据/配网/OTA 模块（见其 README）
//   src/app/       应用层：自检、调机、里程碑验证流程
// 编译期配置集中在 include/config.h，产品引脚在 include/pins.h。
// ============================================================

#include <Arduino.h>

#include "config.h"
#include "app/epaper_selftest.h"
#include "app/onboard_led.h"
#include "app/selfcheck.h"

static OnboardLed onboardLed;

void setup() {
  // 原生 USB CDC（ARDUINO_USB_CDC_ON_BOOT=1），上电即可被 Web Serial 枚举
  Serial.begin(SERIAL_BAUD);
  delay(2000);  // 等 CDC 枚举完成，避免开头日志丢失

  selfcheck::print();
  onboardLed.begin();

  if (!SCREEN_ATTACHED) {
    Serial.println("[屏] 未接屏（SCREEN_ATTACHED=false），跳过点屏，进入呼吸心跳");
    return;
  }
  epaper_selftest::run();
}

void loop() {
  onboardLed.update();     // 蓝色呼吸
  selfcheck::heartbeat();  // 串口心跳（内部自行节流）
  delay(20);               // 呼吸周期下 20ms 步进已足够平滑
}
