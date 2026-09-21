# console/ · Web 上位机

零后端、零构建：纯静态文件，双通道直连设备——浏览器经 **Web Serial** 走串口（协议为固件 CLI，`firmware/src/services/cli`），或经设备自建 **HTTP API + mDNS** 走局域网（`firmware/src/services/webapi`）。当前功能（8 卡工作台）：

- **设备**：连接管理（授权一次后页内直连）、设备时间、重启
- **Wi-Fi**：表单配网、凭据状态、周边 AP 扫描（点击回填 SSID）、清除凭据
- **数据源**：GitHub 用户/仓库/Token 配置（存设备 LittleFS，Token 不回显）、立即拉取
- **GitHub**：用户/仓库查询、近 12 周提交柱状图
- **OTA**：固件直链升级（默认 GitHub Release 最新版）、进度条、确认/回滚
- **存储**：LittleFS 文件浏览、内容查看、格式化
- **局域网**：mDNS 名或 IP 直连，状态轮询/流水/OTA/重启全网络操作（日志仍走串口）
- **日志**：实时串口日志（自动滚动）、自由命令输入
- **显示协议 v1 模拟器**：[`simulator.html`](./simulator.html) —— 粘贴布局 JSON，800×480 四色预览 + 与固件同源的协议校验（`docs/显示协议-v1.md`）；默认示例 = 概念图 v2 屏幕版式 1:1 复刻（鲸鱼为 datawhalelogo.png 转 1bpp 位图，协议 `image` 通道内联）

## 像素资源提取（tools/pixelize.html）

把概念图/位图素材转成协议 `image` 资源的零依赖静态工具：`python -m http.server` 起根目录服务后浏览器打开 `tools/pixelize.html?src=<图片路径>&grid=1`（grid = 叠加 100px 坐标网格）。支持框选裁剪、阈值二值化、红/黄通道分离、点采样降维，输出字符矩阵与 JS 数组；1bpp 位图打包（MSB-first）可直接作为 `resources[].data` 内联进布局 JSON。当前鲸鱼资源即由它从 `鲸鱼概念图素材` 生成。

## 线上版与本地版

- **线上版**：<https://lion-1209.github.io/WhaleDock-Project/console/>（GitHub Pages，随 main 自动部署；含 [模拟器](https://lion-1209.github.io/WhaleDock-Project/console/simulator.html)）
- **本地版**：Web Serial 要求安全上下文（HTTPS 或 localhost），**直接双击 index.html 打开无效**：

```bash
cd console
python -m http.server 8765
# 浏览器（Chrome / Edge / Opera）打开 http://localhost:8765
```

两版功能一致；改版后若样式未更新，强刷一次（Ctrl+F5）——静态资源引用已带 `?v=` 版本参数。

## 已知边界

- 仅 Chromium 内核浏览器支持 Web Serial（Firefox / Safari 不行）；
- **线上版（HTTPS）的局域网卡受混合内容限制**：浏览器禁止 HTTPS 页面向 `http://192.168.x.x` 发请求，网络通道在线上版不可用（串口通道不受影响）。需要网络通道时用本地版；
- 设备须插 DevKitC-1 的 **"USB"** 口（原生 USB CDC，VID 0x303A），页面已按此过滤串口列表；
- 连接前关闭其它占用 COM 口的程序（VS Code 串口监视、其它监视器）；
- SSID/密码含英文逗号无法下发（CLI 协议限制），页面已拦截并提示。

## 路线（对齐设计文档）

位图推送（M2 验收项）→ 布局编辑器 Canvas 1:1 → mDNS 局域网批量。
