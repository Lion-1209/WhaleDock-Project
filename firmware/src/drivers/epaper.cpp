#include "epaper.h"

#include <SPI.h>

namespace epaper {

// GxEPD2 1.6 起 API 为"驱动实例注入"式
static DriverT epd2(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);
static DisplayT disp(epd2);

void init() {
  // SPI2/FSPI：SCK/MOSI 见 pins.h，MISO 无（墨水屏只写不读）；CS 由驱动库控制
  SPI.begin(EPD_CLK, -1, EPD_DIN, -1);
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
