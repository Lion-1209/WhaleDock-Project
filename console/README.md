# console/ · Web 上位机（起步）

零后端、零构建：纯静态文件，浏览器经 **Web Serial** 直连设备串口，协议为固件的串口 CLI（`firmware/src/services/cli`）。当前功能：连接设备、Wi-Fi 表单配网、状态徽标（由固件 `[WiFi]` 日志行驱动）、日志面板与自由命令输入。

## 本地运行

Web Serial 要求安全上下文（HTTPS 或 localhost），**直接双击 index.html 打开无效**：

```bash
cd console
python -m http.server 8765
# 浏览器（Chrome / Edge / Opera）打开 http://localhost:8765
```

## 已知边界

- 仅 Chromium 内核浏览器支持 Web Serial（Firefox / Safari 不行）；
- 设备须插 DevKitC-1 的 **"USB"** 口（原生 USB CDC，VID 0x303A），页面已按此过滤串口列表；
- 连接前关闭其它占用 COM 口的程序（VS Code 串口监视、其它监视器）；
- SSID/密码含英文逗号无法下发（CLI 协议限制），页面已拦截并提示。

## 路线（对齐设计文档）

GitHub Pages 托管（免本地起服）→ 位图推送（M2 验收项）→ 布局编辑器 Canvas 1:1 → mDNS 局域网批量。设备侧 HTTP API + mDNS 为第二通道（非 Chromium 兜底）。
