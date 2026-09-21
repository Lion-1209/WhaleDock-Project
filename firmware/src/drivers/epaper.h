#pragma once
// ============================================================
// 驱动层 · 7.5" 四色墨水屏（黑白红黄，8pin 模组，SPI2/FSPI，GxEPD2）
// 驱动类按屏 FPC 排线丝印型号选择（换屏时只改下面 using 那行）：
//   丝印 GDEM075F52（800×480 四色，控制器 JD79665AA）→ GxEPD2_750c_GDEM075F52
//
// 四色屏全刷约 21s，BUSY 等待期间勿断电。驱动标记 hasPartialUpdate=true
// 但彩色残迹风险未验证，架构仍按"整点齐刷"，局刷留 B1 实机实验、勿默认启用。
// （全刷阻塞调用方、呼吸灯暂停；任务拆分待 §9 RTOS 规划落地）。
// ============================================================

#include <GxEPD2_4C.h>  // 四色（黑白红黄）驱动家族，已含全部 4C 驱动类

#include "pins.h"

namespace epaper {

// 黑白 + 红 + 黄三平面整屏缓冲（约 141KB），不做分页
using DriverT = GxEPD2_750c_GDEM075F52;
using DisplayT = GxEPD2_4C<DriverT, DriverT::HEIGHT>;

void init();          // SPI2 初始化 + 上电清屏
DisplayT& display();  // 渲染入口：画完调用 display().display() 触发全刷
void hibernate();     // 深度下电保护屏体，静态画面保留

}  // namespace epaper
