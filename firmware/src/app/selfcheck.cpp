#include "selfcheck.h"

#include <Arduino.h>

#include "config.h"

namespace selfcheck {

void print() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("      鲸屿 WhaleDock · 板级自检");
  Serial.println("========================================");
  Serial.printf("芯片    : %s rev%d / %d 核 @ %u MHz\n",
                ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores(),
                ESP.getCpuFreqMHz());

  const size_t flashMB = ESP.getFlashChipSize() / (1024UL * 1024);
  const size_t psramMB = ESP.getPsramSize() / (1024UL * 1024);
  Serial.printf("Flash   : %u MB @ %u MHz  %s\n", flashMB,
                ESP.getFlashChipSpeed() / 1000000,
                flashMB >= 16 ? "[OK]" : "[!] 应为 16MB");
  Serial.printf("PSRAM   : %u MB  %s\n", psramMB,
                psramMB >= 8 ? "[OK]" : "[!] 应为 8MB，检查 memory_type=qio_opi");
  Serial.printf("堆内存  : 空闲 %u KB / 历史最低 %u KB\n",
                ESP.getFreeHeap() / 1024, ESP.getMinFreeHeap() / 1024);
  Serial.println("========================================");
}

void heartbeat() {
  static uint32_t last = 0;
  static uint32_t beat = 0;
  const uint32_t now = millis();
  if (now - last < HEARTBEAT_PERIOD_MS) return;
  last = now;
  Serial.printf("[心跳] %us  堆 %u KB / PSRAM 空闲 %u KB\n",
                (uint32_t)(++beat * HEARTBEAT_PERIOD_MS / 1000),
                ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);
}

}  // namespace selfcheck
