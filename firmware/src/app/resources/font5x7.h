#pragma once
// 5x7 ASCII 字库（0x20-0x7E，源自 Adafruit-GFX glcdfont）——供任意缩放字形渲染
#include <stdint.h>

namespace font5x7 {
constexpr int GLYPHS = 95;      // 空格起共 95 个
constexpr int BYTES = 5;        // 每字形 5 列字节（bit0..bit4 = 行 0..4? glcdfont 列内 bit0 在顶）
extern const uint8_t DATA[GLYPHS][BYTES];
}  // namespace font5x7
