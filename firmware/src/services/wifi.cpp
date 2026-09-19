#include "wifi.h"

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

#include "config.h"

namespace wifi {

namespace {

constexpr const char* kPrefsNs = "whaledock";
constexpr const char* kKeySsid = "wifi_ssid";
constexpr const char* kKeyPass = "wifi_pass";

// 事件回调上下文（core0 event loop）→ 应用上下文（loopTask）只经这几个标志
volatile bool sGotIp = false;
volatile bool sDisconnected = false;
volatile uint8_t sDiscReason = 0;

State sState = State::Disabled;
String sSsid, sPass, sIp = "--";
uint32_t sConnectStarted = 0;
uint32_t sRetryAt = 0;

void startConnect() {
  Serial.printf("[WiFi] 连接中：%s\n", sSsid.c_str());
  WiFi.begin(sSsid.c_str(), sPass.c_str());
  sState = State::Connecting;
  sConnectStarted = millis();
}

void enterRetryWait() {
  sState = State::RetryWait;
  sRetryAt = millis() + WIFI_RETRY_INTERVAL_MS;
}

void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      sGotIp = true;  // IP 等重活回 poll() 里取
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      sDisconnected = true;
      sDiscReason = info.wifi_sta_disconnected.reason;
      break;
    default:
      break;
  }
}

}  // namespace

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);  // 重连由本模块状态机管理，不用框架内置的
  WiFi.onEvent(onWifiEvent);

  Preferences prefs;
  // 读写模式打开：命名空间不存在时自动创建（首次启动），
  // 只读模式此时会打 nvs_open failed 日志；不 put 则无写入磨损。
  // isKey 先探再读：直接 getString 对缺失键会打 NOT_FOUND 错误日志
  prefs.begin(kPrefsNs, false);
  if (prefs.isKey(kKeySsid)) sSsid = prefs.getString(kKeySsid, "");
  if (prefs.isKey(kKeyPass)) sPass = prefs.getString(kKeyPass, "");
  prefs.end();

  if (sSsid.isEmpty()) {
    Serial.println("[WiFi] 无已存凭据，串口 CLI 配网：wifi set <ssid>,<pass>");
    return;
  }
  startConnect();
}

void poll() {
  if (sGotIp) {
    sGotIp = false;
    sState = State::Connected;
    sIp = WiFi.localIP().toString();
    Serial.printf("[WiFi] 已连接：%s  IP=%s\n", sSsid.c_str(), sIp.c_str());
  }
  if (sDisconnected) {
    sDisconnected = false;
    // Disabled = wifi clear 的主动断开，不进重连；其余断开（含 NO_AP_FOUND）
    // 一律自愈重试
    if (sState != State::Disabled) {
      if (sState == State::Connected) sIp = "--";
      enterRetryWait();
      Serial.printf("[WiFi] 断开（原因 %u），%us 后自动重连\n", sDiscReason,
                    (unsigned)(WIFI_RETRY_INTERVAL_MS / 1000));
    }
  }

  const uint32_t now = millis();
  if (sState == State::RetryWait && !sSsid.isEmpty() &&
      (int32_t)(now - sRetryAt) >= 0) {
    startConnect();  // 退避到点，重连
  } else if (sState == State::Connecting &&
             now - sConnectStarted > WIFI_CONNECT_TIMEOUT_MS) {
    Serial.println("[WiFi] 连接超时（30s 无事件），进入重连等待");
    enterRetryWait();
  }
}

void connect(const char* ssid, const char* pass) {
  sSsid = ssid;
  sPass = pass;
  Preferences prefs;
  prefs.begin(kPrefsNs, false);
  prefs.putString(kKeySsid, sSsid);
  prefs.putString(kKeyPass, sPass);
  prefs.end();
  Serial.printf("[WiFi] 凭据已保存（NVS）：%s\n", sSsid.c_str());
  startConnect();
}

void forget() {
  Preferences prefs;
  prefs.begin(kPrefsNs, false);
  prefs.remove(kKeySsid);
  prefs.remove(kKeyPass);
  prefs.end();
  WiFi.disconnect();
  sSsid = sPass = "";
  sIp = "--";
  sState = State::Disabled;
  Serial.println("[WiFi] 凭据已清除，停止连接");
}

State state() { return sState; }
const char* ssid() { return sSsid.c_str(); }
const char* ip() { return sIp.c_str(); }

}  // namespace wifi
