#pragma once
// ============================================================
// 应用层 · 板级自检与状态上报
// print()：一次性自检（芯片 / Flash 16MB / PSRAM 8MB，配置与硬件双重确认）
// heartbeat()：运行期心跳，内部按 HEARTBEAT_PERIOD_MS 节流
// ============================================================

namespace selfcheck {

void print();
void heartbeat();

}  // namespace selfcheck
