#pragma once
// ============================================================
// 应用层 · 渲染画布（B2）：PSRAM 三平面（黑白/红/黄各 48000B）
// 与《显示协议-v1.md》§7 位图通道、console 模拟器三平面同构：
// 行主序、MSB 在前、bit=1 表示该色、全零为白。
//
// 绘制 = Adafruit_GFX 语义（fillRect/drawRect/print 文本直接可用），
// drawPixel/fillScreen 落三平面；flush() 打包 2bpp（00黑/01白/10黄/11红）
// 经驱动分页整帧推送并触发全刷。C2 Widget 引擎与本画布对接。
//
// 内存：三平面共 141KB，heap_caps_malloc(MALLOC_CAP_SPIRAM) 显式入
// PSRAM（设计约定，内部 RAM 只留 3.2KB 分页中转缓冲）。
// ============================================================

#include <Adafruit_GFX.h>

namespace canvas {

constexpr int16_t W = 800;
constexpr int16_t H = 480;
constexpr int PLANE_BYTES = W * H / 8;  // 48000，与协议 §7 校验规则 9 一致

// 平面索引（协议顺序：黑白/红/黄）
enum Plane : uint8_t { PL_BW = 0, PL_RED = 1, PL_YELLOW = 2 };

// GFX 画布：图元与文本继承 Adafruit_GFX，像素写入三平面
class Canvas : public Adafruit_GFX {
 public:
  Canvas() : Adafruit_GFX(W, H) {}
  bool begin();  // 三平面 PSRAM 分配（幂等，全有或全无）；失败返回 false
  bool ready() const { return planes_[0] != nullptr; }
  uint8_t* plane(uint8_t p) { return planes_[p]; }  // B3 位图通道直写（48000B）
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void fillScreen(uint16_t color) override;

 private:
  uint8_t* planes_[3] = {nullptr, nullptr, nullptr};
};

Canvas& get();  // 全局画布（绘图入口）

// 整帧推送：三平面 → 2bpp 分页 writeNative → refresh 全刷 → 屏体休眠。
// 阻塞约 22s（全刷期间勿断电）；返回 false = 画布未就绪
bool flush();

}  // namespace canvas
