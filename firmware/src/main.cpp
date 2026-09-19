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
#include "services/cli.h"
#include "services/ntp.h"
#include "services/storage.h"
#include "services/wifi.h"

static OnboardLed onboardLed;

void setup() {
  // 原生 USB CDC（ARDUINO_USB_CDC_ON_BOOT=1），上电即可被 Web Serial 枚举
  Serial.begin(SERIAL_BAUD);
  delay(2000);  // 等 CDC 枚举完成，避免开头日志丢失

  selfcheck::print();
  onboardLed.begin();
  storage::begin();  // 配置与数据缓存的持久化（LittleFS，分区 lfs）
  wifi::begin();
  cli::begin();

  if (!SCREEN_ATTACHED) {
    Serial.println("[屏] 未接屏（SCREEN_ATTACHED=false），跳过点屏，进入呼吸心跳");
    return;
  }
  epaper_selftest::run();
}

void loop() {
  cli::poll();
  wifi::poll();
  ntp::poll();  // 对时 + 整点调度（回调挂载点留给 A4 数据流水）

  // 呼吸灯兼任联网状态指示：绿 = 已连接，蓝 = 未连接/重连中
  const bool online = wifi::state() == wifi::State::Connected;
  onboardLed.setBreathColor(online ? 0 : 0, online ? 255 : 0, online ? 0 : 255);
  onboardLed.update();

  selfcheck::heartbeat();  // 串口心跳（内部自行节流）
  delay(20);               // 呼吸周期下 20ms 步进已足够平滑
}
