#pragma once
// ============================================================
// App 层 · 显示协议 v1 布局 JSON 校验（docs/显示协议-v1.md §9）
// 屏未到货，先落地结构校验：CLI layout sample|check 可完整验证
// （LittleFS 持久化 + JSON 解析 + 规则校验）。
// C2 Widget 引擎在此之上实现各类型 draw()；冻结后协议改动走版本号。
// ============================================================

#include <ArduinoJson.h>
#include <String>

namespace layout {

struct CheckResult {
  bool ok = false;
  bool bitmapMode = false;   // true = 位图通道 display，false = Widget 布局
  size_t widgetCount = 0;
  size_t sourceCount = 0;
  String errors;             // '\n' 分隔，每条带规则编号
  String summary;            // 人读摘要：各槽位类型/颜色
};

// 校验一段布局 JSON（协议 §9 规则 1-11 中可静态判定者）
CheckResult check(const String& json);

// 内置示例布局（与协议 §11 示例一致，用于 layout sample 与联调）
const char* sampleJson();

}  // namespace layout
