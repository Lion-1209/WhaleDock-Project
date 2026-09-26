# 鲸屿 WhaleDock

> 鲸尾托手机，墨屏映沧海 —— Datawhale 智能桌搭（ESP32-S3 + 7.5" 四色墨水屏：黑白红黄）
>
> 一座长在工位上的“数据小岛”：鲸尾托着手机，墨水屏映着海面，GitHub 的每一次提交都会让岛上的鲸鱼活过来。

开源硬件项目：固件、上位机、结构图纸全部开源（for the learner）。

## 仓库结构

| 目录 | 内容 |
|---|---|
| `firmware/` | ESP32-S3 固件（PlatformIO / Arduino-ESP32 3.x） |
| `docs/` | 环境搭建、协议、开发文档 |
| `console/` | Web 上位机「统一控制台」：布局编辑器（拖拽排版/数据源绑定/一键推送）+ 设备运维（配网/OTA/存储/日志），串口与局域网双通道，纯静态零后端 |
| `hardware/` | 结构 STEP 图纸（规划中） |

## 快速开始

见 [docs/环境搭建.md](docs/环境搭建.md)。核心三步：

```bash
cd firmware
pio run            # 首次约下载 1GB 工具链（国内需配代理，见文档 §2）
pio run -t upload  # 烧录（USB-C 接 DevKitC-1 的 "USB" 口）
```

已联网的设备支持 **OTA 免线升级**（网页一键 / 串口命令，详见 [docs/OTA升级指南.md](docs/OTA升级指南.md)）；上位机见 [console/](console/)——布局编辑 + 设备运维统一入口（[线上版](https://lion-1209.github.io/WhaleDock-Project/console/)）。

## 当前状态（v0.12.0，实机验收）

- [x] M1 板级自检：Flash/PSRAM 自检 + 四色点屏（黑白红黄，800×480）
- [x] M2 数据闭环：Wi-Fi 状态机（断开自愈）/ Web 配网 / NTP 整点调度 / GitHub REST 拉数（含 52 周提交序列）/ LittleFS 缓存 / 串口 CLI（28 命令）
- [x] 屏驱动：四色全刷 22s 实测稳定，刷新期射频静默防断流
- [x] 显示协议 v1.2（12 种挂件 + 中文字库子集 + dataSources 声明式数据源 + 资源内联）：三方同源校验（固件/编辑器/模拟器），见 [docs/显示协议-v1.md](docs/显示协议-v1.md)
- [x] C2 Widget 引擎：布局 JSON 实时渲染上屏；dataSources 按声明逐源拉数、挂件 source 绑定
- [x] 上位机：统一控制台（布局编辑器拖拽排版/图片导入/数据源绑定/一键推送 + 设备运维 8 区），设计语言「桌面」+ 氛围层「潮汐」
- [x] D4 OTA：HTTP 下载 + 双分区 + MD5 + 自动确认/回滚
- [ ] C3 外设补全：水位灯 ×2 + 按键 ×2
- [ ] E1-E3 量产备产：老化、40 台一致性、结构

## 许可

MIT（与主计划 TBD-6 一致，正式开源发布时统一确认）
