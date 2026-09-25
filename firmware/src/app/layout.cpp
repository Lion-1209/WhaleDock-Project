#include "layout.h"

#include <vector>

namespace layout {

const char* sampleJson() {
  // 实机在显版式（与模拟器 SAMPLE 一致；v1.2 组件分解 + 用户调优：
  // 标题/鲸鱼错位构图 + 红黑黑红统计条 + 放大日期条）
  return R"LAY({"version":1,"dataSources":[{"id":"gh","type":"github.user","params":{"user":"datawhalechina"}},{"id":"repo","type":"github.repo","params":{"owner":"datawhalechina","repo":"leeml-notes"}}],"resources":[],"layout":{"resolution":[800,480],"widgets":[{"slotRect":{"x":63,"y":14,"w":330,"h":46},"type":"title"},{"slotRect":{"x":89,"y":60,"w":253,"h":196},"type":"image","resBW":"whale_pixel"},{"slotRect":{"x":12,"y":266,"w":330,"h":22},"type":"repo","align":"left"},{"slotRect":{"x":524,"y":14,"w":264,"h":62},"type":"clock","align":"right"},{"slotRect":{"x":456,"y":83,"w":314,"h":45},"type":"date","size":"m","align":"right"},{"slotRect":{"x":524,"y":116,"w":264,"h":176},"type":"calendar"},{"slotRect":{"x":524,"y":296,"w":264,"h":38},"type":"stats","variant":"chips","color":"red","fields":["public_repos"],"labels":["Repositories"]},{"slotRect":{"x":524,"y":338,"w":264,"h":38},"type":"stats","variant":"chips","fields":["stars"],"labels":["Stars"]},{"slotRect":{"x":524,"y":380,"w":264,"h":38},"type":"stats","variant":"chips","color":"black","fields":["forks"],"labels":["Forks"]},{"slotRect":{"x":524,"y":422,"w":264,"h":38},"type":"stats","variant":"chips","color":"red","fields":["followers"],"labels":["Followers"]},{"slotRect":{"x":12,"y":296,"w":500,"h":170},"type":"heatMap","source":"repo","title":"GitHub Contribution"}]}})LAY";
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

  // 槽位（规则 4，v1.2）：slot 可选 = 默认几何提示，不再查重；无 slot 时须给出 slotRect
  static const char* kQuadrants[] = {"tl", "tr", "bl", "br", "ticker"};
  static const char* kTypes[] = {"clock",  "date",     "calendar", "stats",
                                 "barChart", "heatMap", "text",    "image",
                                 "repo",   "title",    "qr",       "ticker"};

  JsonArray widgets = layoutO["widgets"];
  if (!widgets || widgets.size() == 0) {
    r.errors += "规则4: widgets 不能为空\n";
  } else {
    for (JsonObject w : widgets) {
      ++r.widgetCount;
      const char* slot = w["slot"] | "-";
      const char* type = w["type"] | "";
      if (w["slot"].is<const char*>()) {
        bool slotOk = false;
        for (const char* q : kQuadrants) slotOk |= !strcmp(slot, q);
        if (!slotOk) { r.errors += String("规则4: 非法 slot '") + slot + "'\n"; continue; }
      } else if (w["slotRect"].isNull()) {
        r.errors += "规则4: 无 slot 时必须给出 slotRect\n";
        continue;
      }
      // 类型注册（规则 5）
      bool typeOk = false;
      for (const char* t : kTypes) typeOk |= !strcmp(type, t);
      if (!typeOk) { r.errors += String("规则5: 未注册类型 '") + type + "'\n"; continue; }
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
          JsonArray vals = w["values"];
          if (vals && vals.size() != fields.size())
            r.errors += "规则8: values 与 fields 数量不一致\n";
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
