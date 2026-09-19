// ============================================================
// 鲸屿 WhaleDock · M1 自检程序（点屏里程碑）
// 对应开发约定§10 M1 清单：
//   [x] Flash 16MB / PSRAM 8MB 自检（配置与硬件双重确认）
//   [x] 双色测试图上屏：黑白棋盘格 + 红色角标 + 文字
//
// 屏幕驱动类按 FPC 排线丝印型号选择（换屏时只改 using 那行）：
//   丝印 GDEW075Z08（800×480 三色，微雪 7.5" B 型）→ GxEPD2_750c_GDEW075Z08
//   丝印 GDEY075Z08（新版屏）                       → GxEPD2_750c_GDEY075Z08
//
// 三色屏全刷约 15–25s，BUSY 等待期间属正常现象，勿断电
// ============================================================

#include <Arduino.h>
#include <SPI.h>

#include <GxEPD2_3C.h>  // 该头文件已包含全部三色驱动类，无需再 include 具体驱动

#include "pins.h"

// 三色组合显示类（黑白 + 红双平面）：GxEPD2 1.6 起 API 为"驱动实例注入"式
// page_height 取 HEIGHT = 整屏缓冲（双平面 96KB），不做分页
using DriverT = GxEPD2_750c_GDEW075Z08;
using DisplayT = GxEPD2_3C<DriverT, DriverT::HEIGHT>;
static DriverT epd2(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);
static DisplayT display(epd2);

static void printSelfCheck() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("        鲸屿 WhaleDock · M1 自检");
  Serial.println("========================================");
  Serial.printf("芯片    : %s rev%d / %d 核 @ %u MHz\n",
                ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores(),
                ESP.getCpuFreqMHz());

  const size_t flashMB = ESP.getFlashChipSize() / (1024UL * 1024);
  const size_t psramMB = ESP.getPsramSize() / (1024UL * 1024);
  Serial.printf("Flash   : %u MB @ %u MHz  %s\n", flashMB,
                ESP.getFlashChipSpeed() / 1000000,
                flashMB >= 16 ? "[OK]" : "[!] 应为 16MB");
  Serial.printf("PSRAM   : %u MB  %s\n", psramMB,
                psramMB >= 8 ? "[OK]" : "[!] 应为 8MB，检查 memory_type=qio_opi");
  Serial.printf("堆内存  : 空闲 %u KB / 历史最低 %u KB\n",
                ESP.getFreeHeap() / 1024, ESP.getMinFreeHeap() / 1024);
  Serial.println("========================================");
}

// 黑白棋盘格验证黑白平面寻址，红色角标验证红平面寻址
static void drawTestPattern() {
  constexpr int cell = 40;  // 800/40 = 20 列，480/40 = 12 行
  const int w = display.width();
  const int h = display.height();

  display.fillScreen(GxEPD_WHITE);

  for (int y = 0; y < h; y += cell)
    for (int x = 0; x < w; x += cell)
      if (((x / cell) + (y / cell)) % 2 == 0)
        display.fillRect(x, y, cell, cell, GxEPD_BLACK);

  display.fillRect(0, 0, 260, 36, GxEPD_RED);            // 左上红标
  display.fillRect(w - 260, h - 36, 260, 36, GxEPD_RED); // 右下红标
  display.drawRect(2, 2, w - 4, h - 4, GxEPD_BLACK);

  display.setTextColor(GxEPD_WHITE);  // 红标上写白字
  display.setTextSize(3);
  display.setCursor(16, 12);
  display.print("WhaleDock M1");

  display.setTextColor(GxEPD_BLACK);
  display.setTextSize(2);
  display.setCursor(16, h - 28);
  display.print("800x480 BWR checkerboard");

  Serial.println("[屏] 双色测试图渲染完成，开始全刷（约 15-25s，勿断电）...");
  const uint32_t t0 = millis();
  display.display();  // 全刷：写两平面到显存并触发整帧刷新，内部等待 BUSY
  Serial.printf("[屏] 全刷完成，耗时 %.1fs\n", (millis() - t0) / 1000.0);
  display.hibernate();  // 深度下电保护屏体，静态画面保留
}

void setup() {
  // 原生 USB CDC（ARDUINO_USB_CDC_ON_BOOT=1），上电即可被 Web Serial 枚举
  Serial.begin(115200);
  delay(2000);  // 等 CDC 枚举完成，避免开头日志丢失

  printSelfCheck();

  // SPI2/FSPI：SCK/MOSI 如上，MISO 无（墨水屏只写不读）；CS 由驱动库控制
  SPI.begin(EPD_CLK, -1, EPD_DIN, -1);

  // init(串口诊断波特率, 上电先清屏, 复位脉冲 ms, RST 下拉模式)
  display.init(115200, true, 2, false);
  drawTestPattern();
}

// 心跳：确认固件存活、观察内存水位
void loop() {
  static uint32_t beat = 0;
  delay(10000);
  Serial.printf("[心跳] %us  堆 %u KB / PSRAM 空闲 %u KB\n", ++beat * 10,
                ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);
}
