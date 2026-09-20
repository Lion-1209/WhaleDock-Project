#pragma once
// ============================================================
// 服务层 · HTTP OTA 升级（M2 · D4）
// 流程：下载 .bin → Update 流式写入【备用】OTA 分区 → 校验 → 重启
//       → 新固件进入"待验证"态 → 健康运行 OTA_CONFIRM_S 自动确认
//       （或 CLI/网页手动确认 / ota rollback 手动回滚）。
// 安全性：下载中断/校验失败只 abort，当前固件不受影响；
// 写入期间勿断电（闪存写入窗口，约数秒）。
// 已知偏差：下载与写入阻塞 loopTask（与 datapipe 同类，§9 拆分时归还）。
// ============================================================

namespace ota {

void begin();  // setup 调用：检测待验证状态并布防自动确认
void poll();   // loop 调用：待验证固件健康运行超时后自动确认

void fromUrl(const char* url);       // 下载并刷入备用分区，成功后重启
void confirm(const char* why);       // 确认新固件（清除回滚点）
void rollbackNow();                  // 标记当前固件无效并回滚重启
void printStatus();                  // 版本/分区/状态

}  // namespace ota
