#pragma once
// ============================================================
// 服务层 · 设备 HTTP API + mDNS（D2）
// 局域网通道（设计文档：Web Serial 之外的设备自建接口）：
// 浏览器/上位机经 Wi-Fi 直连设备，串口从此只是备用调试通道。
//
// 接口（JSON，CORS 全开，供网页直连）：
//   GET  /api/status        版本/Wi-Fi/时间/内存/分区一览
//   GET  /api/config        当前配置（token 仅报状态，不回显）
//   POST /api/config        更新配置（JSON 字段可选，空=不改，"-"=清除）
//   POST /api/wifi          {ssid,pass} 配网（同 CLI wifi set）
//   POST /api/pipe          立即执行一轮数据流水（阻塞至完成）
//   POST /api/ota           {url} 启动 OTA（先应答后执行，成功即重启）
//   POST /api/reboot        重启设备
//   POST /api/display/bw|red|yellow?off=<字节偏移>  B3 位图通道：body=该块
//        裸数据的 base64（≤12000 字节/块，48000B 平面分 4 块；分块规避
//        WebServer plain 参数对内嵌 NUL 的截断与大 body 内存峰值）
//   POST /api/display/flush 位图上屏（未推平面=全白）。先应答"已受理"后
//        执行：约 22s 完成上屏（长阻塞会杀死 TCP 故先应答；期间射频静默
//        + BUSY 保险丝保护刷新波形）
//
// 安全边界（v1 如实）：仅监听 STA（不对外网），无鉴权——限可信局域网；
// 量产前评估加配对码。长操作（pipe/ota）在 HTTP 线程内阻塞执行，属
// 已知偏差（§9 任务拆分时归还）。
// ============================================================

namespace webapi {

void begin();  // 注册路由并启动服务（setup 调用；mDNS 待联网后在 poll 里注册）
void poll();   // loop 调用：联网后注册 mDNS + 处理 HTTP 请求

}  // namespace webapi
