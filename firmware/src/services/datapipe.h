#pragma once
// ============================================================
// 服务层 · 数据流水线（M2 · A4）
// 整点动作：读设备配置（storage）→ 按 GitHub 数据源拉取（github）
// → 成功项自动落盘缓存（A3 已内建）。断网/失败时本轮跳过或报错，
// 渲染侧（B2/B3 起）用 loadCached* 兜底——"开箱即有完整体验"。
//
// 触发：ntp 整点回调（begin() 注册）+ 手动（CLI `pipe` / 网页按钮）。
// 已知偏差（有意）：HTTP 同步阻塞跑在 loopTask，拉取期间呼吸暂停数秒，
// RTOS 任务拆分（开发约定 §9）时归还。
// ============================================================

namespace datapipe {

void begin();  // 注册 ntp 整点回调（setup 调用一次）
void runOnce(const char* trigger);  // 执行一轮流水（trigger 仅用于日志：整点/手动）
bool busy();  // 拉取进行中（状态指示用）

}  // namespace datapipe
