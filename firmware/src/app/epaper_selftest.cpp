#include "epaper_selftest.h"

#include <Arduino.h>

#include "config.h"
#include "drivers/epaper.h"

namespace epaper_selftest {

// B1 实机验收测试图（尺寸自适应驱动口径，800×480 / 880×528 均可渲染）。
// 验收点与判读：
//   ① 四角 L 形角标 + 全幅黑框 —— 整幅寻址；右/下白边 = 物理分辨率大于驱动口径
//   ② 顶部四色条（黑/白/红/黄）—— 四色各平面都能驱动
//   ③ 中部棋盘格 —— 黑白平面逐行逐列寻址，可见断线/错位
//   ④ 红/黄验证带带字 —— 彩色平面与文字混排
static void drawTestPattern() {
  auto& d = epaper::display();
  const int w = d.width();
  const int h = d.height();

  d.fillScreen(GxEPD_WHITE);

  // ① 全幅黑框 + 四角 L 形角标（臂长 28、线宽 6）
  constexpr int L = 28;
  constexpr int T = 6;
  d.drawRect(0, 0, w, h, GxEPD_BLACK);
  d.fillRect(0, 0, L, T, GxEPD_BLACK);
  d.fillRect(0, 0, T, L, GxEPD_BLACK);
  d.fillRect(w - L, 0, L, T, GxEPD_BLACK);
  d.fillRect(w - T, 0, T, L, GxEPD_BLACK);
  d.fillRect(0, h - T, L, T, GxEPD_BLACK);
  d.fillRect(0, h - L, T, L, GxEPD_BLACK);
  d.fillRect(w - L, h - T, L, T, GxEPD_BLACK);
  d.fillRect(w - T, h - L, T, L, GxEPD_BLACK);

  // ② 顶部四色条：白段描黑框使其可见
  constexpr int barH = 40;
  const int seg = w / 4;
  for (int i = 0; i < 4; i++) {
    const int x = seg * i + 8;
    const uint16_t c = (i == 0) ? GxEPD_BLACK : (i == 1) ? GxEPD_WHITE : (i == 2) ? GxEPD_RED : GxEPD_YELLOW;
    d.fillRect(x, 16, seg - 16, barH, c);
    if (i == 1) d.drawRect(x, 16, seg - 16, barH, GxEPD_BLACK);
  }

  // ③ 中部黑白棋盘格
  constexpr int cell = 40;
  const int bandY = h - 92;  // 底部彩色验证带顶缘
  for (int y = 128; y + cell <= bandY - 12; y += cell)
    for (int x = 16; x + cell <= w - 16; x += cell)
      if (((x / cell) + (y / cell)) % 2 == 0)
        d.fillRect(x, y, cell, cell, GxEPD_BLACK);

  // ④ 底部彩色验证带：左红（白字）右黄（黑字）
  d.fillRect(16, bandY, 300, 44, GxEPD_RED);
  d.setTextColor(GxEPD_WHITE);
  d.setTextSize(2);
  d.setCursor(28, bandY + 15);
  d.print("RED");

  d.fillRect(w - 316, bandY, 300, 44, GxEPD_YELLOW);
  d.setTextColor(GxEPD_BLACK);
  d.setCursor(w - 304, bandY + 15);
  d.print("YELLOW");

  // 底部信息行：驱动口径分辨率 + 固件版本（分辨率与实物不符时右/下会留白边）
  d.setCursor(16, h - 34);
  d.printf("%dx%d driver | FW v%s | B1 checkup", w, h, FW_VERSION);

  Serial.printf("[屏] B1 四色测试图渲染完成（驱动口径 %dx%d），开始全刷（约 21s，勿断电）...\n", w, h);
  const uint32_t t0 = millis();
  d.display();  // 全刷：2bpp 单命令 0x10 推全帧并触发整帧刷新，内部等待 BUSY
  Serial.printf("[屏] 全刷完成，耗时 %.1fs\n", (millis() - t0) / 1000.0);
  Serial.println("[屏] 判读：四角角标齐全=寻址完整；右/下白边=物理分辨率大于驱动（切 DFG0750RYS 驱动复测）");
}

void run() {
  epaper::init();
  drawTestPattern();
  epaper::hibernate();
}

}  // namespace epaper_selftest
