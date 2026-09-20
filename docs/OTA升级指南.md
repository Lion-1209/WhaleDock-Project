# OTA 固件升级指南

> 设备联网后**免 USB 线**升级固件。适用于固件 **v0.2.0 及以上**（OTA 功能自该版本引入，更早版本请先用 USB 烧录一次）。

## 快速上手（网页）

1. 打开 [console 配网调机台](../console/)（本地运行见其 README），连接设备；
2. 「固件升级（OTA）」卡中地址栏默认已填官方最新版直链：
   `https://github.com/Lion-1209/WhaleDock-Project/releases/latest/download/firmware.bin`
3. 点「开始升级」→ 看进度条（下载 + 写入约 1 分钟）→ 设备自动重启进入新固件；
4. 重启后状态行显示新版本号即成功（升级后设备自动恢复联网/对时/整点流水，无需任何操作）。

## 快速上手（串口 CLI）

```
ota status                          # 当前版本 / 所在分区 / 备用分区
ota https://…/firmware.bin          # 从直链下载并升级
ota confirm                         # （如有待验证状态）确认新固件
ota rollback                        # 回滚到上一分区固件
```

## 安全模型

- **写备用分区**：下载的固件写入当前运行分区之外的另一个 OTA 分区，成功校验后才切换启动；
- **失败安全**：下载中断 / 校验失败只是中止本次升级，**当前固件毫发无损**——重试是免费的；
- **双分区后备**：升级后旧固件完整保留在另一分区（新固件无法启动时 bootloader 自动落回）；
- 回滚说明（如实）：当前 Arduino 预编译内核未启用 bootloader 级"待验证自动回滚"，保护层级为上述三条；量产前评估是否自编译 bootloader 开启完整回滚；
- 唯一禁区：**写入期间（约十几秒）勿断电**。

## 发布新版本（开发者）

1. 改代码，并把 `firmware/include/config.h` 的 `FW_VERSION` **递增**（否则升级后看不出变化）；
2. commit → `git tag vX.Y.Z` → push（含 tag）；
3. CI（`.github/workflows/release.yml`）自动构建并发布 Release（firmware / bootloader / partitions 三个附件）；
4. 所有设备点一下「开始升级」即升到最新（latest 直链永远指向最新 Release）。

## 网络不通 / 下载失败？

`github.com` 直连在国内**间歇性**不稳定，失败（HTTP -1 等）不影响设备，直接**重试**即可，通常几次内成功。长期恶化时：

- OTA 地址是自由参数——可指向任意镜像或自建服务器上的 `.bin`（改网页输入框或 CLI 的 URL 即可，不用改固件）；
- 数据拉取通道（`api.github.com`）与 OTA 下载通道（`github.com`）是**独立**的域名，一条恶化不代表另一条不通。

## 常见问题

| 现象 | 说明 |
|---|---|
| 升级后版本号没变 | 目标 Release 的 `FW_VERSION` 没有递增，或下到的是同一版本 |
| `下载失败：HTTP -1/-5` | 网络抖动，重试；见上节 |
| `写入不完整` 后设备正常 | 下载中途断流，已自动中止，当前固件未受影响，重试 |
| `ota confirm` 显示"无待验证固件" | 正常——回滚配置未启用时无待验证状态 |
| 想回到旧版本 | 用旧版本 Release 的具体 URL（如 `…/download/v0.2.0/firmware.bin`）再升一次即可 |
