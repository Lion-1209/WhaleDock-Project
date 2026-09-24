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
#include "app/assets/concept_demo.h"
#include "app/epaper_selftest.h"
#include "app/onboard_led.h"
#include "app/selfcheck.h"
#include "services/cli.h"
#include "services/datapipe.h"
#include "services/ntp.h"
#include "services/ota.h"
#include "services/storage.h"
#include "services/webapi.h"
#include "services/worker.h"
#include "services/wifi.h"

static OnboardLed onboardLed;

void setup() {
  // 原生 USB CDC（ARDUINO_USB_CDC_ON_BOOT=1），上电即可被 Web Serial 枚举
  Serial.begin(SERIAL_BAUD);
  delay(2000);  // 等 CDC 枚举完成，避免开头日志丢失
  Serial.printf("[固件] WhaleDock v%s\n", FW_VERSION);

  selfcheck::print();
  onboardLed.begin();
  storage::begin();      // 配置与数据缓存的持久化（LittleFS，分区 lfs）
  wifi::begin();
  cli::begin();
  ota::begin();          // OTA 分区状态检查 + 待验证固件自动确认
  datapipe::begin();     // 挂接整点回调：到点按设备配置自动拉取数据源
  webapi::begin();       // 设备 HTTP API（局域网通道，mDNS 联网后注册）
  worker::begin();       // 重活工作队列（渲染/流水/OTA 出 loopTask）

  if (!SCREEN_ATTACHED) {
    Serial.println("[屏] 未接屏（SCREEN_ATTACHED=false），跳过点屏，进入呼吸心跳");
    return;
  }
  // 开机画面入队（worker 任务渲染，CLI/HTTP 上电即在线，不再被 22s 全刷阻塞）；
  // 渲染 /layout.json（C2 引擎）；无文件时 worker 内部回退示例帧
  worker::requestRender(worker::Render::Layout);
}

void loop() {
  cli::poll();
  wifi::poll();
  ntp::poll();  // 对时 + 整点调度（回调挂载点留给 A4 数据流水）
  ota::poll();  // 待验证固件健康运行超时自动确认
  webapi::poll();  // 处理局域网 HTTP 请求 + mDNS 注册

  // 呼吸灯兼任联网状态指示：绿 = 已连接，蓝 = 未连接/重连中
  const bool online = wifi::state() == wifi::State::Connected;
  onboardLed.setBreathColor(online ? 0 : 0, online ? 255 : 0, online ? 0 : 255);
  onboardLed.update();

  selfcheck::heartbeat();  // 串口心跳（内部自行节流）
  delay(20);               // 呼吸周期下 20ms 步进已足够平滑
}
