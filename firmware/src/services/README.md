# 服务层（M2 起启用）

按设计文档"固件方案要点"，数据类模块落位于此，每个模块一对 `.h/.cpp`、平铺：

- `wifi`：Wi-Fi 事件状态机（连上/断开自愈）+ 串口 CLI 配网（`wifi set ssid,pass`）
- `ntp`：SNTP 对时 + 整点调度（esp_timer 一次性定时器，不用空转 delay 对表）
- `github`：REST 拉取 `/users` `/repos` / commit-activity + LittleFS 缓存兜底
- `storage`：LittleFS 配置与缓存读写
- `ota`：HTTP OTA + 双分区回滚

约定（对齐开发约定§9）：`esp_event` 回调里只发标志/入队，重活回应用任务做；任何路径不阻塞超过秒级。
