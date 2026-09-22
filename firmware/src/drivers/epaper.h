#pragma once
// ============================================================
// 驱动层 · 7.5" 四色墨水屏（黑白红黄，8pin 模组，SPI2/FSPI，GxEPD2）
// 驱动类按屏 FPC 排线丝印型号选择（换屏时只改下面 using 那行）：
//   丝印 GDEM075F52（800×480 四色，控制器 JD79665AA）→ GxEPD2_750c_GDEM075F52
//   丝印 DFG0750RYS…（DKE，880×528 四色，同族控制器）→ GxEPD2_750c_DFG0750RYS
//
// B1 实测定案（2026-09-22）：到货屏实为 **800×480**（手册标称 880×528 系卖家
// 错配文档——880 驱动实测下屏只显示上部 480 行、底部文字/角标被裁，即证）。
// FPC 丝印 FPC-8612 为排线板号非屏型号。定稿用库原生 800×480 驱动类。
//
// 四色屏全刷实测 21.8s（与手册 21s 一致），BUSY 等待期间勿断电。
// 驱动标记 hasPartialUpdate=true 但彩色残迹风险未验证，架构仍按"整点齐刷"，
// 局刷留 B1 实机实验、勿默认启用。（全刷阻塞调用方、呼吸灯暂停；任务拆分待
// §9 RTOS 规划落地）。
//
// 接线（EVK011 转接板 J2 → DevKitC-1，2026-09-22 B1 实测通过）：
//   J2-3 SCK→GPIO12  J2-5 SDO→GPIO11(MOSI)  J2-6 CS→GPIO10  J2-7 D/C→GPIO9
//   J2-8 RES→GPIO14  J2-9 BUSY→GPIO13       J2-10 BS→GND（4 线 SPI）
//   J2-15 GND→GND    J2-16 VCI→3V3
//   ⚠ J2-5 丝印 SDO = 屏 SDA（唯一数据线，双向；MCU 侧即 MOSI），非屏读回脚
//   ⚠ J2-13 VCI_EN 本机实测悬空即可（VCI 有输出，屏已点亮）
// ============================================================

#include <GxEPD2_4C.h>  // 四色（黑白红黄）驱动家族，已含全部 4C 驱动类

#include "GxEPD2_750c_DFG0750RYS.h"  // B1 备选：880×528 本地派生驱动（默认未启用）

#include "pins.h"

namespace epaper {

// 黑白 + 红 + 黄三平面整屏缓冲（约 141KB），不做分页
using DriverT = GxEPD2_750c_GDEM075F52;  // 定稿（09-22 B1 实测定案：屏物理 800×480）
// using DriverT = GxEPD2_750c_DFG0750RYS;  // 880×528 实验（已排除，见文件头）
using DisplayT = GxEPD2_4C<DriverT, DriverT::HEIGHT>;

void init();          // SPI2 初始化 + 上电清屏
DisplayT& display();  // 渲染入口：画完调用 display().display() 触发全刷
void hibernate();     // 深度下电保护屏体，静态画面保留

}  // namespace epaper
