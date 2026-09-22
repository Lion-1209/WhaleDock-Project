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
  // initial=false 时跳过自身复位分支，若屏因上次 flush 深睡而未断电，
  // 会忽略全部命令（实测全刷 0.5s 假完成）。每次 init/demo 重触发前都先唤醒。
  digitalWrite(EPD_RST, HIGH);
  delay(10);
  digitalWrite(EPD_RST, LOW);
  delay(50);   // 厂商例程口径（手册最小 200µs，取保守值）
  digitalWrite(EPD_RST, HIGH);
  delay(50);   // 等控制器复位完成、OTP 加载
  // initial=false：跳过上电清屏（省一轮 21s 全刷），开机即由 canvas::flush 整帧上屏
  epd2.init(115200, false, 2, false);
}

DriverT& driver() {
  return epd2;
}

void hibernate() {
  epd2.hibernate();
}

}  // namespace epaper
