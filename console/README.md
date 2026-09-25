# console/ · Web 上位机

零后端、零构建：纯静态文件，双通道直连设备——浏览器经 **Web Serial** 走串口（协议为固件 CLI，`firmware/src/services/cli`），或经设备自建 **HTTP API + mDNS** 走局域网（`firmware/src/services/webapi`）。

界面设计语言「桌面」：控制台长成设备周围的那张桌子——深墨桌面（机身边框同族暖黑）上摆纸白面板，取色自 7.5″ 四色墨水屏（纸白/墨黑/屏红/屏黄：红只做印章式强调与数据色，黄做焦点环）；标题走衬线印刷字配等宽英文副标，地址/日志/JSON 走等宽字，设备画布以真机边框形态出场（设计系统集中在 `css/style.css`，类名契约与 `js/` 解耦）。

入口 [`index.html`](./index.html) = 统一控制台（双页签）：

**页签一 · 布局编辑**（原独立编辑器并入，`editor.html` 已重定向至此）

- 拖拽排版（移动/边角缩放写回 slotRect）、图层管理、按类型属性编辑
- 数据源卡：声明布局 dataSources（id/类型/参数固定格式，id 改名级联、被引用禁删），挂件 `source` 下拉绑定——整点流水按声明逐源拉数（协议 §5）
- 图片导入（Bayer/Otsu 二值化）、中文字库子集自动生成、布局打开/保存/自动存档
- USB 探测（串口取 IP/mDNS）+ 一键推送上屏（HTTP API）
- JSON 调试卡：粘贴协议 JSON → 与固件同源校验 → 四色渲染（与编辑文档互转）

**页签二 · 设备运维**（原 8 卡调机台并入）

- **设备**：连接管理（授权一次后页内直连）、设备时间、重启
- **Wi-Fi**：表单配网、凭据状态、周边 AP 扫描（点击回填 SSID）、清除凭据
- **兜底数据配置**：GitHub 用户/仓库/Token 设备级配置（布局未声明数据源时生效；Token 仅此处可配）、立即拉取
- **GitHub**：用户/仓库查询、近 12 周提交柱状图
- **OTA**：固件直链升级（默认 GitHub Release 最新版）、进度条、确认/回滚
- **存储**：LittleFS 文件浏览、内容查看、格式化
- **局域网**：mDNS 名或 IP 直连，状态轮询/流水/OTA/重启全网络操作（日志仍走串口）
- **日志**：实时串口日志（自动滚动）、自由命令输入

**开发用**：显示协议模拟器 [`simulator.html`](./simulator.html) —— 协议联调独立页（编辑器与模拟器共享 `js/sim.js` 渲染/校验内核）

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
