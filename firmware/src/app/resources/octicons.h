#pragma once
// GitHub Octicons（primer/octicons 16px，MIT）22px 栅格化 1bpp——与模拟器 drawIcon 同源同缩放
#include <stdint.h>

namespace octicons {
constexpr int SIZE = 22;   // 22x22
constexpr int BYTES = 3;   // 22bit 行宽 → 3 字节（MSB 起）
constexpr int ROWS = 22;
extern const uint8_t REPO[ROWS][BYTES];
extern const uint8_t STAR[ROWS][BYTES];
extern const uint8_t FORK[ROWS][BYTES];
extern const uint8_t PEOPLE[ROWS][BYTES];
const uint8_t* of(int kind);  // 0 repo / 1 star / 2 fork / 3 people
}  // namespace octicons
