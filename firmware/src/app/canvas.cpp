#include "canvas.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_wifi.h>

#include "drivers/epaper.h"
#include "../services/wifi.h"

namespace canvas {

Canvas& get() {
  static Canvas c;
  return c;
}

bool Canvas::begin() {
  if (ready()) return true;
  for (int p = 0; p < 3; p++) {
    planes_[p] = (uint8_t*)heap_caps_malloc(PLANE_BYTES, MALLOC_CAP_SPIRAM);
    if (!planes_[p]) {  // 全有或全无：任一失败即回收，避免半初始化状态
      Serial.printf("[屏] PSRAM 平面 %d 分配失败（需 %dB/平面）\n", p, PLANE_BYTES);
      for (int q = 0; q < 3; q++) {
        heap_caps_free(planes_[q]);
        planes_[q] = nullptr;
      }
      return false;
    }
  }
  return true;
}

// 平面位写（行主序，MSB 前，与协议/模拟器一致）
static inline void bit_(uint8_t* plane, int16_t x, int16_t y, bool on) {
  uint8_t& b = plane[int32_t(y) * (canvas::W / 8) + (x >> 3)];
  const uint8_t mask = 0x80 >> (x & 7);
  if (on) b |= mask; else b &= ~mask;
}

void Canvas::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height) || !ready()) return;
  // 颜色映射与协议三平面一一对应；其余颜色按白处理（三平面全清）
  bit_(planes_[PL_BW], x, y, color == GxEPD_BLACK);
  bit_(planes_[PL_RED], x, y, color == GxEPD_RED);
  bit_(planes_[PL_YELLOW], x, y, color == GxEPD_YELLOW);
}

void Canvas::fillScreen(uint16_t color) {
  if (!ready()) return;
  memset(planes_[PL_BW], color == GxEPD_BLACK ? 0xFF : 0x00, PLANE_BYTES);
  memset(planes_[PL_RED], color == GxEPD_RED ? 0xFF : 0x00, PLANE_BYTES);
  memset(planes_[PL_YELLOW], color == GxEPD_YELLOW ? 0xFF : 0x00, PLANE_BYTES);
}

bool flush() {
  Canvas& c = get();
  if (!c.ready()) {
    Serial.println("[屏] canvas 未就绪，跳过推送");
    return false;
  }

  // 分页中转缓冲（内部 RAM）：16 行 × 200B（800px ÷ 4px/B）= 3.2KB
  constexpr int16_t PAGE_ROWS = 16;
  constexpr int16_t ROW_BYTES = W / 4;
  static uint8_t stage[PAGE_ROWS * ROW_BYTES];

  const uint8_t* bw = c.plane(PL_BW);
  const uint8_t* rd = c.plane(PL_RED);
  const uint8_t* yl = c.plane(PL_YELLOW);
  auto& epd2 = epaper::driver();

  // 射频静默（实测定案）：WiFi 已关联时 modem-sleep 周期唤醒的发射尖峰经
  // 杜邦供电线把面板侧 VCI 拉垮，PON/DRF 中途夭折（BUSY 提前释放、画面不变；
  // boot 期射频静默故总能刷成）。刷新期间设备本就阻塞无响应，停射频换波形
  // 完整；结束后重启射频，wifi 状态机自愈重连
  wifi_mode_t wifiMode = WIFI_MODE_NULL;
  esp_wifi_get_mode(&wifiMode);
  const bool wifiWasOn = wifiMode != WIFI_MODE_NULL;
  if (wifiWasOn) esp_wifi_stop();

  const uint32_t t0 = millis();
  epd2.setPaged();  // 内部完成 _InitDisplay 并发出 0x10 数据起始命令
  for (int16_t y = 0; y < H; y += PAGE_ROWS) {
    uint8_t* o = stage;
    for (int16_t r = 0; r < PAGE_ROWS; r++) {
      const int32_t row = int32_t(y + r) * (W / 8);
      for (int16_t xb = 0; xb < W / 8; xb++) {
        const uint8_t b = bw[row + xb], rn = rd[row + xb], ye = yl[row + xb];
        // 每 8 像素 → 2 个 2bpp 字节；优先级 红>黄>黑（与 GxEPD2_4C 映射一致）
        for (int16_t half = 0; half < 2; half++) {
          uint8_t out = 0;
          for (int16_t k = 0; k < 4; k++) {
            const uint8_t mask = 0x80 >> (half * 4 + k);
            out <<= 2;
            if (rn & mask) out |= 0x03;       // 11 红
            else if (ye & mask) out |= 0x02;  // 10 黄
            else if (b & mask) out |= 0x00;   // 00 黑
            else out |= 0x01;                 // 01 白
          }
          *o++ = out;
        }
      }
    }
    epd2.writeNative(stage, 0, 0, y, W, PAGE_ROWS);  // 末页内部自动收尾
  }
  const uint32_t tPack = millis() - t0;
  Serial.printf("[屏] 三平面→2bpp 分页推送完成（%ums），触发全刷（约 21s，勿断电）...\n", (unsigned)tPack);

  const uint32_t t1 = millis();
  epd2.refresh(false);  // 整帧刷新，内部等待 BUSY
  // BUSY 保险丝：四色全刷真实时长约 21.8s（实测口径）。若 refresh 返回得
  // 异常快，说明 BUSY 反馈未回（线松/驱动异常）——面板仍在刷波形，此刻
  // 发后续命令会打断刷新（花屏风险）。按最坏时长保守补等，绝不盲发。
  constexpr uint32_t kMinRefreshMs = 21000;
  const uint32_t tRef = millis() - t1;
  if (tRef < kMinRefreshMs) {
    Serial.printf("[屏] ⚠ BUSY 反馈异常（%ums 即返回，正常约 21800ms）——按最坏时长保守等待，勿断电\n", (unsigned)tRef);
    delay(kMinRefreshMs - tRef);
  }
  epaper::powerOff();  // 断驱动电压、保持唤醒态（DSLP 深睡后唤不醒，v0.8.0 实测教训）
  if (wifiWasOn) {
    esp_wifi_start();
    wifi::kickReconnect();  // 不等 15s 退避，立即重连
    Serial.println("[屏] 射频已恢复，Wi-Fi 重连已触发");
  }
  Serial.printf("[屏] 全刷完成 %.1fs（打包推送 %ums + 刷新等待 %ums）\n",
                (millis() - t0) / 1000.0, (unsigned)tPack, (unsigned)(millis() - t1));
  return true;
}

}  // namespace canvas
