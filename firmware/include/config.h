#pragma once
// ============================================================
// 鲸屿 WhaleDock · 编译期配置（调机/换件只改这里）
// 产品引脚规划不在此文件，见 pins.h（与开发约定§4 同步）
// ============================================================

#include <stdint.h>

// 固件版本（OTA 升级前后在日志/网页可见；发版必须递增）
// 0.5.0：显示协议 v1 冻结 + 布局校验（layout sample|check）+ 四色驱动类切换
// 0.5.1：协议 v1.1 追加（slotRect / heatMap / stats 聚合），示例改概念图复刻版
// 0.6.0：B1 实机点屏——SCREEN_ATTACHED 开闸 + 四色测试图（黑/白/红/黄）
// 0.7.0：B2 canvas 三平面——PSRAM 帧缓冲 + 2bpp 打包分页整帧推送管线
// 0.7.1：概念图 v2 示例帧固化上屏（模拟器三平面提取）+ init 硬复位唤醒 DSLP
//        + BUSY 保险丝（反馈异常时按最坏时长保守等待，防打断刷新波形）
// 0.8.0：B3 位图通道——HTTP /api/display/{bw,red,yellow,flush} 三平面裸推上屏
// 0.8.1：安全加固批次——设备配对码（写操作鉴权）+ GitHub/OTA 根证书校验 +
// 0.9.0：C2 Widget 引擎 + V5 任务拆分——布局实时渲染上屏 + 重活队列
//        OTA MD5 完整性校验 + token 迁 NVS + CORS 白名单 + 输入/路径校验
// 0.11.0：中文字库子集（协议 §6）——编辑器按用字生成随布局下发，设备端
//        UTF-8 解码 + 位图 blit；预览/设备同源渲染（一份位图两处画）
// 0.12.0：dataSources 打通（协议 §5）——整点流水按布局声明逐源拉数落
//        per-id 缓存，挂件 source 绑定生效；热力图接真实周参与数据；
//        无声明回退设备配置，未绑定挂件回退全局缓存（存量布局零变化）
constexpr const char* FW_VERSION = "0.12.0";

// 屏到货（2026-09-22，DFG0750RYS677F71HP-D1，经 EVK011 转接板）= true：开机跑 B1 点屏自检
constexpr bool SCREEN_ATTACHED = true;

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
