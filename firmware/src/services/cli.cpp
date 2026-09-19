#include "cli.h"

#include <Arduino.h>
#include <WiFi.h>

#include "wifi.h"

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
