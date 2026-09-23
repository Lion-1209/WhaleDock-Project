#include "ota.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_ota_ops.h>

#include "certs.h"
#include "config.h"
#include "wifi.h"

namespace ota {

namespace {

bool sPendingVerify = false;

const char* stateName(esp_ota_img_states_t st) {
  switch (st) {
    case ESP_OTA_IMG_NEW: return "新写入";
    case ESP_OTA_IMG_PENDING_VERIFY: return "待验证";
    case ESP_OTA_IMG_VALID: return "已确认";
    case ESP_OTA_IMG_INVALID: return "无效";
    case ESP_OTA_IMG_ABORTED: return "已中止";
    default: return "未知";
  }
}

// 进度打印按 10% 粒度节流，避免刷爆串口/网页日志
void printProgress(size_t done, size_t total) {
  static int lastPct = -1;
  const int pct = total ? (int)(done * 100 / total) : 0;
  if (pct / 10 != lastPct / 10 || pct == 100) {
    lastPct = pct;
    Serial.printf("[OTA] 进度 %d%%（%u/%u KB）\n", pct, (unsigned)(done / 1024),
                  (unsigned)(total / 1024));
  }
}

}  // namespace

void begin() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_ota_img_states_t st = ESP_OTA_IMG_VALID;
  const bool known = esp_ota_get_state_partition(running, &st) == ESP_OK;
  Serial.printf("[OTA] 当前 v%s @%s 分区（%s），备用分区 %s\n", FW_VERSION,
                running->label, known ? stateName(st) : "状态未知",
                esp_ota_get_next_update_partition(nullptr)->label);
  if (known && st == ESP_OTA_IMG_PENDING_VERIFY) {
    sPendingVerify = true;
    Serial.printf("[OTA] 新固件待验证：健康运行 %us 后自动确认；"
                  "异常可立即 ota rollback 回滚\n",
                  (unsigned)(OTA_CONFIRM_MS / 1000));
  }
}

void poll() {
  if (sPendingVerify && millis() > OTA_CONFIRM_MS) confirm("健康运行超时，自动");
}

void confirm(const char* why) {
  if (!sPendingVerify) {
    Serial.println("[OTA] 无待验证固件（无需确认）");
    return;
  }
  if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
    sPendingVerify = false;
    Serial.printf("[OTA] 新固件已确认（%s），回滚点已清除\n", why);
  } else {
    Serial.println("[OTA] 确认失败（分区状态异常）");
  }
}

void rollbackNow() {
  Serial.println("[OTA] 标记当前固件无效，回滚到上一分区并重启…");
  delay(300);  // 等日志送出
  esp_ota_mark_app_invalid_rollback_and_reboot();
}

void fromUrl(const char* url, const char* md5) {
  if (wifi::state() != wifi::State::Connected) {
    Serial.println("[OTA] Wi-Fi 未连接，无法下载");
    return;
  }
  Serial.printf("[OTA] 开始下载：%s\n", url);

  // GitHub Release 直链必有 302 跳转（latest/download → objects 域），必须跟随
  WiFiClientSecure tls;
  WiFiClient plain;
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  const bool secure = strncasecmp(url, "http://", 7) != 0;  // 大小写不敏感（HTTP:// 误判会致 TLS 客户端连明文口）
  const bool begun = secure ? (tls.setCACert(kGithubRoots), http.begin(tls, url))
                            : http.begin(plain, url);
  if (!begun) {
    Serial.println("[OTA] URL 无效");
    return;
  }
  http.setTimeout(15000);
  http.addHeader("User-Agent", "WhaleDock-Firmware");
  const int16_t code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[OTA] 下载失败：HTTP %d\n", code);
    http.end();
    return;
  }
  const int total = http.getSize();
  if (total <= 0) {
    Serial.println("[OTA] 响应无 Content-Length，拒绝写入");
    http.end();
    return;
  }

  Serial.printf("[OTA] 固件 %u KB，写入备用分区（期间勿断电）…\n",
                (unsigned)(total / 1024));
  Update.onProgress(printProgress);
  if (!Update.begin(total)) {
    Serial.printf("[OTA] 无法开始写入：%s\n", Update.errorString());
    http.end();
    return;
  }
  // 完整性校验：提供 md5 则启用（防"长度正确的损坏/篡改镜像"）；
  // Update.end 内部完成 MD5 比对，不匹配自动拒绝
  if (md5 && *md5) {
    if (!Update.setMD5(md5)) {
      Serial.println("[OTA] md5 格式非法（应 32 位十六进制），拒绝写入");
      Update.abort();
      http.end();
      return;
    }
    Serial.println("[OTA] 已启用 MD5 完整性校验");
  } else {
    Serial.println("[OTA] ⚠ 未提供 md5，跳过完整性校验（建议 CLI/网页升级带上 Release 的 md5）");
  }
  const size_t written = Update.writeStream(http.getStream());
  http.end();
  if (written != (size_t)total) {
    Serial.printf("[OTA] 写入不完整（%u/%u B）：%s，已中止，当前固件未受影响\n",
                  (unsigned)written, (unsigned)total, Update.errorString());
    Update.abort();
    return;
  }
  if (!Update.end(true)) {
    Serial.printf("[OTA] 收尾失败：%s（内容不完整或 md5 不匹配），当前固件未受影响\n",
                  Update.errorString());
    return;
  }
  Serial.println("[OTA] 写入完成，完整性校验通过。2s 后重启进入新固件…");
  delay(2000);
  ESP.restart();
}

void printStatus() {
  begin();  // 状态打印与上电自检同源
}

}  // namespace ota
