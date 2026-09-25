#pragma once
// ============================================================
// 服务层 · GitHub REST 拉取（M2 · A2）
// 设计文档数据源端点映射（一期全部 REST，无需 token）：
//   github.user  → GET /users/{u}                       （公开仓库数 / 关注者）
//   github.repo  → GET /repos/{o}/{r}                   （Stars / Forks）
//   月度柱状图   → GET /repos/{o}/{r}/stats/participation（52 周提交序列；
//                  原方案的 commit-activity 已废弃，2026-09 实测 404）
// 限额：无 token 60 次/时/IP，整点一次远低于限额。
//
// 已知工程偏差（有意为之，A4/RTOS 任务拆分时归还）：
// HTTP 同步阻塞（超时 10s），当前跑在 loopTask——CLI 调试场景可接受，
// 挂上整点流水后拉取期间呼吸灯会暂停数秒，届时移入独立任务。
// TLS 暂 setInsecure（TODO：正式版做证书校验，见设计文档风险表）。
// ============================================================

#include <vector>

#include "WString.h"

namespace github {

struct UserStats {
  bool ok = false;
  String login;
  int publicRepos = 0;
  int followers = 0;
  String fetchedAt;  // 拉取时刻（NTP 已同步时；空 = 未对时）
  String error;
};

struct RepoStats {
  bool ok = false;
  String fullName;
  int stars = 0;
  int forks = 0;
  String fetchedAt;
  String error;
};

struct CommitActivity {
  bool ok = false;
  std::vector<int> weeklyTotals;  // 每周提交数（旧 → 新，参与接口给 52 周；
                                  // v0.12 起全量保留，热力图取尾部 26 周窗口）
  int total = 0;                  // 合计
  String fetchedAt;
  String error;
};

UserStats fetchUser(const char* login);
RepoStats fetchRepo(const char* owner, const char* repo);
CommitActivity fetchCommitActivity(const char* owner, const char* repo);

// ---- 缓存（v0.12：拉取与落盘解耦，写哪里由调用方决定） ----
// 全局缓存 = 设备配置口径，未绑定 source 挂件的兜底渲染数据
void saveUserCache(const UserStats& u);
void saveRepoCache(const RepoStats& r);
void saveCommitsCache(const CommitActivity& c);
bool loadCachedUser(UserStats& out);
bool loadCachedRepo(RepoStats& out);
bool loadCachedCommits(CommitActivity& out);

// 按数据源 id 的缓存（协议 §5 dataSources 绑定）：id 净化为 [a-z0-9_]
// （非法字符转 _，含 src_ 前缀共 ≤16 位）后作 LittleFS 缓存名，
// 单文件合并存该源全部分项；saveSourceData 只覆盖非空分项（保留旧分项）
void saveSourceData(const char* id, const UserStats* u, const RepoStats* r,
                    const CommitActivity* c);
bool loadSourceUser(const char* id, UserStats& out);
bool loadSourceRepo(const char* id, RepoStats& out);
bool loadSourceCommits(const char* id, CommitActivity& out);

}  // namespace github
