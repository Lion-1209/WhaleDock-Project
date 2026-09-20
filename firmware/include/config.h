#pragma once
// ============================================================
// 鲸屿 WhaleDock · 编译期配置（调机/换件只改这里）
// 产品引脚规划不在此文件，见 pins.h（与开发约定§4 同步）
// ============================================================

#include <stdint.h>

// 固件版本（OTA 升级前后在日志/网页可见；发版必须递增）
constexpr const char* FW_VERSION = "0.4.0";

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

// OTA（services/ota）：新固件"待验证"状态健康运行超时后自动确认（回滚点清除）
constexpr uint32_t OTA_CONFIRM_MS = 90000;

// GitHub（services/github）：API 基址 + 可选代理前缀（应对国内可达性，空 = 直连；
// 填形如 "http://192.168.1.100:8080/gh/" 的中转时走普通 HTTP）
constexpr const char* GITHUB_API_BASE = "https://api.github.com";
constexpr const char* GITHUB_PROXY_PREFIX = "";

// 数据源出厂默认（storage::Config 无配置文件时生效）。
// 注意用 datawhalechina：GitHub 字面 "datawhale" 是 0 仓库空账号（勘误 #6）。
// 改动此处须同步 console/js/app.js 的 GH_DEFAULT_USER
constexpr const char* GITHUB_DEFAULT_USER = "datawhalechina";
