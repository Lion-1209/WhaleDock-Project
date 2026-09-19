#pragma once
// ============================================================
// 鲸屿 WhaleDock · 编译期配置（调机/换件只改这里）
// 产品引脚规划不在此文件，见 pins.h（与开发约定§4 同步）
// ============================================================

#include <stdint.h>

// 屏未到货 = false：跳过点屏，只跑板级自检 + 呼吸心跳；屏到货改 true
constexpr bool SCREEN_ATTACHED = false;

constexpr uint32_t SERIAL_BAUD = 115200;         // 原生 USB CDC
constexpr uint32_t HEARTBEAT_PERIOD_MS = 10000;  // 串口心跳周期
constexpr uint32_t BREATH_PERIOD_MS = 3000;      // 呼吸灯周期
constexpr uint8_t BREATH_MAX = 32;               // 呼吸峰值亮度（WS2812 满亮刺眼）
constexpr uint8_t BREATH_MIN = 6;                // 呼吸谷值亮度（保持可见微光）
