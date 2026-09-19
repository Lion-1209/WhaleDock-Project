#pragma once
// ============================================================
// 服务层 · SNTP 对时 + 整点调度（M2 · A1）
// ESP32 无电池时钟，每次断电重启时间归零；Wi-Fi 连接后经 SNTP
// 同步真实时间（UTC+8），随后用 esp_timer 一次性定时器布防"下一个整点"，
// 触发后回调——整点拉数据流水（A4）从这里起步。
//
// §9 约定：esp_timer 回调只置标志，实际处理与重布防都在 poll() 完成；
// 不阻塞（同步完成靠轮询 time() 判断，不用带超时的 getLocalTime 卡等）。
// 时区固定 UTC+8（POSIX TZ "CST-8"，无夏令时）。
// ============================================================

// 整点触发回调（在 loopTask 上下文经 poll() 调用，可做重活）
typedef void (*HourlyCallback)();

namespace ntp {

void poll();  // loop 调用：Wi-Fi 连上后配置 SNTP；同步完成后布防整点定时器

bool synced();                    // 是否已完成对时
const char* timeString();         // "YYYY-MM-DD HH:MM:SS"（未同步时为提示串）
void setHourlyCallback(HourlyCallback cb);  // A4 数据流水挂载点（暂为空转）

}  // namespace ntp
