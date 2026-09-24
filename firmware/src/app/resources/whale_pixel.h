#pragma once
// ============================================================
// 应用层 · 内置资源：像素鲸鱼 whale_pixel（pet widget 默认资源）
// 提取自模拟器示例（tools/pixelize 从概念图生成，220×192 1bpp），
// 位布局与协议 resources 一致：行主序 MSB 前 bit=1=黑。
// C2 引擎落地资产；编辑器后续可经 resources 覆盖。
// ============================================================

#include <stdint.h>

namespace whale_pixel {
constexpr int W = 220;
constexpr int H = 192;
constexpr int STRIDE = 28;  // (W+7)/8
extern const uint8_t BITS[STRIDE * H];
}  // namespace whale_pixel
