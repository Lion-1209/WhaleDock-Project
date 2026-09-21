#include "layout.h"

#include <vector>

namespace layout {

const char* sampleJson() {
  // 概念图 v2（鲸屿形态）屏幕版式复刻（与模拟器 SAMPLE 一致；glyphs 置空：结构校验不展开字库）
  return R"({"version":1,"dataSources":[
    {"id":"gh","type":"github.user","params":{"user":"datawhalechina"}},
    {"id":"repo","type":"github.repo","params":{"owner":"datawhalechina","repo":"leeml-notes"}}],
  "layout":{"resolution":[800,480],"widgets":[
    {"slot":"tl","slotRect":{"x":12,"y":16,"w":300,"h":148},"type":"pet","res":"whale_pixel"},
    {"slot":"tr","slotRect":{"x":330,"y":16,"w":458,"h":148},"type":"clock","calendar":true,"align":"right"},
    {"slot":"bl","slotRect":{"x":12,"y":180,"w":776,"h":170},"type":"heatMap","source":"repo","title":"GitHub Contribution"},
    {"slot":"br","slotRect":{"x":236,"y":364,"w":552,"h":72},"type":"stats","variant":"chips",
     "fields":["public_repos","stars","forks","followers"],"labels":["Repositories","Stars","Forks","Followers"]},
    {"slot":"ticker","slotRect":{"x":424,"y":444,"w":364,"h":24},"type":"text","size":"s","align":"right","text":"Keep coding. Keep shipping."}]}})";
}

namespace {

// 规则 7：颜色枚举（qr 恒黑、无 color 字段，由调用处单独判定）
bool validColor(const char* c) {
  return !strcmp(c, "black") || !strcmp(c, "red") || !strcmp(c, "yellow");
}

}  // namespace

CheckResult check(const String& json) {
  CheckResult r;
  JsonDocument doc;
  if (DeserializationError e = deserializeJson(doc, json); e) {
    r.errors = String("规则0: JSON 解析失败（") + e.c_str() + "）";
    return r;
  }

  // 规则 1：version == 1
  if (doc["version"].as<int>() != 1) r.errors += "规则1: version 必须为 1\n";

  const bool hasLayout = doc["layout"].is<JsonObject>();
  const bool hasDisplay = doc["display"].is<JsonObject>();
  // 规则 2：layout 与 display 恰有一个
  if (hasLayout == hasDisplay)
    r.errors += "规则2: layout 与 display 必须恰有一个\n";

  if (hasDisplay) {  // 规则 9：位图平面解码长度（b64 → 恰 48000 B）
    r.bitmapMode = true;
    for (const char* plane : {"bitmapBW", "bitmapRed", "bitmapYellow"}) {
      const String b64 = doc["display"][plane] | "";
      if (!b64.length()) continue;  // 可省略 = 全白
      const size_t bytes = (b64.length() / 4) * 3 -
                           (b64.endsWith("==") ? 2 : b64.endsWith("=") ? 1 : 0);
      if (bytes != 48000)
        r.errors += String("规则9: ") + plane + " 解码 " + bytes + " B，应为 48000 B\n";
    }
  }

  if (!hasLayout) {
    r.ok = r.errors.length() == 0;
    return r;
  }

  JsonObject layoutO = doc["layout"];
  // 规则 3：resolution
  JsonArray res = layoutO["resolution"];
  if (res.isNull() || res.size() != 2 || res[0] != 800 || res[1] != 480)
    r.errors += "规则3: resolution 必须为 [800,480]\n";

  // 数据源 id 集合（规则 6 用）
  std::vector<String> sourceIds;
  for (JsonObject ds : doc["dataSources"].as<JsonArray>()) {
    if (ds["id"].is<const char*>()) sourceIds.push_back(ds["id"].as<String>());
    else r.errors += "规则6: dataSources 存在无 id 项\n";
  }
  r.sourceCount = sourceIds.size();

  // 槽位占用表（规则 4）
  std::vector<String> usedSlots;
  static const char* kQuadrants[] = {"tl", "tr", "bl", "br", "ticker"};
  static const char* kTypes[] = {"clock",   "stats", "barChart", "pet",
                                 "text",    "image", "qr",       "ticker",
                                 "heatMap"};

  JsonArray widgets = layoutO["widgets"];
  if (!widgets || widgets.size() == 0) {
    r.errors += "规则4: widgets 不能为空\n";
  } else {
    for (JsonObject w : widgets) {
      ++r.widgetCount;
      const char* slot = w["slot"] | "";
      const char* type = w["type"] | "";
      // 槽位合法 + 唯一
      bool slotOk = false;
      for (const char* q : kQuadrants) slotOk |= !strcmp(slot, q);
      if (!slotOk) { r.errors += String("规则4: 非法 slot '") + slot + "'\n"; continue; }
      for (const String& u : usedSlots)
        if (u == slot) r.errors += String("规则4: slot '") + slot + "' 重复占用\n";
      usedSlots.push_back(slot);
      // 类型注册（规则 5）
      bool typeOk = false;
      for (const char* t : kTypes) typeOk |= !strcmp(type, t);
      if (!typeOk) { r.errors += String("规则5: 未注册类型 '") + type + "'\n"; continue; }
      // ticker 槽类型限制（规则 4）
      if (!strcmp(slot, "ticker") && strcmp(type, "ticker") && strcmp(type, "text"))
        r.errors += String("规则4: ticker 槽不允许类型 '") + type + "'\n";
      // 规则 12（v1.1）：slotRect 覆写边界
      if (w["slotRect"].is<JsonObject>()) {
        JsonObject sr = w["slotRect"];
        const int sx = sr["x"] | -1, sy = sr["y"] | -1;
        const int sw = sr["w"] | -1, sh = sr["h"] | -1;
        if (sx < 0 || sy < 0 || sw <= 0 || sh <= 0 || sx + sw > 800 || sy + sh > 480)
          r.errors += String("规则12: slotRect 越界或非法（'") + slot + "'）\n";
      }
      // 颜色（规则 7）
      if (w["color"].is<const char*>()) {
        const char* c = w["color"];
        if (!strcmp(type, "qr")) r.errors += "规则7: qr 恒黑，不允许 color\n";
        else if (!validColor(c))
          r.errors += String("规则7: 非法颜色 '") + c + "'\n";
      }
      if (w["goalColor"].is<const char*>() && !validColor(w["goalColor"]))
        r.errors += "规则7: 非法 goalColor\n";
      // 数据源引用（规则 6；stats 可省略 source = 聚合全部数据源，v1.1）
      const char* src = w["source"] | "";
      if (!strcmp(type, "barChart") || !strcmp(type, "heatMap")) {
        if (!*src) r.errors += String("规则6: ") + type + " 缺 source\n";
        else {
          bool found = false;
          for (const String& s : sourceIds) found |= (s == src);
          if (!found) r.errors += String("规则6: source '") + src + "' 未在 dataSources 声明\n";
        }
      }
      // 字段白名单（规则 8）
      if (!strcmp(type, "stats")) {
        JsonArray fields = w["fields"];
        static const char* kFields[] = {"public_repos", "followers", "stars", "forks"};
        if (!fields || fields.size() == 0)
          r.errors += "规则8: stats.fields 不能为空\n";
        else {
          for (const char* f : fields) {
            bool ok = false;
            for (const char* k : kFields) ok |= !strcmp(f, k);
            if (!ok) r.errors += String("规则8: 字段不在白名单 '") + f + "'\n";
          }
          JsonArray labels = w["labels"];
          if (labels && labels.size() != fields.size())
            r.errors += "规则8: labels 与 fields 数量不一致\n";
        }
      }
      // 摘要
      r.summary += String("  ") + slot + " → " + type + "\n";
    }
  }

  r.ok = r.errors.length() == 0;
  return r;
}

}  // namespace layout
