#pragma once
// ============================================================
// 应用层 · B2 点屏自检（canvas 三平面管线验证）
// 与 B1 同版式四色测试图，绘制走 app/canvas（PSRAM 三平面 + GFX），
// 上屏走 canvas::flush（2bpp 打包分页推送 + 全刷计时）。
// 流程：epaper::init → canvas::begin → 渲染 → flush（全刷约 22s 阻塞，勿断电）。
// ============================================================

namespace epaper_selftest {

void run();  // B1 全流程（全刷约 21s 期间阻塞调用方，勿断电）

}  // namespace epaper_selftest
