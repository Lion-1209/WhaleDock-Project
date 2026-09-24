#include "cli.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "app/assets/concept_demo.h"
#include "app/epaper_selftest.h"
#include "app/widgets.h"
#include "app/layout.h"
#include "config.h"
#include "datapipe.h"
#include "github.h"
#include "ntp.h"
#include "ota.h"
#include "webapi.h"
#include "drivers/epaper.h"
#include "pins.h"
#include "storage.h"
#include "worker.h"
#include "wifi.h"

#include <SPI.h>

namespace cli {

namespace {

constexpr size_t kLineMax = 128;
char sBuf[kLineMax];
size_t sLen = 0;

void printHelp() {
  Serial.println("命令：");
  Serial.println("  help                     本帮助");
  Serial.println("  wifi set <ssid>,<pass>   保存凭据并连接（pass >= 8，中文 SSID 可用）");
  Serial.println("  wifi status              连接状态");
  Serial.println("  wifi scan                扫描周边 AP（核对 SSID 可见性/编码）");
  Serial.println("  wifi clear               清除凭据");
  Serial.println("  time                     当前时间与 NTP 同步状态");
  Serial.println("  gh user <login>          GitHub 用户（仓库数/关注者）");
  Serial.println("  gh repo <owner>/<repo>   GitHub 仓库（Stars/Forks）");
  Serial.println("  gh commits <owner>/<repo>  近 12 周提交序列");
  Serial.println("  config show|user <l>|repo <o/r>|token <t>  数据源配置（- 清除对应项）");
  Serial.println("  fs ls [dir]|cat <f>|rm <f>|format   文件系统调试");
  Serial.println("  layout sample|check     写入/校验示例显示布局（协议 v1，见 docs/显示协议-v1.md）");
  Serial.println("  demo                     渲染 /layout.json 上屏（C2 引擎；无文件回退示例帧）");
  Serial.println("  screen test              四色诊断图上屏（B2 管线验证图，全刷约 22s）");
  Serial.println("  pipe                     立即执行一轮数据流水（同整点动作）");
  Serial.println("  ota status               固件版本/分区状态");
  Serial.println("  ota <url> [md5]          下载 .bin 升级（建议带 Release 的 md5 校验）");
  Serial.println("  ota confirm|rollback     确认新固件 / 回滚旧版本");
  Serial.println("  probe                    位读屏控制器 REV+PON 轨迹（硬件排障）");
  Serial.println("  key                      显示设备配对码（局域网写操作 API 需要）");
  Serial.println("  ip                       网络一览：IP + mDNS 域名（WebUI USB 探测用）");
  Serial.println("  reboot                   重启（验证凭据持久化）");
}

// 非 ASCII SSID（如中文）附 hex：正常应为 UTF-8；
// 若显示为 GBK 字节序列，说明老路由器按 GBK 广播，需改路由器名
void printSsid(const String& ssid) {
  Serial.print(ssid);
  bool ascii = true;
  for (size_t i = 0; i < ssid.length(); ++i)
    if ((uint8_t)ssid[i] >= 0x80) ascii = false;
  if (!ascii) {
    Serial.print("  (hex:");
    for (size_t i = 0; i < ssid.length(); ++i)
      Serial.printf("%02X", (uint8_t)ssid[i]);
    Serial.print(")");
  }
}

void cmdWifiScan() {
  Serial.println("[WiFi] 扫描中（约 2-5s，期间阻塞心跳）…");
  const int n = WiFi.scanNetworks();  // 同步扫描：调试命令，可接受短暂阻塞
  if (n <= 0) {
    Serial.println("[WiFi] 未扫到任何 AP");
    return;
  }
  Serial.printf("[WiFi] 共 %d 个 AP：\n", n);
  for (int i = 0; i < n; ++i) {
    Serial.printf("  %2d  ch%-2u  %4d dBm  ", WiFi.channel(i), WiFi.RSSI(i));
    printSsid(WiFi.SSID(i));
    Serial.println();
  }
  WiFi.scanDelete();
}

void cmdWifi(char* rest) {
  if (!strncmp(rest, "set ", 4)) {
    char* args = rest + 4;
    char* comma = strchr(args, ',');
    if (!comma || comma == args) {
      Serial.println("[CLI] 格式：wifi set <ssid>,<pass>");
      return;
    }
    *comma = '\0';
    const char* pass = comma + 1;
    if (strlen(pass) < 8)
      Serial.println("[CLI] 提示：WPA 密码应 >=8 字符，仍将尝试连接");
    wifi::connect(args, pass);
  } else if (!strcmp(rest, "status")) {
    static const char* kStateName[] = {"未配网", "连接中", "已连接", "等待重连"};
    Serial.printf("[WiFi] %s  SSID=", kStateName[static_cast<int>(wifi::state())]);
    printSsid(String(wifi::ssid()));
    Serial.printf("  IP=%s  RSSI=%d dBm\n", wifi::ip(),
                  wifi::state() == wifi::State::Connected ? WiFi.RSSI() : 0);
  } else if (!strcmp(rest, "scan")) {
    cmdWifiScan();
  } else if (!strcmp(rest, "clear")) {
    wifi::forget();
  } else {
    Serial.println("[CLI] 未知子命令，见 help");
  }
}

// gh user <login> | gh repo <o>/<r> | gh commits <o>/<r>
// 拉取为同步阻塞（超时 10s），命令期间心跳/呼吸暂停属预期
void cmdGh(char* rest) {
  if (wifi::state() != wifi::State::Connected) {
    Serial.println("[GitHub] Wi-Fi 未连接（或连接中），稍后再试");
    return;
  }
  const uint32_t t0 = millis();
  if (!strncmp(rest, "user ", 5)) {
    const github::UserStats u = github::fetchUser(rest + 5);
    if (u.ok)
      Serial.printf("[GitHub] 用户 %s：公开仓库 %d · 关注者 %d（耗时 %.1fs）\n",
                    u.login.c_str(), u.publicRepos, u.followers,
                    (millis() - t0) / 1000.0);
    else
      Serial.printf("[GitHub] 失败：%s（耗时 %.1fs）\n", u.error.c_str(),
                    (millis() - t0) / 1000.0);
  } else if (!strncmp(rest, "repo ", 5) || !strncmp(rest, "commits ", 8)) {
    const bool commits = rest[0] == 'c';
    char* slug = rest + (commits ? 8 : 5);
    char* slash = strchr(slug, '/');
    if (!slash || slash == slug || !slash[1]) {
      Serial.println("[CLI] 格式：gh repo <owner>/<repo>");
      return;
    }
    *slash = '\0';
    const char* repo = slash + 1;
    if (commits) {
      const github::CommitActivity c = github::fetchCommitActivity(slug, repo);
      if (!c.ok) {
        Serial.printf("[GitHub] 失败：%s（耗时 %.1fs）\n", c.error.c_str(),
                      (millis() - t0) / 1000.0);
        return;
      }
      Serial.printf("[GitHub] %s/%s 近 %d 周提交（旧→新，耗时 %.1fs）：\n  ", slug,
                    repo, (int)c.weeklyTotals.size(), (millis() - t0) / 1000.0);
      for (const int w : c.weeklyTotals) Serial.printf("%d ", w);
      Serial.printf("\n  合计 %d 次\n", c.total);
    } else {
      const github::RepoStats s = github::fetchRepo(slug, repo);
      if (s.ok)
        Serial.printf("[GitHub] 仓库 %s：Stars %d · Forks %d（耗时 %.1fs）\n",
                      s.fullName.c_str(), s.stars, s.forks,
                      (millis() - t0) / 1000.0);
      else
        Serial.printf("[GitHub] 失败：%s（耗时 %.1fs）\n", s.error.c_str(),
                      (millis() - t0) / 1000.0);
    }
  } else {
    Serial.println("[CLI] 未知子命令，见 help");
  }
}

// fs ls [dir] | fs cat <path> | fs rm <path> | fs format
void cmdFs(char* rest) {
  char* sp = strchr(rest, ' ');
  char* arg = const_cast<char*>("");
  if (sp) {
    *sp = '\0';
    arg = sp + 1;
  }
  // V6：路径穿越防护（cat/rm 拒绝 .. 与空白名单路径；物理接触面有限防护）
  const auto pathOk = [](const char* p) {
    return strstr(p, "..") == nullptr;
  };
  if (!strcmp(rest, "ls")) {
    const char* dir = *arg ? arg : "/";
    const auto names = storage::listDir(dir);
    Serial.printf("[FS] %s：%zu 项\n", dir, names.size());
    for (const String& n : names) Serial.printf("  %s\n", n.c_str());
  } else if (!strcmp(rest, "cat")) {
    if (!*arg) {
      Serial.println("[CLI] 格式：fs cat <路径>");
      return;
    }
    if (!pathOk(arg)) {
      Serial.println("[FS] 路径非法（拒绝 ..）");
      return;
    }
    if (!storage::exists(arg)) {
      Serial.printf("[FS] %s 不存在\n", arg);
      return;
    }
    const String c = storage::readFile(arg);
    Serial.printf("[FS] %s（%u B）：\n%s\n", arg, c.length(), c.c_str());
  } else if (!strcmp(rest, "rm")) {
    if (!*arg) {
      Serial.println("[CLI] 格式：fs rm <路径>");
      return;
    }
    if (!pathOk(arg)) {
      Serial.println("[FS] 路径非法（拒绝 ..）");
      return;
    }
    Serial.println(storage::removeFile(arg) ? "[FS] 已删除" : "[FS] 删除失败（不存在？）");
  } else if (!strcmp(rest, "format")) {
    Serial.println(storage::format() ? "[FS] 已格式化（配置与缓存全部清空）"
                                     : "[FS] 格式化失败");
  } else {
    Serial.println("[CLI] 未知子命令，见 help");
  }
}

// config show | config user <l> | config repo <o/r>|- | config token <t>|-
void cmdConfig(char* rest) {
  storage::Config c;
  storage::loadConfig(c);  // 无文件时自动带出厂默认
  char* sp = strchr(rest, ' ');
  char* arg = const_cast<char*>("");
  if (sp) {
    *sp = '\0';
    arg = sp + 1;
  }
  if (!strcmp(rest, "show") || !*rest) {
    // token 只报状态不回显（串口/网页日志半公开环境）
    Serial.printf("[配置] githubUser=%s · githubRepo=%s · token=%s\n",
                  c.githubUser.length() ? c.githubUser.c_str() : "（未配置）",
                  c.githubRepo.length() ? c.githubRepo.c_str() : "（未配置）",
                  c.githubToken.length() ? "已配置" : "未配置");
  } else if (!strcmp(rest, "user") && *arg) {
    if (strcmp(arg, "-") && (strlen(arg) > 64 || strpbrk(arg, " /?&#@"))) {
      Serial.println("[配置] githubUser 非法（≤64，限登录名字符）");
      return;
    }
    c.githubUser = arg;
    Serial.println(storage::saveConfig(c) ? "[配置] 已保存" : "[配置] 保存失败");
  } else if (!strcmp(rest, "repo") && *arg) {
    if (strcmp(arg, "-") && (strlen(arg) > 64 || strpbrk(arg, " ?&#@"))) {
      Serial.println("[配置] githubRepo 非法（≤64，限 o/r 字符）");
      return;
    }
    c.githubRepo = !strcmp(arg, "-") ? "" : String(arg);
    Serial.println(storage::saveConfig(c) ? "[配置] 已保存" : "[配置] 保存失败");
  } else if (!strcmp(rest, "token") && *arg) {
    if (strcmp(arg, "-") && strlen(arg) > 255) {
      Serial.println("[配置] githubToken 非法（≤255）");
      return;
    }
    c.githubToken = !strcmp(arg, "-") ? "" : String(arg);
    Serial.println(storage::saveConfig(c) ? "[配置] 已保存" : "[配置] 保存失败");
  } else {
    Serial.println("[CLI] 格式：config show | user <l> | repo <o/r>|- | token <t>|-");
  }
}

// layout sample | layout check —— 显示协议 v1 布局校验（app/layout）
void cmdLayout(char* rest) {
  if (!strcmp(rest, "sample")) {
    if (storage::writeFile("/layout.json", layout::sampleJson()))
      Serial.println("[布局] 示例已写入 /layout.json（协议 v1 §11）");
    else
      Serial.println("[布局] 写入失败");
  } else if (!strcmp(rest, "check")) {
    if (!storage::exists("/layout.json")) {
      Serial.println("[布局] /layout.json 不存在，先 layout sample");
      return;
    }
    const layout::CheckResult r = layout::check(storage::readFile("/layout.json"));
    Serial.printf("[布局] 校验%s：%zu widgets · %zu 数据源 · %s 模式\n",
                  r.ok ? "通过" : "未通过", r.widgetCount, r.sourceCount,
                  r.bitmapMode ? "位图" : "Widget");
    if (r.summary.length()) Serial.print(r.summary.c_str());
    if (!r.ok) Serial.print(r.errors.c_str());
  } else {
    Serial.println("[CLI] 格式：layout sample | check");
  }
}

// B3 排障工具集：位读 REV / 对照组 / PON·DRF 轨迹 / SPI 引脚出波自检
namespace {
void bbPinsOut() {
  SPI.end();
  pinMode(EPD_CS, OUTPUT);
  pinMode(EPD_DC, OUTPUT);
  pinMode(EPD_CLK, OUTPUT);
  pinMode(EPD_DIN, OUTPUT);
  pinMode(EPD_RST, OUTPUT);
  pinMode(EPD_BUSY, INPUT);
  digitalWrite(EPD_CLK, LOW);
}
void bbCmd(uint8_t c, bool dcHigh) {  // 位发一字节（CS 由调用者管理）
  digitalWrite(EPD_DC, dcHigh ? HIGH : LOW);
  pinMode(EPD_DIN, OUTPUT);
  for (int i = 7; i >= 0; i--) {
    digitalWrite(EPD_DIN, (c >> i) & 1);
    delayMicroseconds(3);
    digitalWrite(EPD_CLK, HIGH);
    delayMicroseconds(3);
    digitalWrite(EPD_CLK, LOW);
    delayMicroseconds(3);
  }
}
void bbRead3(bool csLow) {  // 位读 REV(0x70) 3 字节；csLow=false 为对照组（CS 悬高）
  digitalWrite(EPD_CS, csLow ? LOW : HIGH);
  delayMicroseconds(4);
  if (csLow) bbCmd(0x70, false);
  digitalWrite(EPD_DC, HIGH);
  pinMode(EPD_DIN, INPUT);
  delayMicroseconds(4);
  uint8_t out[3] = {0, 0, 0};
  for (int b = 0; b < 3; b++)
    for (int i = 7; i >= 0; i--) {
      digitalWrite(EPD_CLK, HIGH);
      delayMicroseconds(3);
      digitalWrite(EPD_CLK, LOW);
      delayMicroseconds(3);
      out[b] = (out[b] << 1) | (digitalRead(EPD_DIN) ? 1 : 0);
    }
  digitalWrite(EPD_CS, HIGH);
  Serial.printf("[probe] REV(CS=%s)=%02X %02X %02X\n", csLow ? "L" : "H", out[0], out[1], out[2]);
}
int bbBusyTrace(int ms) {  // BUSY 轨迹，返回低电平毫秒数
  int lowMs = 0;
  String tr;
  for (int i = 0; i < ms; i++) {
    const bool busy = !digitalRead(EPD_BUSY);
    if (busy) lowMs++;
    if (i < 400) {
      tr += busy ? 'L' : 'H';
      if (i % 50 == 49) tr += '|';
    }
    delay(1);
  }
  Serial.printf("[probe] BUSY 轨迹(前400ms)：%s\n", tr.c_str());
  return lowMs;
}
}  // namespace

void cmdProbe() {
  Serial.println("[probe] RST 复位后位读 REV(0x70) …");
  bbPinsOut();
  // ① 硬复位面板（深睡/异常态唯一出口），等它完成内部初始化
  digitalWrite(EPD_RST, HIGH);
  delay(10);
  digitalWrite(EPD_RST, LOW);
  delay(100);
  digitalWrite(EPD_RST, HIGH);
  delay(100);
  // ② 真读（CS 拉低）vs ③ 对照组（CS 悬高，应答应全 0/1 = 悬空耦合噪声）
  bbRead3(true);
  bbRead3(false);
  // ④ PON + DRF 位发，看升压/刷新是否真执行（BUSY 低=忙；正常 PON≈119ms、DRF≈21.8s）
  Serial.println("[probe] 位发 PON(0x04)…");
  digitalWrite(EPD_CS, LOW);
  bbCmd(0x04, false);
  digitalWrite(EPD_CS, HIGH);
  int ponLow = bbBusyTrace(400);
  Serial.printf("[probe] PON 忙 %dms（正常 ~119ms；0=未执行，7=启动即溃）\n", ponLow);
  Serial.println("[probe] 位发 DRF(0x12)，观察 6s …");
  digitalWrite(EPD_CS, LOW);
  bbCmd(0x12, false);
  digitalWrite(EPD_CS, HIGH);
  int drfLow = bbBusyTrace(6000);
  Serial.printf("[probe] DRF 忙 %dms/6000ms（正常 21800ms——只验证是否启动）\n", drfLow);
  SPI.begin(EPD_CLK, -1, EPD_DIN, -1);
}

// 渲染布局到三平面（不刷屏）并输出 ASCII 预览：调试 Widget 排版的"眼睛"
void cmdPreview(char* rest) {
  const String json = storage::exists("/layout.json")
                          ? storage::readFile("/layout.json") : String("");
  if (!json.length()) {
    Serial.println("[预览] 无 /layout.json");
    return;
  }
  {
    JsonDocument probe;
    const DeserializationError e = deserializeJson(probe, json);
    Serial.printf("[预览] JSON %uB 解析：%s（首 60 字：%.*s）\n",
                  (unsigned)json.length(), e ? e.c_str() : "OK", 60, json.c_str());
    if (e) return;
  }
  if (!widgets::render(json)) {
    Serial.println("[预览] 渲染失败");
    return;
  }
  auto& c = canvas::get();
  Serial.println("[预览] ASCII（#黑 R红 Y黄 .白）");
  const int step = *rest ? atoi(rest) : 8;
  for (int y = 0; y < 480; y += step) {
    String row;
    for (int x = 0; x < 800; x += step) {
      const int bit = 0x80 >> (x & 7);
      const int idx = y * 100 + (x >> 3);
      if (c.plane(canvas::PL_RED)[idx] & bit) row += 'R';
      else if (c.plane(canvas::PL_YELLOW)[idx] & bit) row += 'Y';
      else if (c.plane(canvas::PL_BW)[idx] & bit) row += '#';
      else row += '.';
    }
    Serial.println(row);
  }
}

void dispatch(char* line) {
  if (!*line) return;
  char* sp = strchr(line, ' ');
  char* rest = const_cast<char*>("");
  if (sp) {
    *sp = '\0';
    rest = sp + 1;
  }
  if (!strcmp(line, "help")) {
    printHelp();
  } else if (!strcmp(line, "wifi")) {
    cmdWifi(rest);
  } else if (!strcmp(line, "gh")) {
    cmdGh(rest);
  } else if (!strcmp(line, "config")) {
    cmdConfig(rest);
  } else if (!strcmp(line, "fs")) {
    cmdFs(rest);
  } else if (!strcmp(line, "layout")) {
    cmdLayout(rest);
  } else if (!strcmp(line, "demo")) {
    if (SCREEN_ATTACHED) {
      if (!worker::requestRender(worker::Render::Layout))
        Serial.println("[CLI] 工作队列忙碌，稍后再试");
    } else {
      Serial.println("[CLI] SCREEN_ATTACHED=false，屏未启用");
    }
  } else if (!strcmp(line, "screen")) {
    if (!SCREEN_ATTACHED) {
      Serial.println("[CLI] SCREEN_ATTACHED=false，屏未启用");
    } else if (!strcmp(rest, "test")) {
      if (!worker::requestRender(worker::Render::Test))
        Serial.println("[CLI] 工作队列忙碌，稍后再试");
    } else {
      Serial.println("[CLI] 格式：screen test");
    }
  } else if (!strcmp(line, "probe")) {
    cmdProbe();
  } else if (!strcmp(line, "preview")) {
    cmdPreview(rest);
  } else if (!strcmp(line, "key")) {
    // 设备配对码：同网段调用写操作 API（wifi/ota/reboot/display/config）须带
    // X-Device-Key 头；码由 eFuse MAC 派生，量产时印机身标签
    char key[8];
    snprintf(key, sizeof(key), "%06u", (unsigned)(ESP.getEfuseMac() % 1000000));
    Serial.printf("[配对码] %s（HTTP 写操作须带请求头 X-Device-Key: %s）\n", key, key);
  } else if (!strcmp(line, "ip")) {
    // 网络一览（单行 key=value，WebUI「USB 探测」解析此行自动填地址；
    // mDNS 名由 eFuse MAC 派生，换芯片自动换名，量产每台唯一）
    Serial.printf("[IP] state=%s ip=%s mdns=%s.local\n",
                  wifi::state() == wifi::State::Connected ? "online" : "offline",
                  wifi::ip(), webapi::mdnsHost());
  } else if (!strcmp(line, "pipe")) {
    if (!worker::requestPipe()) Serial.println("[CLI] 工作队列忙碌，稍后再试");
  } else if (!strcmp(line, "ota")) {
    if (!strcmp(rest, "status") || !*rest) {
      ota::printStatus();
    } else if (!strcmp(rest, "confirm")) {
      ota::confirm("手动");
    } else if (!strcmp(rest, "rollback")) {
      ota::rollbackNow();
    } else if (strncmp(rest, "http", 4) == 0) {
      // ota <url> [md5]：md5 来自 Release 附件 .md5，强烈建议携带（完整性校验）
      char* sp = strchr(rest, ' ');
      const char* md5 = nullptr;
      if (sp) {
        *sp = '\0';
        md5 = sp + 1;
      }
      ota::fromUrl(rest, md5);
    } else {
      Serial.println("[CLI] 格式：ota status | <url> [md5] | confirm | rollback");
    }
  } else if (!strcmp(line, "time")) {
    Serial.printf("[时间] %s（UTC+8）\n", ntp::timeString());
  } else if (!strcmp(line, "reboot")) {
    Serial.println("[CLI] 重启...");
    delay(100);  // 等 CDC 把这行送出去
    ESP.restart();
  } else {
    Serial.printf("[CLI] 未知命令：%s（help 查看命令）\n", line);
  }
}

}  // namespace

void begin() {
  Serial.println("[CLI] 串口命令行就绪，输入 help 查看");
}

void poll() {
  while (Serial.available() > 0) {
    // 必须用无符号收字节：char 在本平台是 signed，UTF-8 中文等
    // 0x80+ 字节会变负数，被下面的控制字符过滤误杀（中文 SSID 就这么丢的）
    const uint8_t c = (uint8_t)Serial.read();
    // \r / \n 都作为结束符：兼容监视器回车的 CR / LF / CRLF 三种发送模式
    if (c == '\r' || c == '\n') {
      if (sLen > 0) {
        sBuf[sLen] = '\0';
        Serial.println();  // 回显换行，再输出命令响应
        dispatch(sBuf);
        sLen = 0;
      }
      continue;
    }
    if (c == '\b' || c == 127) {  // 退格：改密码打错时可用
      if (sLen > 0) {
        sLen--;
        Serial.print("\b \b");
      }
      continue;
    }
    if (c < 32) continue;  // 其余控制字符忽略（0x80+ 的多字节字符正常通过）
    if (sLen < kLineMax - 1) {
      sBuf[sLen++] = (char)c;
      Serial.write(c);  // 回显（监视器本身不回显，密码输入会明文可见，调机可接受）
    } else {
      sLen = 0;
      Serial.println();
      Serial.println("[CLI] 行超长，已丢弃");
    }
  }
}

}  // namespace cli
