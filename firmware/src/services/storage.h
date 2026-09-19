#pragma once
// ============================================================
// 服务层 · LittleFS 存储（M2 · A3）
// 目的：断电断网不失忆——配置（要查什么）与数据缓存（上次查到什么）
// 落盘持久化，M2 验收项"断电/断网重启：配置与缓存保留"。
//
// 分层约定：本模块只做文件设施（挂载/原子写/读/列/删）+ 应用配置
// （Config）；数据缓存的内容格式由数据生产方（github 模块）自定，
// 经 saveCache/loadCache 存取——A4 流水线与断网兜底渲染的交接面。
//
// 分区：partitions_whaledock.csv 标签 "lfs"（0xC90000 起，2.9MB）。
// ============================================================

#include <vector>

#include "WString.h"

namespace storage {

bool begin();  // 挂载 LittleFS（分区不存在时自动格式化）；失败返回 false

bool exists(const char* path);
String readFile(const char* path);                         // 不存在返回空串
bool writeFile(const char* path, const String& content);   // 原子写：.tmp → rename
bool removeFile(const char* path);
bool format();                                             // 危险：全盘格式化（调试用）
std::vector<String> listDir(const char* path);             // 一级条目（目录带 / 后缀）

// ---- 应用配置（/config.json）----
struct Config {
  String githubUser;   // github.user 数据源（空 = 未配置）
  String githubRepo;   // github.repo 数据源 "owner/repo"（空 = 未配置）
  String githubToken;  // GitHub PAT（空 = 匿名，60 次/h；配置后认证 5000 次/h）
};
bool loadConfig(Config& out);      // false = 尚无配置文件（出厂态）
bool saveConfig(const Config& c);

// ---- 数据缓存（/cache/<name>.json）----
bool saveCache(const char* name, const String& json);
String loadCache(const char* name);  // 不存在返回空串

}  // namespace storage
