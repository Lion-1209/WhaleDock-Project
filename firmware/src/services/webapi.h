#pragma once
// ============================================================
// 服务层 · 设备 HTTP API + mDNS（D2）
// 局域网通道（Web Serial 之外的设备自建接口）：
// 浏览器/上位机经 Wi-Fi 直连设备，串口从此只是备用调试通道。
//
// **鉴权（v0.8.1 安全批次）**：写操作端点（config/wifi/ota/reboot/
// display/*）须带请求头 `X-Device-Key: <6 位配对码>`；配对码由设备
// eFuse MAC 派生，串口 CLI `key` 查看，量产印机身标签。GET status
// 无 key 时 SSID/分区字段脱敏。
//
// 接口（JSON；CORS 仅放行 console 线上版与 localhost）：
//   GET  /api/status        版本/Wi-Fi/时间/内存一览（无 key 脱敏）
//   GET  /api/config        当前配置（token 仅报状态，不回显）
//   POST /api/config        更新配置（需 key；字段可选，空=不改，"-"=清除；
//        user ≤64 [A-Za-z0-9_.-]，repo ≤64 可含 /，token ≤255 可见字符）
//   POST /api/wifi          {ssid,pass} 配网（需 key；ssid≤32，pass 8-64）
//   POST /api/pipe          立即执行一轮数据流水（阻塞至完成）
//   POST /api/ota           {url,md5?} 启动 OTA（需 key；https 走根证书
//        校验；带 md5 时启用完整性校验，先应答后执行）
//   POST /api/reboot        重启设备（需 key）
//   POST /api/display/bw|red|yellow?off=<字节偏移>  B3 位图通道（需 key）：
//        body=该块裸数据的 base64（≤12000 字节/块，48000B 平面分 4 块；
//        分块规避 WebServer plain 参数对内嵌 NUL 的截断与大 body 内存峰值）
//   POST /api/display/flush 位图上屏（需 key；未推平面=全白）。先应答"已受理"
//        后执行：约 22s 完成上屏（长阻塞会杀死 TCP 故先应答；期间射频静默
//        + BUSY 保险丝保护刷新波形）
//
// 安全边界（v0.8.1 起）：写操作有配对码；GitHub/OTA 走根证书校验（certs.h）；
// token 存 NVS 不落文件系统。仍限可信局域网使用，签名级校验留量产批次。
// 长操作（pipe/ota/flush）在 HTTP 线程内阻塞执行，属已知偏差（§9 任务拆分
// 时归还，审计 V5）。
// ============================================================

namespace webapi {

void begin();  // 注册路由并启动服务（setup 调用；mDNS 待联网后在 poll 里注册）
void poll();   // loop 调用：联网后注册 mDNS + 处理 HTTP 请求

}  // namespace webapi
