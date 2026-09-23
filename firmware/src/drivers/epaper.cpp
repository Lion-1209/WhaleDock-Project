#include "epaper.h"

#include <SPI.h>

namespace epaper {

static DriverT epd2(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);

void init() {
  // SPI2/FSPI：SCK/MOSI 见 pins.h，MISO 无（墨水屏只写不读）；CS 由驱动库控制
  SPI.begin(EPD_CLK, -1, EPD_DIN, -1);
  // 预配置控制脚再交库初始化：GxEPD2 先 digitalWrite 后 pinMode 的顺序在
  // Arduino core 3.x 会报 GPIO 错误日志，且 CS/RST 短暂输出 0 产生毛刺复位
  pinMode(EPD_CS, OUTPUT);
  digitalWrite(EPD_CS, HIGH);
  pinMode(EPD_DC, OUTPUT);
  pinMode(EPD_RST, OUTPUT);
  pinMode(EPD_BUSY, INPUT);
  // 硬复位脉冲（必须！）：DSLP 深睡只能由 RST 低脉冲唤醒——GxEPD2 在
  // initial=false 时跳过自身复位分支。每次 init/demo 重触发前都先唤醒。
  digitalWrite(EPD_RST, HIGH);
  delay(10);
  digitalWrite(EPD_RST, LOW);
  const int rbLo = digitalRead(EPD_RST);  // 输出回读：验证 GPIO14 真的在翻转
  delay(100);                             // 手册口径 ≥200µs，取保守 100ms
  digitalWrite(EPD_RST, HIGH);
  const int rbHi = digitalRead(EPD_RST);
  const int busy0 = digitalRead(EPD_BUSY);
  delay(100);
  const int busy1 = digitalRead(EPD_BUSY);
  Serial.printf("[屏] RST 回读 LO=%d HI=%d；BUSY 复位后 %d→%d\n", rbLo, rbHi, busy0, busy1);
  epd2.init(115200, false, 2, false);
  // initial=false：跳过上电清屏（省一轮 21s 全刷），开机即由 canvas::flush 整帧上屏
}

DriverT& driver() {
  return epd2;
}

void powerOff() {
  // 只断面板驱动电压（POF），保持控制器唤醒态（RAM 保留，sleep 电流 20-35µA）。
  // 不用 hibernate/DSLP：实测深睡后 RST 脉冲唤不醒（v0.8.0 loop 语境全灭根因），
  // 同会话二次刷屏必须保持唤醒；市电桌搭对 20µA 级无感
  epd2.powerOff();
}

void hibernate() {
  epd2.hibernate();
}

}  // namespace epaper
