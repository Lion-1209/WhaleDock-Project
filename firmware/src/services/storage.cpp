#include "storage.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "ArduinoJson.h"
#include "config.h"

namespace storage {

namespace {

constexpr const char* kCfgPath = "/config.json";
constexpr const char* kCacheDir = "/cache";

}  // namespace

bool begin() {
  // 分区标签 "lfs"（见分区表）；true = 分区未格式化时自动格一次
  if (!LittleFS.begin(true, "/littlefs", 10, "lfs")) {
    Serial.println("[FS] LittleFS 挂载失败（检查分区表 lfs 标签）");
    return false;
  }
  LittleFS.mkdir(kCacheDir);
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
  LittleFS.remove(path);  // rename 不覆盖，先删旧档
  return LittleFS.rename(tmp, path);
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
  const String s = readFile(kCfgPath);
  if (!s.length()) return false;  // 出厂态（默认值已填，文件尚不存在）
  JsonDocument doc;
  if (deserializeJson(doc, s)) return false;
  out.githubUser = doc["githubUser"] | GITHUB_DEFAULT_USER;
  out.githubRepo = doc["githubRepo"] | "";
  return true;
}

bool saveConfig(const Config& c) {
  JsonDocument doc;
  doc["githubUser"] = c.githubUser;
  doc["githubRepo"] = c.githubRepo;
  String out;
  serializeJson(doc, out);
  return writeFile(kCfgPath, out);
}

bool saveCache(const char* name, const String& json) {
  const String path = String(kCacheDir) + "/" + name + ".json";
  return writeFile(path.c_str(), json);
}

String loadCache(const char* name) {
  const String path = String(kCacheDir) + "/" + name + ".json";
  return readFile(path.c_str());
}

}  // namespace storage
