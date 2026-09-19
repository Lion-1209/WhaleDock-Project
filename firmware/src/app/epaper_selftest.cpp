#include "epaper_selftest.h"

#include <Arduino.h>

#include "drivers/epaper.h"

namespace epaper_selftest {

static void drawTestPattern() {
  auto& d = epaper::display();
  constexpr int cell = 40;  // 800/40 = 20 列，480/40 = 12 行
  const int w = d.width();
  const int h = d.height();

  d.fillScreen(GxEPD_WHITE);

  for (int y = 0; y < h; y += cell)
    for (int x = 0; x < w; x += cell)
      if (((x / cell) + (y / cell)) % 2 == 0)
        d.fillRect(x, y, cell, cell, GxEPD_BLACK);

  d.fillRect(0, 0, 260, 36, GxEPD_RED);            // 左上红标
  d.fillRect(w - 260, h - 36, 260, 36, GxEPD_RED); // 右下红标
  d.drawRect(2, 2, w - 4, h - 4, GxEPD_BLACK);

  d.setTextColor(GxEPD_WHITE);  // 红标上写白字
  d.setTextSize(3);
  d.setCursor(16, 12);
  d.print("WhaleDock M1");

  d.setTextColor(GxEPD_BLACK);
  d.setTextSize(2);
  d.setCursor(16, h - 28);
  d.print("800x480 BWR checkerboard");

  Serial.println("[屏] 双色测试图渲染完成，开始全刷（约 15-25s，勿断电）...");
  const uint32_t t0 = millis();
  d.display();  // 全刷：写两平面到显存并触发整帧刷新，内部等待 BUSY
  Serial.printf("[屏] 全刷完成，耗时 %.1fs\n", (millis() - t0) / 1000.0);
}

void run() {
  epaper::init();
  drawTestPattern();
  epaper::hibernate();
}

}  // namespace epaper_selftest
