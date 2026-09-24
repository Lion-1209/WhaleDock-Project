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
#include "worker.h"
#include "wifi.h"

namespace webapi {

namespace {

WebServer server(80);
bool sMdnsStarted = false;
IPAddress sMdnsIp;  // IP 变化时需重注册 mDNS（V7）

// ---- 设备配对码（V1）：eFuse MAC 派生 6 位数字，出厂贴机身标签 ----
// 同网段未授权者无法调用写操作端点（改配网/OTA/重启/推屏/改配置）
String deviceKey() {
  char buf[8];
  snprintf(buf, sizeof(buf), "%06u", (unsigned)(ESP.getEfuseMac() % 1000000));
  return String(buf);
}

bool keyOk() { return server.header("X-Device-Key") == deviceKey(); }

// ---- CORS 收敛（V4）：Origin 白名单（console 线上版 + 本地调试）----
// 教训（b2c75eb）：响应头只在此处加一次，勿与 enableCORS 叠加
void addCors() {
  const String o = server.header("Origin");
  if (!o.length()) return;  // 非 CORS 请求（curl / 同源）不加头
  const bool ok = o == "https://lion-1209.github.io" ||
                  o.startsWith("http://localhost:") ||
                  o.startsWith("http://127.0.0.1:");
  if (!ok) return;
  server.sendHeader("Access-Control-Allow-Origin", o);
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Device-Key");
  // Chromium 局域网访问策略（LNA）：localhost 页面访问局域网设备时，带自定义头
  // 的预检必须携带此头，否则 fetch 挂起（实测 127.0.0.1 页面 → 设备 IP 场景）
  server.sendHeader("Access-Control-Allow-Private-Network", "true");
}

void sendJson(int code, const String& body) {
  addCors();
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

// CORS 预检：白名单 Origin 回显 + 允许 X-Device-Key（自定义头必过预检）
void handleOptions() {
  addCors();
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
  const bool full = keyOk();
  JsonDocument doc;
  doc["version"] = FW_VERSION;
  doc["uptime"] = millis() / 1000;
  doc["heap"] = ESP.getFreeHeap() / 1024;
  doc["psram"] = ESP.getFreePsram() / 1024;
  doc["wifi"]["state"] = wifiStateName();
  doc["wifi"]["ssid"] = full ? wifi::ssid() : "*";  // V8：无配对码时脱敏
  doc["wifi"]["ip"] = wifi::ip();
  doc["wifi"]["rssi"] =
      wifi::state() == wifi::State::Connected ? WiFi.RSSI() : 0;
  doc["time"] = ntp::timeString();
  doc["ntpSynced"] = ntp::synced();
  if (full) doc["partition"] = esp_ota_get_running_partition()->label;
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

// V9：网络输入长度与字符集校验（防 LittleFS 撑爆与 URL 拼接注入）
bool validIdent(const String& v, bool allowSlash) {
  if (v.length() < 1 || v.length() > 64) return false;
  for (const char c : v) {
    const bool ok = isalnum((unsigned char)c) || c == '-' || c == '_' ||
                    c == '.' || (allowSlash && c == '/');
    if (!ok) return false;
  }
  return true;
}

bool validToken(const String& v) {
  if (v.length() > 255) return false;
  for (const char c : v)
    if (c < 0x21 || c > 0x7E) return false;  // 可见 ASCII，无空格
  return true;
}

void hConfigPost() {
  if (!keyOk()) {
    sendErr(401, "需要 X-Device-Key 请求头（设备串口 CLI 输入 key 查看）");
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendErr(400, "JSON 解析失败");
    return;
  }
  storage::Config c;
  storage::loadConfig(c);
  if (!doc["githubUser"].isNull()) {
    const String v = doc["githubUser"].as<String>();
    if (v.length()) {
      if (v != "-" && !validIdent(v, false)) {
        sendErr(400, "githubUser 非法（≤64，[A-Za-z0-9_.-]）");
        return;
      }
      c.githubUser = v == "-" ? "" : v;
    }
  }
  if (!doc["githubRepo"].isNull()) {
    const String v = doc["githubRepo"].as<String>();
    if (v.length()) {
      if (v != "-" && !validIdent(v, true)) {
        sendErr(400, "githubRepo 非法（≤64，[A-Za-z0-9_.-/]）");
        return;
      }
      c.githubRepo = v == "-" ? "" : v;
    }
  }
  if (!doc["githubToken"].isNull()) {
    const String v = doc["githubToken"].as<String>();
    if (v.length()) {
      if (v != "-" && !validToken(v)) {
        sendErr(400, "githubToken 非法（≤255 可见字符）");
        return;
      }
      c.githubToken = v == "-" ? "" : v;
    }
  }
  sendOk(storage::saveConfig(c) ? "已保存" : "保存失败（文件系统）");
}

void hWifiPost() {
  if (!keyOk()) {
    sendErr(401, "需要 X-Device-Key 请求头（设备串口 CLI 输入 key 查看）");
    return;
  }
  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendErr(400, "JSON 解析失败");
    return;
  }
  const String ssid = doc["ssid"] | "";
  const String pass = doc["pass"] | "";
  if (!ssid.length() || ssid.length() > 32 || pass.length() < 8 || pass.length() > 64) {
    sendErr(400, "ssid 必填（≤32）且 pass 8-64 位");
    return;
  }
  wifi::connect(ssid.c_str(), pass.c_str());
  sendOk("凭据已保存并开始连接");
}

void hPipe() {
  // 入队即应答（拉取在 worker 任务执行，结果见设备日志/缓存/屏）
  if (!worker::requestPipe()) {
    sendErr(409, "工作队列忙碌，稍后再试");
    return;
  }
  sendOk("已受理：流水执行中（结果看设备日志）");
}

void hOta() {
  if (!keyOk()) {
    sendErr(401, "需要 X-Device-Key 请求头（设备串口 CLI 输入 key 查看）");
    return;
  }
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
  const String md5 = doc["md5"] | "";  // 可选：Release 附件 .md5（强烈建议）
  if (!worker::requestOta(url.c_str(), md5.isEmpty() ? nullptr : md5.c_str())) {
    sendErr(409, "工作队列忙碌，稍后再试");
    return;
  }
  sendOk("升级启动：下载写入后设备将重启");
}

void hReboot() {
  if (!keyOk()) {
    sendErr(401, "需要 X-Device-Key 请求头（设备串口 CLI 输入 key 查看）");
    return;
  }
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
  if (!keyOk()) {
    sendErr(401, "需要 X-Device-Key 请求头（设备串口 CLI 输入 key 查看）");
    return;
  }
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
  if (!keyOk()) {
    sendErr(401, "需要 X-Device-Key 请求头（设备串口 CLI 输入 key 查看）");
    return;
  }
  if (!SCREEN_ATTACHED) {
    sendErr(503, "SCREEN_ATTACHED=false，屏未启用");
    return;
  }
  if (!canvas::get().begin()) {
    sendErr(503, "PSRAM 三平面分配失败");
    return;
  }
  if (worker::busy()) {
    sendErr(409, "屏刷新进行中，稍后再推（防止推屏任务与位图写入竞争平面）");
    return;
  }
  for (uint8_t p = 0; p < 3; p++)  // 协议 §7：省略的平面 = 全白
    if (!(sPlaneSeen & (1 << p))) memset(canvas::get().plane(p), 0, canvas::PLANE_BYTES);
  sPlaneSeen = 0;
  // 先应答后执行：22s 阻塞期间 TCP 会被断（errno 113 实测），客户端拿到
  // 的是连接中断而非结果——应答改为"已受理"，刷新结果看设备日志/屏
  // 入队即应答：worker 任务执行 flush（loopTask 不再阻塞，HTTP/CLI 全程在线）
  if (!worker::requestRender(worker::Render::Bitmap)) {
    sendErr(409, "工作队列忙碌，稍后再试");
    return;
  }
  sendOk("已受理：约 22s 完成上屏（结果看屏幕；期间可正常交互）");
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
  // 收集的请求头：Content-Type（form 防御）/ Origin 与 X-Device-Key（CORS 与配对码）
  const char* collect[] = {"Content-Type", "Origin", "X-Device-Key"};
  server.collectHeaders(collect, 3);
}

}  // namespace

void begin() {
  registerRoutes();
  server.begin();
  Serial.println("[Web] HTTP API 已启动（:80，接口见 webapi.h 注释）");
}

void poll() {
  // mDNS：联网后注册；IP 变化时重注册（旧注册会指向失效 IP，V7）。
  // 后缀取 eFuse MAC 低 24 位（6 hex）——16 位空间在 40 台下生日碰撞
  // ~1.2%，24 位降至 ~0.002%
  if (wifi::state() == wifi::State::Connected &&
      (!sMdnsStarted || sMdnsIp != WiFi.localIP())) {
    char host[24];
    snprintf(host, sizeof(host), "whaledock-%06llX",
             (unsigned long long)(ESP.getEfuseMac() & 0xFFFFFF));
    if (MDNS.begin(host)) {
      MDNS.addService("http", "tcp", 80);
      sMdnsIp = WiFi.localIP();
      sMdnsStarted = true;
      Serial.printf("[Web] mDNS 已注册：http://%s.local\n", host);
    }
  }
  server.handleClient();
}

}  // namespace webapi
