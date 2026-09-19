# 鲸屿 WhaleDock

> 鲸尾托手机，墨屏映沧海 —— Datawhale 智能桌搭（ESP32-S3 + 7.5" 三色墨水屏）
>
> 一座长在工位上的“数据小岛”：鲸尾托着手机，墨水屏映着海面，GitHub 的每一次提交都会让岛上的鲸鱼活过来。

开源硬件项目：固件、上位机、结构图纸全部开源（for the learner）。

## 仓库结构

| 目录 | 内容 |
|---|---|
| `firmware/` | ESP32-S3 固件（PlatformIO / Arduino-ESP32 3.x） |
| `docs/` | 环境搭建、协议、开发文档 |
| `console/` | Web 上位机（起步：Web Serial 配网调机台，纯静态零后端） |
| `hardware/` | 结构 STEP 图纸（规划中） |

## 快速开始

见 [docs/环境搭建.md](docs/环境搭建.md)。核心三步：

```bash
cd firmware
pio run            # 首次约下载 1GB 工具链（国内需配代理，见文档 §2）
pio run -t upload  # 烧录（USB-C 接 DevKitC-1 的 "USB" 口）
```

## 当前状态

- [x] M1 自检程序：Flash/PSRAM 自检 + 双色测试图点屏（棋盘格验证黑白平面、红角标验证红平面）
- [ ] M2 数据闭环：Wi-Fi 事件状态机 + 配网 + NTP + GitHub REST + 整点刷新 + 串口 CLI
  - 已完成：Wi-Fi 状态机（断开自愈）、串口 CLI（含中文 SSID）、console/ Web 配网调机台
  - 待做：NTP 对时与整点调度、GitHub REST 拉取、LittleFS 缓存
- [ ] Widget 引擎 / 布局 JSON 协议 v1 / 上位机 / OTA（里程碑见设计文档）

## 许可

MIT（与主计划 TBD-6 一致，正式开源发布时统一确认）
