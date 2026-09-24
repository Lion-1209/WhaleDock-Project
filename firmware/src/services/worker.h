#pragma once

#include <stdint.h>
// ============================================================
// 服务层 · 重活工作队列（审计 V5 / 开发约定 §9 任务拆分）
// 渲染上屏（~22s）、数据流水（~4s）、OTA 下载（~10-60s）统一在独立
// 任务串行执行；loopTask 只入队立即返回——CLI/HTTP/呼吸灯全程在线。
// 互斥由单工作队列天然保证；忙碌时新请求按类型拒绝（调用方提示）。
// ============================================================

namespace worker {

void begin();  // 创建队列与任务（setup 末尾、首次入队前调用）

bool busy();   // 有任务执行中或排队

// 渲染上屏的四种来源：
//   Layout  = 渲染 /layout.json（协议布局，C2 接入；无文件时回退 Demo）
//   Demo    = 内置示例帧（概念图固化资产，C2 后退役）
//   Test    = 四色诊断图（棋盘格）
//   Bitmap  = 三平面现状直推（B3 位图通道：平面已由 HTTP 写好，只 flush）
enum class Render : uint8_t { Layout, Demo, Test, Bitmap };

bool requestRender(Render what);               // 忙碌时 false
void requestRenderFromWorker(Render what);     // worker 上下文内部用（如流水完成后追屏）
bool requestPipe();                            // 一轮数据流水
bool requestOta(const char* url, const char* md5);  // 忙碌时 false

}  // namespace worker
