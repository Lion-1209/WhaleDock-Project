#include "epaper.h"

#include <SPI.h>

namespace epaper {

// GxEPD2 1.6 起 API 为"驱动实例注入"式
static DriverT epd2(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);
static DisplayT disp(epd2);

void init() {
  // SPI2/FSPI：SCK/MOSI 见 pins.h，MISO 无（墨水屏只写不读）；CS 由驱动库控制
  SPI.begin(EPD_CLK, -1, EPD_DIN, -1);
  // 预配置控制脚再交库初始化：GxEPD2 先 digitalWrite 后 pinMode 的顺序在
  // Arduino core 3.x 会报 GPIO 错误日志，且 CS/RST 短暂输出 0 产生毛刺复位
  pinMode(EPD_CS, OUTPUT);
  digitalWrite(EPD_CS, HIGH);
  pinMode(EPD_DC, OUTPUT);
  pinMode(EPD_RST, OUTPUT);
  digitalWrite(EPD_RST, HIGH);
  pinMode(EPD_BUSY, INPUT);
  // init(串口诊断波特率, 上电先清屏, 复位脉冲 ms, RST 下拉模式)
  disp.init(115200, true, 2, false);
}

DisplayT& display() {
  return disp;
}

void hibernate() {
  disp.hibernate();
}

}  // namespace epaper
