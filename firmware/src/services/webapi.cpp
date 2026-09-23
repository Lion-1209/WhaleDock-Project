#include "webapi.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_ota_ops.h>
#include <mbedtls/base64.h>

#include <string.h>

#include "app/canvas.h"
#include "app/epaper_selftest.h"
#include "config.h"
#include "datapipe.h"
#include "drivers/epaper.h"
#include "ntp.h"
#include "ota.h"
#include "storage.h"
#include "wifi.h"

namespace webapi {

namespace {

WebServer server(80);
bool sMdnsStarted = false;

void sendJson(int code, const String& body) {
  server.send(code, "application/json", body);
}

void sendOk(const char* msg = "ok") {
  JsonDocument doc;
  doc["ok"] = true;
  doc["msg"] = msg;
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

void sendErr(int code, const char* msg) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["msg"] = msg;
  String out;
  serializeJson(doc, out);
  sendJson(code, out);
}

// CORS 预检：enableCORS(true) 会给所有响应自动加 CORS 头，
// 这里绝不能再手动加——重复的 Access-Control-Allow-Origin 头会被浏览器
// 判定非法（curl 不检查，浏览器直接拒绝），预检即失败
void handleOptions() {
  server.send(204, "text/plain", "");
}

const char* wifiStateName() {
  switch (wifi::state()) {
    case wifi::State::Disabled: return "未配网";
    case wifi::State::Connecting: return "连接中";
    case wifi::State::Connected: return "已连接";
    case wifi::State::RetryWait: return "重连中";
  }
  return "?";
}

void hStatus() {
  JsonDocument doc;
  doc["version"] = FW_VERSION;
  doc["uptime"] = millis() / 1000;
  doc["heap"] = ESP.getFreeHeap() / 1024;
  doc["psram"] = ESP.getFreePsram() / 1024;
  doc["wifi"]["state"] = wifiStateName();
  doc["wifi"]["ssid"] = wifi::ssid();
  doc["wifi"]["ip"] = wifi::ip();
  doc["wifi"]["rssi"] =
      wifi::state() == wifi::State::Connected ? WiFi.RSSI() : 0;
  doc["time"] = ntp::timeString();
  doc["ntpSynced"] = ntp::synced();
  doc["partition"] = esp_ota_get_running_partition()->label;
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

void hConfigGet() {
  storage::Config c;
  storage::loadConfig(c);
  JsonDocument doc;
  doc["githubUser"] = c.githubUser;
  doc["githubRepo"] = c.githubRepo;
  doc["tokenSet"] = !c.githubToken.isEmpty();  // 只报状态，不回显
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

void hConfigPost() {
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendErr(400, "JSON 解析失败");
    return;
  }
  storage::Config c;
  storage::loadConfig(c);
  if (!doc["githubUser"].isNull()) {
    const String v = doc["githubUser"].as<String>();
    if (v.length()) c.githubUser = v == "-" ? "" : v;
  }
  if (!doc["githubRepo"].isNull()) {
    const String v = doc["githubRepo"].as<String>();
    if (v.length()) c.githubRepo = v == "-" ? "" : v;
  }
  if (!doc["githubToken"].isNull()) {
    const String v = doc["githubToken"].as<String>();
    if (v.length()) c.githubToken = v == "-" ? "" : v;
  }
  sendOk(storage::saveConfig(c) ? "已保存" : "保存失败（文件系统）");
}

void hWifiPost() {
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendErr(400, "JSON 解析失败");
    return;
  }
  const String ssid = doc["ssid"] | "";
  const String pass = doc["pass"] | "";
  if (!ssid.length() || pass.length() < 8) {
    sendErr(400, "ssid 必填且 pass >= 8 位");
    return;
  }
  wifi::connect(ssid.c_str(), pass.c_str());
  sendOk("凭据已保存并开始连接");
}

void hPipe() {
  datapipe::runOnce("网页");  // 阻塞至完成（数秒），结果见串口/缓存
  sendOk("流水已执行（结果见设备日志/缓存）");
}

void hOta() {
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendErr(400, "JSON 解析失败");
    return;
  }
  const String url = doc["url"] | "";
  if (!url.startsWith("http")) {
    sendErr(400, "url 必须以 http 开头");
    return;
  }
  sendOk("升级启动：下载写入后设备将重启（期间 API 短暂无响应）");
  server.handleClient();  // 尽量把应答送出去
  delay(300);
  ota::fromUrl(url.c_str());  // 成功则内部重启，失败则返回
}

void hReboot() {
  sendOk("重启中…");
  server.handleClient();
  delay(500);
  ESP.restart();
}

// ---- B3 位图通道（协议 §7 display 的 HTTP 装载形态）----
// 传输采用分块 b64：POST /api/display/{bw|red|yellow}?off=<字节偏移>，
// body = 该块裸数据的 base64（每块 ≤12000 字节）。两重原因：
//   ① WebServer 的 plain 参数按 String(char*) 构造，内嵌 NUL 的二进制会被
//     截断（实测全零平面 body.length()==0）；
//   ② 一次性 64-192KB body 的 String 增长峰值有 OOM 风险，分块后每块峰值 <50KB。
// 全部推完 flush 上屏；语义对齐 §7：flush 时未推送的平面 = 全白。
constexpr size_t kChunkRaw = 12000;  // 每块裸字节数上限（b64 后 16000 字符）
uint8_t sPlaneSeen = 0;              // bit0 BW / bit1 RED / bit2 YELLOW，flush 后清零

void recvPlane(uint8_t idx, const char* name) {
  const String ctype = server.header("Content-Type");
  if (ctype.length() && (ctype.startsWith("application/x-www-form-urlencoded") || ctype.startsWith("multipart/"))) {
    // form 类 Content-Type 会被 WebServer 当表单解析，body 不进 arg("plain")
    // （curl --data / urllib 默认就是 form-urlencoded，实测 400 且日志报
    //  _parseArguments arg missing value）——客户端须显式 text/plain
    sendErr(400, "Content-Type 勿用 form 类，请 text/plain 直发 base64");
    return;
  }
  if (!server.hasArg("off")) {
    sendErr(400, "需 ?off=<字节偏移>（0..47999，4 的倍数）");
    return;
  }
  const long off = server.arg("off").toInt();
  const String& b64 = server.arg("plain");
  if (off < 0 || off >= canvas::PLANE_BYTES || off % 4 != 0 || b64.isEmpty() ||
      b64.length() > ((kChunkRaw + 2) / 3) * 4) {  // b64 长度 = ceil(裸字节/3)*4
    sendErr(400, "off 越界或 b64 块尺寸非法（每块 ≤12000 字节）");
    return;
  }
  size_t rawLen = 0;
  const int rc = mbedtls_base64_decode(nullptr, 0, &rawLen, (const uint8_t*)b64.c_str(), b64.length());
  if (rc != 0 && rc != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {  // 探长调用按约定返回 TOO_SMALL
    sendErr(400, "base64 解析失败");
    return;
  }
  if (off + rawLen > canvas::PLANE_BYTES) {
    sendErr(400, "块超出平面（48000 字节）");
    return;
  }
  if (!canvas::get().begin()) {
    sendErr(503, "PSRAM 三平面分配失败");
    return;
  }
  static uint8_t chunk[kChunkRaw];  // 12KB 静态中转（避免栈上大缓冲）
  size_t written = 0;
  if (mbedtls_base64_decode(chunk, sizeof(chunk), &written, (const uint8_t*)b64.c_str(), b64.length()) || written != rawLen) {
    sendErr(400, "base64 解码异常");
    return;
  }
  memcpy(canvas::get().plane(idx) + off, chunk, rawLen);
  sPlaneSeen |= 1 << idx;
  char msg[64];
  snprintf(msg, sizeof(msg), "%s +[%ld,%zu)", name, off, rawLen);
  sendOk(msg);
}

void hDisplayBw() { recvPlane(canvas::PL_BW, "黑白平面"); }
void hDisplayRed() { recvPlane(canvas::PL_RED, "红平面"); }
void hDisplayYellow() { recvPlane(canvas::PL_YELLOW, "黄平面"); }

void hDisplayFlush() {
  if (!SCREEN_ATTACHED) {
    sendErr(503, "SCREEN_ATTACHED=false，屏未启用");
    return;
  }
  if (!canvas::get().begin()) {
    sendErr(503, "PSRAM 三平面分配失败");
    return;
  }
  for (uint8_t p = 0; p < 3; p++)  // 协议 §7：省略的平面 = 全白
    if (!(sPlaneSeen & (1 << p))) memset(canvas::get().plane(p), 0, canvas::PLANE_BYTES);
  sPlaneSeen = 0;
  // 先应答后执行：22s 阻塞期间 TCP 会被断（errno 113 实测），客户端拿到
  // 的是连接中断而非结果——应答改为"已受理"，刷新结果看设备日志/屏
  sendOk("已受理：约 22s 完成上屏（期间设备无响应，勿重复推送）");
  server.handleClient();  // 把应答真正送出去
  delay(200);
  Serial.println("[Web] 位图通道 flush：整帧上屏…");
  epaper::init();  // RST 脉冲唤醒/复位面板（幂等）
  canvas::flush();  // 内部含射频静默、BUSY 保险丝与全刷计时
}

void registerRoutes() {
  server.on("/api/status", HTTP_GET, hStatus);
  server.on("/api/config", HTTP_GET, hConfigGet);
  server.on("/api/config", HTTP_POST, hConfigPost);
  server.on("/api/wifi", HTTP_POST, hWifiPost);
  server.on("/api/pipe", HTTP_POST, hPipe);
  server.on("/api/ota", HTTP_POST, hOta);
  server.on("/api/reboot", HTTP_POST, hReboot);
  server.on("/api/display/bw", HTTP_POST, hDisplayBw);
  server.on("/api/display/red", HTTP_POST, hDisplayRed);
  server.on("/api/display/yellow", HTTP_POST, hDisplayYellow);
  server.on("/api/display/flush", HTTP_POST, hDisplayFlush);
  // 各路径的 CORS 预检
  const char* paths[] = {"/api/status", "/api/config", "/api/wifi",
                         "/api/pipe", "/api/ota", "/api/reboot",
                         "/api/display/bw", "/api/display/red",
                         "/api/display/yellow", "/api/display/flush"};
  for (const char* p : paths) server.on(p, HTTP_OPTIONS, handleOptions);
  server.onNotFound([]() { sendErr(404, "not found（API 见 /api/*）"); });
  const char* collect[] = {"Content-Type"};  // recvPlane 的 form 类防御需要读它
  server.collectHeaders(collect, 1);
  server.enableCORS(true);
}

}  // namespace

void begin() {
  registerRoutes();
  server.begin();
  Serial.println("[Web] HTTP API 已启动（:80，接口见 webapi.h 注释）");
}

void poll() {
  // mDNS 依赖网络栈就绪，联网成功后注册一次
  if (!sMdnsStarted && wifi::state() == wifi::State::Connected) {
    const String host = "whaledock-" + String((uint16_t)(ESP.getEfuseMac() >> 32), HEX);
    if (MDNS.begin(host.c_str())) {
      MDNS.addService("http", "tcp", 80);
      Serial.printf("[Web] mDNS 已注册：http://%s.local\n", host.c_str());
      sMdnsStarted = true;
    }
  }
  server.handleClient();
}

}  // namespace webapi
