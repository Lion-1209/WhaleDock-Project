#include "storage.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>

#include "ArduinoJson.h"
#include "config.h"

namespace storage {

namespace {

constexpr const char* kCfgPath = "/config.json";
constexpr const char* kBakPath = "/config.json.bak";
constexpr const char* kCacheDir = "/cache";
Preferences sPrefs;  // 敏感项（GitHub Token）走 NVS，与 Wi-Fi 凭据同级，不落 LittleFS

// 缓存名白名单（saveCache/loadCache 仅接受，防路径拼接注入）
bool validCacheName(const char* name) {
  for (const char* p = name; *p; ++p) {
    const char c = *p;
    const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
    if (!ok) return false;
  }
  return name[0] != '\0' && strlen(name) <= 16;
}

}  // namespace

bool begin() {
  // 分区标签 "lfs"（见分区表）；true = 分区未格式化时自动格一次
  if (!LittleFS.begin(true, "/littlefs", 10, "lfs")) {
    Serial.println("[FS] LittleFS 挂载失败（检查分区表 lfs 标签）");
    return false;
  }
  LittleFS.mkdir(kCacheDir);
  sPrefs.begin("whaledock", false);  // NVS 命名空间：token 等敏感项
  return true;
}

bool exists(const char* path) {
  return LittleFS.exists(path);
}

String readFile(const char* path) {
  File f = LittleFS.open(path, "r");
  if (!f) return "";
  const String s = f.readString();
  f.close();
  return s;
}

bool writeFile(const char* path, const String& content) {
  const String tmp = String(path) + ".tmp";
  File f = LittleFS.open(tmp, "w");
  if (!f) {
    Serial.printf("[FS] 写入失败（打开 %s）\n", tmp.c_str());
    return false;
  }
  const size_t n = f.print(content);
  f.close();
  if (n != content.length()) {
    Serial.println("[FS] 写入失败（空间不足？）");
    return false;
  }
  // 原子性优先（全路径无 remove，消除"删旧档与 rename 之间掉电=配置丢失"窗口）：
  // ① LittleFS 若支持 rename 覆盖 → 直接成功；② 否则旧档先轮换为 .bak 再换入新档，
  // 失败则回滚 .bak——任何时点掉电都有完整可读档（config 或 bak）
  if (LittleFS.rename(tmp, path)) return true;
  if (!LittleFS.exists(path)) return LittleFS.rename(tmp, path);
  if (!LittleFS.rename(path, kBakPath)) return false;
  if (LittleFS.rename(tmp, path)) return true;
  LittleFS.rename(kBakPath, path);  // 换入失败：回滚旧档
  return false;
}

bool removeFile(const char* path) {
  return LittleFS.remove(path);
}

bool format() {
  const bool ok = LittleFS.format();
  if (ok) LittleFS.mkdir(kCacheDir);
  return ok;
}

std::vector<String> listDir(const char* path) {
  std::vector<String> names;
  File dir = LittleFS.open(path);
  if (!dir) return names;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    names.push_back(String(f.name()) + (f.isDirectory() ? "/" : ""));
    f.close();
  }
  dir.close();
  return names;
}

bool loadConfig(Config& out) {
  // 无配置文件（出厂态）时给默认值：Datawhale 组织（datawhalechina）
  out.githubUser = GITHUB_DEFAULT_USER;
  out.githubRepo = "";
  out.githubToken = sPrefs.getString("gh_token", "");  // token 走 NVS（不落 LittleFS）
  String s = readFile(kCfgPath);
  if (!s.length()) s = readFile(kBakPath);  // 主档损坏时兜底（掉电窗口保险）
  if (!s.length()) return false;  // 出厂态（默认值已填，文件尚不存在）
  JsonDocument doc;
  if (deserializeJson(doc, s)) return false;
  out.githubUser = doc["githubUser"] | GITHUB_DEFAULT_USER;
  out.githubRepo = doc["githubRepo"] | "";
  // 一次性迁移：旧版把 token 明文存 config.json，迁入 NVS 后从文件清除
  const char* legacy = doc["githubToken"] | "";
  if (*legacy && out.githubToken.isEmpty()) {
    out.githubToken = legacy;
    sPrefs.putString("gh_token", legacy);
    saveConfig(out);  // 重写 json（不含 token 字段）
    Serial.println("[FS] GitHub Token 已从 config.json 迁移至 NVS");
  }
  return true;
}

bool saveConfig(const Config& c) {
  sPrefs.putString("gh_token", c.githubToken);  // token 走 NVS
  JsonDocument doc;
  doc["githubUser"] = c.githubUser;
  doc["githubRepo"] = c.githubRepo;
  String out;
  serializeJson(doc, out);
  return writeFile(kCfgPath, out);
}

bool saveCache(const char* name, const String& json) {
  if (!validCacheName(name)) {
    Serial.printf("[FS] 非法缓存名（白名单 [a-z0-9_]）：%s\n", name);
    return false;
  }
  const String path = String(kCacheDir) + "/" + name + ".json";
  return writeFile(path.c_str(), json);
}

String loadCache(const char* name) {
  if (!validCacheName(name)) return "";
  const String path = String(kCacheDir) + "/" + name + ".json";
  return readFile(path.c_str());
}

}  // namespace storage
