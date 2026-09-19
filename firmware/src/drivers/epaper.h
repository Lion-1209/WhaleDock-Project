#pragma once
// ============================================================
// 驱动层 · 7.5" 三色墨水屏（微雪 8pin 模组，SPI2/FSPI，GxEPD2）
// 驱动类按屏 FPC 排线丝印型号选择（换屏时只改下面 using 那行）：
//   丝印 GDEW075Z08（800×480 三色，微雪 7.5" B 型）→ GxEPD2_750c_GDEW075Z08
//   丝印 GDEY075Z08（新版屏）                       → GxEPD2_750c_GDEY075Z08
//
// 三色屏无局刷：任何更新 = 15–25s 全屏刷新，BUSY 等待期间勿断电
// （全刷阻塞调用方、呼吸灯暂停；任务拆分待 §9 RTOS 规划落地）。
// ============================================================

#include <GxEPD2_3C.h>  // 该头文件已包含全部三色驱动类，无需再 include 具体驱动

#include "pins.h"

namespace epaper {

// 黑白 + 红双平面整屏缓冲（96KB），不做分页
using DriverT = GxEPD2_750c_GDEW075Z08;
using DisplayT = GxEPD2_3C<DriverT, DriverT::HEIGHT>;

void init();          // SPI2 初始化 + 上电清屏
DisplayT& display();  // 渲染入口：画完调用 display().display() 触发全刷
void hibernate();     // 深度下电保护屏体，静态画面保留

}  // namespace epaper
