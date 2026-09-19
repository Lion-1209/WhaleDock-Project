此目录存放本项目私有库。PlatformIO 约定：`lib/<库名>/` 每个子文件夹是一个独立库（含 `library.json` 或 `src/`），只对本工程可见。

规划中的库拆分（M2 起逐步落地，对齐设计文档固件分层）：

- `WidgetEngine/` —— Widget 渲染引擎（`draw()` 注册制 + 布局 JSON 解析）
- `DataSources/` —— GitHub / NTP 数据拉取与 LittleFS 缓存
- `NetService/` —— 配网 / 局域网 HTTP API / mDNS / OTA 双分区
