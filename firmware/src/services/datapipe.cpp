#include "datapipe.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "github.h"
#include "ntp.h"
#include "storage.h"
#include "wifi.h"
#include "worker.h"

namespace datapipe {

namespace {

bool sBusy = false;

void onHourly() {
  // §9：回调上下文只入队，拉取在 worker 任务执行（loopTask 不再阻塞数秒）
  worker::requestPipe();
}

}  // namespace

void begin() {
  ntp::setHourlyCallback(onHourly);
}

bool busy() {
  return sBusy;
}

void runOnce(const char* trigger) {
  // 有界等网 20s：紧邻屏刷新的调用（射频静默后 Wi-Fi 重连退避中）能自愈
  if (wifi::state() != wifi::State::Connected) {
    Serial.printf("[流水] %s触发：Wi-Fi 未连接，等待重连（最多 20s）…\n", trigger);
    const uint32_t tWait = millis();
    while (wifi::state() != wifi::State::Connected && millis() - tWait < 20000)
      delay(500);
  }
  if (wifi::state() != wifi::State::Connected) {
    Serial.printf("[流水] %s触发：Wi-Fi 未连接，跳过本轮（下个整点自动重试）\n",
                  trigger);
    return;
  }

  sBusy = true;
  const uint32_t t0 = millis();

  // 取数口径（协议 §5）：/layout.json 声明了 dataSources → 按声明逐源拉数，
  // 写 per-id 缓存（绑定 source 的挂件读它）；首个成功的 user/repo/commits
  // 同步全局缓存（未绑定 source 挂件的兜底口径）。无声明 → 按设备配置（存量行为）
  String layoutJson = storage::readFile("/layout.json");
  JsonDocument ldoc;
  JsonArray dss;
  if (layoutJson.length() && !deserializeJson(ldoc, layoutJson))
    dss = ldoc["dataSources"].as<JsonArray>();

  if (!dss.isNull() && dss.size() > 0) {
    Serial.printf("[流水] %s触发：按布局 dataSources 声明拉取…\n", trigger);
    bool gUser = false, gRepo = false, gCommits = false;  // 全局缓存只吃首个成功结果
    for (JsonObject ds : dss) {
      const char* id = ds["id"] | "";
      const char* type = ds["type"] | "";
      const char* user = ds["params"]["user"] | "";
      const char* owner = ds["params"]["owner"] | "";
      const char* repo = ds["params"]["repo"] | "";
      if (!*id || !*type) {
        Serial.println("[流水] 跳过缺 id/type 的数据源声明");
        continue;
      }
      if (!strcmp(type, "github.user") && *user) {
        const github::UserStats u = github::fetchUser(user);
        if (u.ok) {
          github::saveSourceData(id, &u, nullptr, nullptr);
          if (!gUser) { github::saveUserCache(u); gUser = true; }
          Serial.printf("[流水] 源 %s 用户 %s：公开仓库 %d · 关注者 %d\n", id,
                        u.login.c_str(), u.publicRepos, u.followers);
        } else {
          Serial.printf("[流水] 源 %s 用户拉取失败：%s（渲染时用缓存兜底）\n",
                        id, u.error.c_str());
        }
      } else if (!strcmp(type, "github.repo") && *owner && *repo) {
        const github::RepoStats s = github::fetchRepo(owner, repo);
        const github::CommitActivity c = github::fetchCommitActivity(owner, repo);
        if (s.ok || c.ok)
          github::saveSourceData(id, nullptr, s.ok ? &s : nullptr, c.ok ? &c : nullptr);
        if (s.ok) {
          if (!gRepo) { github::saveRepoCache(s); gRepo = true; }
          Serial.printf("[流水] 源 %s 仓库 %s：Stars %d · Forks %d\n", id,
                        s.fullName.c_str(), s.stars, s.forks);
        } else {
          Serial.printf("[流水] 源 %s 仓库拉取失败：%s\n", id, s.error.c_str());
        }
        if (c.ok) {
          if (!gCommits) { github::saveCommitsCache(c); gCommits = true; }
          Serial.printf("[流水] 源 %s 近 %d 周提交合计 %d 次\n", id,
                        (int)c.weeklyTotals.size(), c.total);
        } else {
          Serial.printf("[流水] 源 %s 提交序列拉取失败：%s\n", id, c.error.c_str());
        }
      } else {
        Serial.printf("[流水] 源 %s 类型 %s 不识别或缺参数，跳过\n", id, type);
      }
    }
  } else {
    Serial.printf("[流水] %s触发：按设备配置拉取…\n", trigger);

    storage::Config cfg;
    storage::loadConfig(cfg);  // 无配置文件时自动带出厂默认（config.h）

    if (!cfg.githubUser.isEmpty()) {
      const github::UserStats u = github::fetchUser(cfg.githubUser.c_str());
      if (u.ok) {
        github::saveUserCache(u);
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
          github::saveRepoCache(s);
          Serial.printf("[流水] 仓库 %s：Stars %d · Forks %d\n", s.fullName.c_str(),
                        s.stars, s.forks);
        } else {
          Serial.printf("[流水] 仓库拉取失败：%s\n", s.error.c_str());
        }

        const github::CommitActivity c =
            github::fetchCommitActivity(owner.c_str(), repo.c_str());
        if (c.ok) {
          github::saveCommitsCache(c);
          Serial.printf("[流水] %s 近 %d 周提交合计 %d 次\n", cfg.githubRepo.c_str(),
                        (int)c.weeklyTotals.size(), c.total);
        } else {
          Serial.printf("[流水] 提交序列拉取失败：%s\n", c.error.c_str());
        }
      } else {
        Serial.println("[流水] 配置的仓库格式异常（应为 owner/repo），已跳过");
      }
    }
  }

  Serial.printf("[流水] 完成，耗时 %.1fs（成功项缓存已更新）\n",
                (millis() - t0) / 1000.0);
  worker::requestRenderFromWorker(worker::Render::Layout);  // 闭环：拉数→缓存→上屏
  sBusy = false;
}

}  // namespace datapipe
