#include "webapi.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_ota_ops.h>

#include "config.h"
#include "datapipe.h"
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

void registerRoutes() {
  server.on("/api/status", HTTP_GET, hStatus);
  server.on("/api/config", HTTP_GET, hConfigGet);
  server.on("/api/config", HTTP_POST, hConfigPost);
  server.on("/api/wifi", HTTP_POST, hWifiPost);
  server.on("/api/pipe", HTTP_POST, hPipe);
  server.on("/api/ota", HTTP_POST, hOta);
  server.on("/api/reboot", HTTP_POST, hReboot);
  // 各路径的 CORS 预检
  const char* paths[] = {"/api/status", "/api/config", "/api/wifi",
                         "/api/pipe", "/api/ota", "/api/reboot"};
  for (const char* p : paths) server.on(p, HTTP_OPTIONS, handleOptions);
  server.onNotFound([]() { sendErr(404, "not found（API 见 /api/*）"); });
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
