#include "datapipe.h"

#include <Arduino.h>

#include "github.h"
#include "ntp.h"
#include "storage.h"
#include "wifi.h"

namespace datapipe {

namespace {

bool sBusy = false;

void onHourly() {
  runOnce("整点");
}

}  // namespace

void begin() {
  ntp::setHourlyCallback(onHourly);
}

bool busy() {
  return sBusy;
}

void runOnce(const char* trigger) {
  if (wifi::state() != wifi::State::Connected) {
    Serial.printf("[流水] %s触发：Wi-Fi 未连接，跳过本轮（下个整点自动重试）\n",
                  trigger);
    return;
  }

  sBusy = true;
  const uint32_t t0 = millis();
  Serial.printf("[流水] %s触发：按设备配置拉取…\n", trigger);

  storage::Config cfg;
  storage::loadConfig(cfg);  // 无配置文件时自动带出厂默认（config.h）

  if (!cfg.githubUser.isEmpty()) {
    const github::UserStats u = github::fetchUser(cfg.githubUser.c_str());
    if (u.ok) {
      Serial.printf("[流水] 用户 %s：公开仓库 %d · 关注者 %d\n", u.login.c_str(),
                    u.publicRepos, u.followers);
    } else {
      Serial.printf("[流水] 用户拉取失败：%s（渲染时用缓存兜底）\n",
                    u.error.c_str());
    }
  }

  if (!cfg.githubRepo.isEmpty()) {
    const int slash = cfg.githubRepo.indexOf('/');
    if (slash > 0 && slash < (int)cfg.githubRepo.length() - 1) {
      const String owner = cfg.githubRepo.substring(0, slash);
      const String repo = cfg.githubRepo.substring(slash + 1);

      const github::RepoStats s = github::fetchRepo(owner.c_str(), repo.c_str());
      if (s.ok) {
        Serial.printf("[流水] 仓库 %s：Stars %d · Forks %d\n", s.fullName.c_str(),
                      s.stars, s.forks);
      } else {
        Serial.printf("[流水] 仓库拉取失败：%s\n", s.error.c_str());
      }

      const github::CommitActivity c =
          github::fetchCommitActivity(owner.c_str(), repo.c_str());
      if (c.ok) {
        Serial.printf("[流水] %s 近 %d 周提交合计 %d 次\n", cfg.githubRepo.c_str(),
                      (int)c.weeklyTotals.size(), c.total);
      } else {
        Serial.printf("[流水] 提交序列拉取失败：%s\n", c.error.c_str());
      }
    } else {
      Serial.println("[流水] 配置的仓库格式异常（应为 owner/repo），已跳过");
    }
  }

  Serial.printf("[流水] 完成，耗时 %.1fs（成功项缓存已更新）\n",
                (millis() - t0) / 1000.0);
  sBusy = false;
}

}  // namespace datapipe
