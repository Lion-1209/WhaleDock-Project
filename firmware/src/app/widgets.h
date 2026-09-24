#pragma once
// ============================================================
// 应用层 · Widget 渲染引擎（C2）
// 消费已通过 layout::check 校验的布局 JSON：槽位几何（协议 §4/§12）
// → 按 type 分发注册的绘制器 → 落 canvas 三平面；数据取自 LittleFS
// 缓存（datapipe 落盘的最后一版），断网兜底"数据截至"口径。
// 与 console 模拟器（sim.js）同构：几何/颜色/字号规则一致，字体用
// GFX 经典 5×7（无中文/粗体，协议 fonts 子集留 D3 接入）。
// ============================================================

#include <ArduinoJson.h>

namespace widgets {

// 渲染布局（worker 任务上下文调用；canvas 需已 begin）。
// 返回 false = JSON 无法解析（调用方兜底示例帧）
bool render(const String& layoutJson);

}  // namespace widgets
