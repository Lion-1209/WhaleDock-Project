#include "ntp.h"

#include <Arduino.h>
#include <esp_timer.h>
#include <time.h>

#include "config.h"
#include "wifi.h"

namespace ntp {

namespace {

// 早于该时刻视为"未同步"（芯片冷启动时钟从 1970 起步）
constexpr time_t kMinValidEpoch = 1600000000;  // 2020-09

// 国内可达优先，pool 兜底
constexpr const char* kServers[] = {"ntp.aliyun.com", "pool.ntp.org"};

volatile bool sTick = false;   // esp_timer 回调 → poll 的唯一信号
bool sConfigured = false;
bool sSynced = false;
bool sWarned = false;
uint32_t sWifiReadyAt = 0;
esp_timer_handle_t sTimer = nullptr;
HourlyCallback sHourlyCb = nullptr;
char sTimeBuf[24] = "未同步";

const char* fmt(time_t t) {
  struct tm tmv;
  localtime_r(&t, &tmv);
  strftime(sTimeBuf, sizeof(sTimeBuf), "%Y-%m-%d %H:%M:%S", &tmv);
  return sTimeBuf;
}

void onTimer(void*) {
  sTick = true;  // 回调上下文只置标志（§9）
}

// 布防"下一次整点"的一次性定时器（调用方须已同步）
void arm() {
  const time_t t = time(nullptr);
  const uint64_t waitSec =
      SCHEDULE_DEBUG ? 60 : (uint64_t)(3600 - (t % 3600));
  esp_timer_start_once(sTimer, waitSec * 1000000ULL);
  if (SCHEDULE_DEBUG) {
    Serial.printf("[NTP] 调度已布防（调试模式：60s 周期，正式节拍须关闭）\n");
  } else {
    Serial.printf("[NTP] 整点调度：下一次 %s（%llu s 后）\n",
                  fmt(t + (time_t)waitSec), (unsigned long long)waitSec);
  }
}

}  // namespace

void poll() {
  // 1) Wi-Fi 连上后配置 SNTP（仅一次；lwIP SNTP 之后自动周期校时）
  if (!sConfigured && wifi::state() == wifi::State::Connected) {
    configTzTime("CST-8", kServers[0], kServers[1], nullptr);
    sConfigured = true;
    sWifiReadyAt = millis();
    Serial.println("[NTP] Wi-Fi 已连接，启动 SNTP 对时（UTC+8）");
  }

  // 2) 轮询系统时间判定同步完成
  if (sConfigured && !sSynced && time(nullptr) > kMinValidEpoch) {
    sSynced = true;
    Serial.printf("[NTP] 对时成功：%s\n", fmt(time(nullptr)));
    if (!sTimer) {
      esp_timer_create_args_t args = {};
      args.callback = onTimer;
      args.name = "whaledock_hourly";
      args.skip_unhandled_events = true;
      esp_timer_create(&args, &sTimer);
    }
    arm();
  }

  // 3) 同步偏慢提醒（SNTP 自身持续重试，不需要干预）
  if (sConfigured && !sSynced && !sWarned &&
      millis() - sWifiReadyAt > 60000) {
    sWarned = true;
    Serial.println("[NTP] 对时 60s 未完成（NTP 服务器不可达？），将持续自动重试");
  }

  // 4) 整点触发：处理标志 → 回调 → 重布防（全部在应用上下文）
  if (sTick) {
    sTick = false;
    Serial.printf("[调度] 整点触发 %s\n", fmt(time(nullptr)));
    if (sHourlyCb) sHourlyCb();
    arm();
  }
}

bool synced() { return sSynced; }

const char* timeString() {
  if (!sSynced) return "未同步（Wi-Fi 连接后自动对时）";
  return fmt(time(nullptr));
}

void setHourlyCallback(HourlyCallback cb) { sHourlyCb = cb; }

}  // namespace ntp
