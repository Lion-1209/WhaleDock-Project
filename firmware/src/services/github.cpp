#include "github.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "certs.h"
#include "config.h"
#include "ntp.h"
#include "storage.h"

namespace github {

namespace {

// 拉取成功即落盘（storage 原子写），fetchedAt 供断网兜底渲染的"数据截至"角标
void cacheUser(const UserStats& u) {
  JsonDocument doc;
  doc["login"] = u.login;
  doc["public_repos"] = u.publicRepos;
  doc["followers"] = u.followers;
  doc["fetchedAt"] = u.fetchedAt;
  String out;
  serializeJson(doc, out);
  storage::saveCache("user", out);
}

void cacheRepo(const RepoStats& r) {
  JsonDocument doc;
  doc["full_name"] = r.fullName;
  doc["stars"] = r.stars;
  doc["forks"] = r.forks;
  doc["fetchedAt"] = r.fetchedAt;
  String out;
  serializeJson(doc, out);
  storage::saveCache("repo", out);
}

void cacheCommits(const CommitActivity& c) {
  JsonDocument doc;
  JsonArray weeks = doc["weeklyTotals"].to<JsonArray>();
  for (const int w : c.weeklyTotals) weeks.add(w);
  doc["total"] = c.total;
  doc["fetchedAt"] = c.fetchedAt;
  String out;
  serializeJson(doc, out);
  storage::saveCache("commits", out);
}

String nowStamp() {
  return ntp::synced() ? String(ntp::timeString()) : String("");
}


// 统一 GET：成功返回 200 且填充 payload；返回值<0 为网络层错误，>0 为 HTTP 状态码
int get(const String& path, String& payload) {
  const bool proxied = GITHUB_PROXY_PREFIX[0] != '\0';
  if (proxied && strncmp(GITHUB_PROXY_PREFIX, "https://", 8) != 0) {
    Serial.println("[GitHub] 代理前缀必须 https://（明文中转会把 Bearer 头暴露给同网段，已禁用）");
    return -2;
  }
  if (!ntp::synced()) {
    Serial.println("[GitHub] 未完成 NTP 对时（证书有效期校验需正确时间），稍后再试");
    return -2;
  }
  const String url = proxied ? String(GITHUB_PROXY_PREFIX) + path
                             : String(GITHUB_API_BASE) + path;

  // 客户端对象必须与 http 同层存活到 end()：HTTPClient 只持指针，
  // 分支内局部变量会在 GET() 前析构导致野指针
  WiFiClientSecure tls;
  HTTPClient http;
  tls.setCACert(kGithubRoots);  // 根证书校验（替代 setInsecure，防 MITM 窃取 Bearer）
  if (!http.begin(tls, url)) return -1;

  http.setTimeout(10000);
  http.addHeader("User-Agent", "WhaleDock-Firmware");
  http.addHeader("Accept", "application/vnd.github+json");
  // 配置了 PAT 则走认证（限额 60/h → 5000/h）；token 每次从配置读，
  // 改配置即时生效。明文存于 config.json，与 Wi-Fi 密码同级的现阶段取舍
  storage::Config cfg;
  storage::loadConfig(cfg);
  if (!cfg.githubToken.isEmpty())
    http.addHeader("Authorization", "Bearer " + cfg.githubToken);
  const int code = http.GET();
  if (code == HTTP_CODE_OK) payload = http.getString();
  http.end();
  return code;
}

String errOf(int code) {
  if (code < 0) {
    String s = "网络/TLS 错误(";
    s += code;
    s += ")：检查网络可达性；若网络存在 TLS 拦截，改用 https 中转前缀";
    return s;
  }
  if (code == 202) return "HTTP 202：GitHub 统计计算中，稍后再试";
  if (code == 401) return "HTTP 401：token 无效或过期（config token - 可清除）";
  if (code == 403) return "HTTP 403：限额或被封（匿名 60 次/时/IP，配 token 可至 5000）";
  String s = "HTTP ";
  s += code;
  return s;
}

}  // namespace

UserStats fetchUser(const char* login) {
  UserStats r;
  r.login = login;
  String body;
  const int code = get(String("/users/") + login, body);
  if (code != HTTP_CODE_OK) {
    r.error = errOf(code);
    return r;
  }
  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    r.error = "JSON 解析失败";
    return r;
  }
  r.ok = true;
  r.login = doc["login"] | login;
  r.publicRepos = doc["public_repos"] | 0;
  r.followers = doc["followers"] | 0;
  r.fetchedAt = nowStamp();
  cacheUser(r);
  return r;
}

RepoStats fetchRepo(const char* owner, const char* repo) {
  RepoStats r;
  r.fullName = String(owner) + "/" + repo;
  String body;
  const int code = get(String("/repos/") + owner + "/" + repo, body);
  if (code != HTTP_CODE_OK) {
    r.error = errOf(code);
    return r;
  }
  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    r.error = "JSON 解析失败";
    return r;
  }
  r.ok = true;
  r.fullName = doc["full_name"] | r.fullName;
  r.stars = doc["stargazers_count"] | 0;
  r.forks = doc["forks_count"] | 0;
  r.fetchedAt = nowStamp();
  cacheRepo(r);
  return r;
}

CommitActivity fetchCommitActivity(const char* owner, const char* repo) {
  CommitActivity r;
  // 注意：设计文档原文写的 /stats/commit-activity 已废弃（2026-09 实测
  // 连认证访问都 404），改用 /stats/participation——同为 52 周提交序列，
  // 语义等价且响应更轻（{all:[52], owner:[52]}，all = 全体贡献者合计）
  String body;
  const int code =
      get(String("/repos/") + owner + "/" + repo + "/stats/participation", body);
  if (code != HTTP_CODE_OK) {
    r.error = code == 404 ? String("仓库不存在，或 GitHub 无统计数据（接口返回 404）")
                          : errOf(code);
    return r;
  }
  JsonDocument doc;
  if (deserializeJson(doc, body) || doc["all"].isNull() ||
      !doc["all"].is<JsonArray>()) {
    r.error = "JSON 解析失败";
    return r;
  }
  r.ok = true;
  JsonArray all = doc["all"];
  const size_t weeks = all.size();
  const size_t from = weeks > 12 ? weeks - 12 : 0;  // 只取最近 12 周（旧 → 新）
  for (size_t i = from; i < weeks; ++i) {
    const int t = all[i] | 0;
    r.weeklyTotals.push_back(t);
    r.total += t;
  }
  r.fetchedAt = nowStamp();
  cacheCommits(r);
  return r;
}

// ---- 缓存回读（A4 断网兜底渲染用） ----

bool loadCachedUser(UserStats& out) {
  const String s = storage::loadCache("user");
  if (!s.length()) return false;
  JsonDocument doc;
  if (deserializeJson(doc, s)) return false;
  out.ok = true;
  out.login = doc["login"] | "";
  out.publicRepos = doc["public_repos"] | 0;
  out.followers = doc["followers"] | 0;
  out.fetchedAt = doc["fetchedAt"] | "";
  return true;
}

bool loadCachedRepo(RepoStats& out) {
  const String s = storage::loadCache("repo");
  if (!s.length()) return false;
  JsonDocument doc;
  if (deserializeJson(doc, s)) return false;
  out.ok = true;
  out.fullName = doc["full_name"] | "";
  out.stars = doc["stars"] | 0;
  out.forks = doc["forks"] | 0;
  out.fetchedAt = doc["fetchedAt"] | "";
  return true;
}

bool loadCachedCommits(CommitActivity& out) {
  const String s = storage::loadCache("commits");
  if (!s.length()) return false;
  JsonDocument doc;
  if (deserializeJson(doc, s)) return false;
  JsonArray weeks = doc["weeklyTotals"];
  if (weeks.isNull()) return false;
  out.ok = true;
  for (JsonVariant v : weeks) out.weeklyTotals.push_back(v | 0);
  out.total = doc["total"] | 0;
  out.fetchedAt = doc["fetchedAt"] | "";
  return true;
}

}  // namespace github
