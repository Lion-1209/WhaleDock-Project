#pragma once
// ============================================================
// 鲸屿 WhaleDock · 引脚规划
// 与开发约定§4 保持一致，改动前先改文档再改这里
//
// 已被模组占用的引脚（不可用）：
//   GPIO26–32 模组 Quad Flash；GPIO33–37 Octal PSRAM（R8 版）
//   GPIO19/20 USB D−/D+；GPIO0/3/45/46 strapping，慎用
// ============================================================

// ---- 7.5" 四色墨水屏（黑白红黄，8pin 模组，走 SPI2/FSPI）----
constexpr int EPD_CS   = 10;   // 片选
constexpr int EPD_DC   = 9;    // 数据/命令
constexpr int EPD_RST  = 14;   // 复位
constexpr int EPD_BUSY = 13;   // 忙状态（输入）
constexpr int EPD_CLK  = 12;   // SPI2 SCK
constexpr int EPD_DIN  = 11;   // SPI2 MOSI

// ---- 预留（后续里程碑启用，规划时勿挪用）----
// WS2812 水线灯 ×2：RMT 外设驱动（led_strip 组件）
// 按键 ×2：空闲 IO + 内部上拉，esp_timer 轮询去抖
