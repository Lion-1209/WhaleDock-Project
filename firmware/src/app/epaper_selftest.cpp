#include "epaper_selftest.h"

#include <Arduino.h>

#include "app/canvas.h"
#include "config.h"
#include "drivers/epaper.h"

namespace epaper_selftest {

// B2 管线验证图（与 B1 测试图同版式，画布换 canvas 三平面管线）。
// 验收点与判读：
//   ① 四角 L 形角标 + 全幅黑框 —— 整幅寻址（800×480 定稿）
//   ② 顶部四色条（黑/白/红/黄）—— 四色各平面都能驱动
//   ③ 中部棋盘格 —— 黑白平面逐行逐列寻址，可见断线/错位
//   ④ 红/黄验证带带字 —— 彩色平面与文字混排（GFX 文本走 drawPixel→三平面）
//   ⑤ 串口打印打包/刷新分段耗时 —— PSRAM 读取 + 2bpp 打包性能留档
static void drawTestPattern() {
  auto& d = canvas::get();  // Adafruit_GFX 语义
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

  // 底部信息行：画布分辨率 + 固件版本 + 管线标识
  d.setCursor(16, h - 34);
  d.printf("%dx%d canvas | FW v%s | B2 pipeline", w, h, FW_VERSION);
}

void run() {
  epaper::init();
  if (!canvas::get().begin()) {
    Serial.println("[屏] 三平面 PSRAM 分配失败，跳过 B2 验证图");
    return;
  }
  drawTestPattern();
  canvas::flush();
}

}  // namespace epaper_selftest
