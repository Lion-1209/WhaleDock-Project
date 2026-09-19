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
constexpr uint8_t BREATH_MAX = 10;               // 呼吸峰值亮度（实测 32 晃眼，桌面级微光）
constexpr uint8_t BREATH_MIN = 2;                // 呼吸谷值亮度（隐约可见）

// Wi-Fi（services/wifi）：断开退避重连间隔、单次连接超时
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 15000;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 30000;

// 调度（services/ntp）：true 时整点调度改为 60s 周期（验收用），正式节拍必须 false
constexpr bool SCHEDULE_DEBUG = false;

// GitHub（services/github）：API 基址 + 可选代理前缀（应对国内可达性，空 = 直连；
// 填形如 "http://192.168.1.100:8080/gh/" 的中转时走普通 HTTP）
constexpr const char* GITHUB_API_BASE = "https://api.github.com";
constexpr const char* GITHUB_PROXY_PREFIX = "";
