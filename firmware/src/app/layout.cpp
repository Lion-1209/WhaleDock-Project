#include "layout.h"

#include <vector>

namespace layout {

const char* sampleJson() {
  // 概念图 v2（鲸屿形态）复刻版式 —— 与模拟器 SAMPLE 同源（含 whale_logo 220x192 内联资源；
  // glyphs 字段结构校验不展开）。版式：左半屏大鲸鱼 image / 右上时钟+整月历 / 中层左热力图+右统计竖卡 / 右下标语
  return R"LAY({
  "version": 1,
  "dataSources": [
    {
      "id": "gh",
      "type": "github.user",
      "params": {
        "user": "datawhalechina"
      }
    },
    {
      "id": "repo",
      "type": "github.repo",
      "params": {
        "owner": "datawhalechina",
        "repo": "leeml-notes"
      }
    }
  ],
  "resources": [
    {
      "id": "whale_logo",
      "w": 220,
      "h": 192,
      "data": "AAAAAAAAAAAAAAAAAAAAAAAAAD/AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAH/+AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAH//wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD//+AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD///gAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAB///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA////AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAf///wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAH///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD///+AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAB////gAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA////4AAAAAAAAAAAAAAAAAAAAAAAAAAAAPgAAP///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAP/AAH////AAAAAAAAAAAAAAAAAAAAAAAAAAAAH/8AB////gAAAAAAAAAAAAAAAAAAAAAAAAAAAB//gAf///wAAAAAAAAAAAAAAAAAAAAAAAAAAAAf/4AP///4AAAAAAAAAAAAAAAAAAAAAAAAAAAAH//AD///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAB//4A///+AAAAAAAAAAAAAAAAAAAAAAAAAAAAAP/+AP///AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA//gH///AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAB/8B///AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAH/Af//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAfwH//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD8B//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPAf/AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADwH/gAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAcB/gAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAfwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQH4AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAB8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA+AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA4AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAH///gAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA/////4AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAH//////4AAAAAAAAAAAAAAAAAAAAAAAAAAAAAP///////4AAAAAAAAAAAAAAAAAAAAAAAAAAAAf////////wAAAAAAAAAAAAAAAAAAAAAAAAAAA//////////gAAAAAAAAAAAAAAAAAAAAAAAAAB//////////+AAAAAAAAAAAAAAAAAAAAAAAAAB///////////4AAAAAAAAAAAAAAAAAAAAAAAAB////////////gAAAAAAAAAAAAAAAAAAAAAAAB////////////+AAAAAAAAAAAAAAAAAAAAAAAB/////////////wAAAAAAAAAAAAAAAAAAAAAAB//////////////AAAAAAAAAAAAAAAAAAAAAAA//////////////4AAAAAAAAAAAAAAAAAAAAAA///////////////gAAAAAAAAAAAAAAAAAAAAAf//////////////8AAAAAAAAAAAAAAAAAAAAAf///////////////gAAAAAAAAAAAAAAAAAAAAP///////////////8AAAAAAAAAAAAAAAAAAAAH////////////////gAAAAAAAAAAAAAAAAAAAH////////////////8AAAAAAAAAAAAAAAAAAAD/////////////////gAAAAAAAAAAAAAAAAAAB/////////////////8AAAAAAAAAAAAAAAAAAA//////////////////gAAAAAAAAAAAAAAAAAAf/////////////////8AAAAAAAAAAAAAAAAAAP//////////////////gAAAAAAAAAAAAAAAAAH//////////////////8AAAAAAAAAAAAAAAAAD///////////////////gAAAAAAAAAAAAAAAAB///////////////////8AAAAAAAAAAAAAAAAA////////////////////AAAAAAAAAAAAAAAAAf///////////////////4AAAAAAAAAAAAAAAAH////////////////////AAAAAAAAAAAAAAAAD////////////////////wAAAAAAAAAAAAAAAB////////////////////+AAAAAAAAAAAAAAAA/////////////////////wAAAAAAAAAAAAAAAP////////////////////8AAAAAAAAAAAAAAAH/////////////////////gAAAAAAAAAAAAAAD/////////////////////4AAAAAAAAAAAAAAB//////////////////////AAAAAAAAAAAAAAAf/////////////////////4AAAAAAAAAAAAAAP/////////////////////+AAAAAAAAAAAAAAD//////////////////////wAAAAAAAAAAAAAB//////////////////////8AAAAAAAAAAAAAA///////////////////////AAAAAAAAAAAAAAP//////////////////////4AAAAAAAAAAAAAH//////////////////////+AAAAAAAAAAAAAB///////////////////////wAAAAAAAAAAAAA///////////////////////8AAAAAAAAAAAAAf///////////////////////gAAAAAAAAAAAAH///////////////////////4AAAAAACAAAAAD///////////////////////+AAAAAABwAAAAA////////////////////////wAAAAAAeAAAAAf///////////////////////8AAAAAAPwAAAAH////////////////////////AAAAAAH+AAAAD////////////////////////4AAAAAB/wAAAA////////////////////////+AAAAAA/+AAAAf////////////////////////gAAAAAP/gAAAH////////////////////////4AAAAAD/8AAAD/////////////////////////AAAAAB//AAAA/////////////////////////wAAAAAf/wAAAP////////////////////////8AAAAAH/+AAAH/////////////////////////AAAAAD//gAAB/////////////////////////wAAAAA//4AAA/////////////////////////+AAAAAP/+AAAP/////////////////////////gAAAAD//wAAD/////////////////////////4AAAAA//8AAB/////////////////////////+AAAAAP//AAAf/////////////////////////gAAAAH//wAAH/////////////////////////4AAAAB//8AAD//////////////////////////AAAAAf//AAA//////////////////////////wAAAAH//wAAP/////////////////////////8AAAAB//8AAD//////////////////////////AAAAAf//AAB//////////////////////////wAAAAH//wAAf/////////////////////////8AAAAB//8AAH//////////////////////////AAB+Af//AAB//////////////////////////wA//////wAA//////////////////////////8B//////8AAP//////////////////////////B///////AAD//////////////////////////w///////wAA//////////////////////////8P//////8AAf//////////////////////////B///////gAP//////////////////////////wP//////8AH//////////////////////////8B///////wD//////////////////j////////AP///////D//////////////////gP///////wB//////////////////////////wB///////8AP/////////////////////////4AP///////AB/////////////////////////8AB///////wAP/////////////////////////AAf//////8AA/////////////////////////wAH///////AAB////////////////////////8AB///////gAAAfAD/////////////////////AAf//////4AAAAAAf////////////////////wB3//////+AAAAAAD////////////////////+Af///////gAAAAAAf////////////////////wH///////wAAAAAAD////////////////////+A///////8AAAAAAAf/////////////////////////////AAAAAAAH/////////////////////////////wAAAAAAA/////////////////////////////4AAAAAAAH////////////////////////////+AAAAAAAA/////////////////////////////gAAAAAAAH////////////////////////////wAAAAAAAA////////////////////////////8AAAAAAAAH///////////////////////////+AAAAAAAAA////////////////////////////gAAAAAAAAH///////////////////////////wAAAAAAAAA/////////////////////////3/8AAAAAAAAAH////////////////////////B/+AAAAAAAAAA///////////////////////+A//gAAAAAAAAAH//////////////////////8AP/wAAAAAAAAAA//////////////////////wAH/4AAAAAAAAAAH/////////////////////AAD/+AAAAAAAAAAA////////////////////+AAB//AAAAAAAAAAAH///////////////////+AAAf/gAAAAAAAAAAA////////////////////AAAP/4AAAAAAAAAAAH///////////////////gAAH/8AAAAAAAAAAAA///////////////////gAAD/+AAAAAAAAAAAAH//////////////////wAAB//AAAAAAAAAAAAA//////////////////4AAA//wAAAAAAAAAAAAD/////////////////8AAAf/4AAAAAAAAAAAAAf////////////////+AAAP/8AAAAAAAAAAAAAD////////////////+AAAP/+AAAAAAAAAAAAAAf////////////////AAAH//AAAAAAAAAAAAAAD////////////////AAAD//gAAAAAAAAAAAAAAP///////////////gAAD//wAAAAAAAAAAAAAAB///////////////gAAB//4AAAAAAAAAAAAAAAP//////////////gAAA//4AAAAAAAAAAAAAAAB//////////////gAAA//8AAAAAAAAAAAAAAAAH/////////////gAAA//+AAAAAAAAAAAAAAAAA/////////////gAAAf//AAAAAAAAAAAAAAAAAH////////////gAAAf//AAAAAAAAAAAAAAAAAAf///////////AAAAf//gAAAAAAAAAAAAAAAAAD//////////+AAAAf//wAAAAAAAAAAAAAAAAAAP/////////8AAAAf//wAAAAAAAAAAAAAAAAAAB/////////4AAAAf//4AAAAAAAAAAAAAAAAAAAH////////AAAAA///4AAAAAAAAAAAAAAAAAAAA///////8AAAAB///8AAAAAAAAAAAAAAAAAAAAD//////AAAAAD///8AAAAAAAAAAAAAAAAAAAAAP///+AAAAAAP///8AAAAAAAAAAAAAAAAAAAAAA////wAAAAB////8AAAAAAAAAAAAAAAAAAAAAAD////8AAAf////8AAAAAAAAAAAAAAAAAAAAAAAP////////////8AAAAAAAAAAAAAAAAAAAAAAAA////////////8AAAAAAAAAAAAAAAAAAAAAAAAB///////////8AAAAAAAAAAAAAAAAAAAAAAAAAH//////////4AAAAAAAAAAAAAAAAAAAAAAAAAAP/////////4AAAAAAAAAAAAAAAAAAAAAAAAAAAf////////wAAAAAAAAAAAAAAAAAAAAAAAAAAAA////////gAAAAAAAAAAAAAAAAAAAAAAAAAAAAA//////+AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAP////wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAB//4AAAAAAAAAAAAA"
    }
  ],
  "layout": {
    "resolution": [
      800,
      480
    ],
    "widgets": [
      {
        "slot": "tl",
        "slotRect": {
          "x": 12,
          "y": 14,
          "w": 330,
          "h": 264
        },
        "type": "image",
        "resBW": "whale_logo"
      },
      {
        "slot": "tr",
        "slotRect": {
          "x": 354,
          "y": 14,
          "w": 426,
          "h": 264
        },
        "type": "clock",
        "calendar": true,
        "align": "right"
      },
      {
        "slot": "bl",
        "slotRect": {
          "x": 12,
          "y": 282,
          "w": 500,
          "h": 160
        },
        "type": "heatMap",
        "source": "repo",
        "title": "GitHub Contribution"
      },
      {
        "slot": "br",
        "slotRect": {
          "x": 524,
          "y": 282,
          "w": 264,
          "h": 160
        },
        "type": "stats",
        "variant": "chips",
        "fields": [
          "public_repos",
          "stars",
          "forks",
          "followers"
        ],
        "labels": [
          "Repositories",
          "Stars",
          "Forks",
          "Followers"
        ]
      },
      {
        "slot": "ticker",
        "slotRect": {
          "x": 500,
          "y": 448,
          "w": 288,
          "h": 20
        },
        "type": "text",
        "size": "s",
        "align": "right",
        "text": "Keep coding. Keep shipping."
      }
    ]
  }
})LAY";
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
