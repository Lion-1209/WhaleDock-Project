#pragma once
// ============================================================
// 应用层 · 概念图 v2 示例帧（模拟器提取的固化资产，C2 后退役）
// show()：init + 三平面 memcpy + flush 整帧上屏（阻塞约 22s）
// ============================================================

#include <stdint.h>

#include "../canvas.h"

namespace concept_demo {

extern const uint8_t BW[canvas::PLANE_BYTES];
extern const uint8_t RED[canvas::PLANE_BYTES];
extern const uint8_t YELLOW[canvas::PLANE_BYTES];

void show();

}  // namespace concept_demo
