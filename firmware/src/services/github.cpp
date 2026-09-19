#include "github.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "config.h"

namespace github {

namespace {

// 统一 GET：成功返回 200 且填充 payload；返回值<0 为网络层错误，>0 为 HTTP 状态码
int16_t get(const String& path, String& payload) {
  const bool proxied = GITHUB_PROXY_PREFIX[0] != '\0';
  const String url = proxied ? String(GITHUB_PROXY_PREFIX) + path
                             : String(GITHUB_API_BASE) + path;

  // 客户端对象必须与 http 同层存活到 end()：HTTPClient 只持指针，
  // 分支内局部变量会在 GET() 前析构导致野指针
  WiFiClientSecure tls;
  WiFiClient plain;
  HTTPClient http;
  bool begun;
  if (proxied && strncmp(GITHUB_PROXY_PREFIX, "https://", 8) != 0) {
    begun = http.begin(plain, url);  // http:// 代理中转走明文
  } else {
    tls.setInsecure();  // TODO：正式版证书校验（设计文档风险表项）
    begun = http.begin(tls, url);
  }
  if (!begun) return -1;

  http.setTimeout(10000);
  http.addHeader("User-Agent", "WhaleDock-Firmware");
  http.addHeader("Accept", "application/vnd.github+json");
  const int16_t code = http.GET();
  if (code == HTTP_CODE_OK) payload = http.getString();
  http.end();
  return code;
}

String errOf(int16_t code) {
  if (code < 0) {
    String s = "网络错误(";
    s += code;
    s += ")：检查网络/代理（GITHUB_PROXY_PREFIX）";
    return s;
  }
  if (code == 202) return "HTTP 202：GitHub 统计计算中，稍后再试";
  if (code == 403) return "HTTP 403：限额或被封（60 次/时/IP）";
  String s = "HTTP ";
  s += code;
  return s;
}

}  // namespace

UserStats fetchUser(const char* login) {
  UserStats r;
  r.login = login;
  String body;
  const int16_t code = get(String("/users/") + login, body);
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
  return r;
}

RepoStats fetchRepo(const char* owner, const char* repo) {
  RepoStats r;
  r.fullName = String(owner) + "/" + repo;
  String body;
  const int16_t code = get(String("/repos/") + owner + "/" + repo, body);
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
  return r;
}

CommitActivity fetchCommitActivity(const char* owner, const char* repo) {
  CommitActivity r;
  // 注意：设计文档原文写的 /stats/commit-activity 已废弃（2026-09 实测
  // 连认证访问都 404），改用 /stats/participation——同为 52 周提交序列，
  // 语义等价且响应更轻（{all:[52], owner:[52]}，all = 全体贡献者合计）
  String body;
  const int16_t code =
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
  return r;
}

}  // namespace github
