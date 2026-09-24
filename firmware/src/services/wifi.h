#pragma once
// ============================================================
// 服务层 · Wi-Fi 连接管理（M2 第一片）
// 事件状态机：esp_event 回调只拷数据+置标志（对齐开发约定 §9），
// 连接/重试逻辑全部在 poll() 的应用上下文执行，任何路径不阻塞。
//
// 断开自愈：RetryWait 态按 WIFI_RETRY_INTERVAL_MS 退避重连；
// 凭据存 NVS（Preferences，键值型/磨损均衡/掉电安全）——有意区别于
// 数据缓存（走 LittleFS，storage 模块 M2 后续落位）。
// ============================================================

namespace wifi {

enum class State { Disabled, Connecting, Connected, RetryWait };

void begin();
void kickReconnect();  // 射频恢复后立即重连（不等退避）  // 装载事件钩子；有已存凭据则自动开始连接
void poll();   // loop 调用：消化事件标志 + 超时/重连调度（非阻塞）

void connect(const char* ssid, const char* pass);  // 保存凭据到 NVS 并连接
void forget();                                     // 清除凭据并断开

State state();
const char* ssid();  // 当前/已存 SSID，无则空串
const char* ip();    // 已连接时的 IP，否则 "--"

}  // namespace wifi
