#include "widgets.h"

#include <Arduino.h>
#include <time.h>

#include "canvas.h"
#include "../drivers/epaper.h"  // GxEPD 颜色常量
#include "../services/github.h"
#include <mbedtls/base64.h>
#include "resources/font5x7.h"
#include "resources/octicons.h"
#include "resources/whale_pixel.h"

namespace widgets {

namespace {

// ---- 槽位几何（协议 §4；与模拟器 SLOTS 一致）----
struct Rect { int x, y, w, h; };
Rect rectOf(JsonObject w) {
  static const struct { const char* slot; Rect r; } kSlots[] = {
      {"tl", {12, 12, 382, 192}},   {"tr", {406, 12, 382, 192}},
      {"bl", {12, 216, 382, 192}},  {"br", {406, 216, 382, 192}},
      {"ticker", {12, 420, 776, 48}},
  };
  if (w["slotRect"].is<JsonObject>()) {
    JsonObject sr = w["slotRect"];
    return {sr["x"] | 0, sr["y"] | 0, sr["w"] | 0, sr["h"] | 0};
  }
  const char* slot = w["slot"] | "tl";
  for (auto& s : kSlots)
    if (!strcmp(s.slot, slot)) return s.r;
  return kSlots[0].r;
}

uint16_t colorOf(const char* name) {
  String c = name ? name : "black";
  if (c == "red") return GxEPD_RED;
  if (c == "yellow") return GxEPD_YELLOW;
  return GxEPD_BLACK;
}

// ---- 文本（GFX 经典 5×7；size 档位与模拟器 s/m/l 近似对齐）----
int sizePx(const char* s) {  // 档位 → GFX size
  String t = s ? s : "m";
  return t == "s" ? 2 : t == "l" ? 3 : 2;
}
// 定宽字体测宽/换行（ASCII；中文留 D3 字库接入）
int strW(const String& s, int size) { return s.length() * 6 * size; }

// 5x7 字形任意缩放绘制：横纵独立（xScale/yScale）。
// 细体（xScale<1.8）= 1px 宽笔画 × ceil(yScale) 高：字高足够、笔画纤细且纵向实心
void drawGlyphText(int x, int y, const String& t, float xScale, float yScale, uint16_t color) {
  auto& d = canvas::get();
  const int penW = xScale >= 1.8f ? (int)(xScale + 0.5f) : 1;
  const int penH = (int)(yScale + 0.9f);
  for (char c : t) {
    if (c < 0x20 || c > 0x7E) c = '?';
    const uint8_t* g = font5x7::DATA[c - 0x20];
    for (int col = 0; col < 5; col++)
      for (int row = 0; row < 8; row++)
        if (g[col] & (1 << row))
          d.fillRect(x + (int)(col * xScale), y + (int)(row * yScale), penW, penH, color);
    x += (int)(6 * xScale);
  }
}
int glyphTextW(const String& t, float scale) { return (int)(t.length() * 6 * scale); }

void drawTextAt(Rect r, const String& text, int gfxSize, const char* alignC,
                const char* colorC) {
  // 号位 → 字形缩放（非整数支持：s≈10px / m≈16px / l≈23px 高）
  const float scale = gfxSize <= 2 ? 1.0f : gfxSize == 3 ? 2.2f : 3.2f;  // s 档横 1.0：整列实心，杜绝点画
  // 细体（s 档）：横 1.4 纵 2.0 —— 高瘦实心笔画；m/l 等比
  const float yScale = gfxSize <= 2 ? 2.0f : scale;
  const uint16_t color = colorOf(colorC);
  const String align = alignC ? alignC : "center";
  const int maxW = r.w + 24;  // 轻微溢出借位（右下标语向左空带延伸，防不必要换行）
  // 按宽换行（单行尽量不拆）
  std::vector<String> lines;
  String cur;
  for (char c : text) {
    if (c == 10 || glyphTextW(cur + c, scale) > maxW) {
      lines.push_back(cur);
      cur = (c == 10) ? "" : String(c);
    } else {
      cur += c;
    }
  }
  if (cur.length()) lines.push_back(cur);
  const int lineH = (int)(8 * scale) + 2;
  int y = r.y + (r.h - (int)lines.size() * lineH) / 2 + 2;
  for (const String& ln : lines) {
    const int tw = glyphTextW(ln, scale);
    int x = r.x + 6;
    if (align == "right") x = r.x + r.w - 6 - tw;
    else if (align == "center") x = r.x + (r.w - tw) / 2;
    drawGlyphText(x, y, ln, scale, yScale, color);
    y += lineH;
  }
}

// 5×7 点阵字模（模拟器 GLYPH5x7 同源，概念图点阵数字风）
const char* kGlyph5x7[11][7] = {
    {"01110", "10001", "10011", "10101", "11001", "10001", "01110"},  // 0
    {"00100", "01100", "00100", "00100", "00100", "00100", "01110"},  // 1
    {"01110", "10001", "00001", "00110", "01100", "11000", "11111"},  // 2
    {"11111", "00010", "00100", "00010", "00001", "10001", "01110"},  // 3
    {"00010", "00110", "01010", "10010", "11111", "00010", "00010"},  // 4
    {"11111", "10000", "11110", "00001", "00001", "10001", "01110"},  // 5
    {"00110", "01000", "10000", "11110", "10001", "10001", "01110"},  // 6
    {"11111", "00001", "00010", "00100", "01000", "01000", "01000"},  // 7
    {"01110", "10001", "10001", "01110", "10001", "10001", "01110"},  // 8
    {"01110", "10001", "10001", "01111", "00001", "00010", "01100"},  // 9
    {"00", "10", "00", "00", "00", "10", "00"},                       // :
};
int glyphIdx(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  return 10;  // ':'
}
void drawPixelDigits(const String& s, int x, int y, int px, int gap) {
  auto& d = canvas::get();
  d.setTextColor(GxEPD_BLACK);
  for (char c : s) {
    const char** g = kGlyph5x7[glyphIdx(c)];
    const int w = strlen(g[0]);
    for (int gy = 0; gy < 7; gy++)
      for (int gx = 0; gx < w; gx++)
        if (g[gy][gx] == '1') d.fillRect(x + gx * px, y + gy * px, px - 1, px - 1, GxEPD_BLACK);
    x += w * px + gap;
  }
}
int pixelDigitsW(const String& s, int px, int gap) {
  int w = 0;
  for (char c : s) w += strlen(kGlyph5x7[glyphIdx(c)][0]) * px + gap;
  return w - gap;
}

// ---- 数据字段（stats 聚合跨源，与模拟器 fieldOf 一致）----
String fmtK(long v) {
  if (v >= 1000) {
    String s = String(v / 1000.0, 1);
    if (s.endsWith(".0")) s.remove(s.length() - 2);
    return s + "k";
  }
  return String(v);
}
long fieldOf(const char* f) {
  github::UserStats u;
  if (github::loadCachedUser(u)) {
    if (!strcmp(f, "public_repos")) return u.publicRepos;
    if (!strcmp(f, "followers")) return u.followers;
  }
  github::RepoStats r;
  if (github::loadCachedRepo(r)) {
    if (!strcmp(f, "stars")) return r.stars;
    if (!strcmp(f, "forks")) return r.forks;
  }
  return -1;  // 无数据 → "—"
}

// ---- Widget 绘制器 ----
void drawClock(Rect r, JsonObject w) {
  auto& d = canvas::get();
  const time_t now = time(nullptr);
  struct tm tmv;
  localtime_r(&now, &tmv);
  const String align = w["align"] | "center";

  // 点阵时间（px=9 gap=7，坐标与模拟器一致：ty=r.y+16）
  char tb[8];
  snprintf(tb, sizeof(tb), "%02d:%02d", tmv.tm_hour, tmv.tm_min);
  const int px = 9, gap = 7;
  const int tw = pixelDigitsW(tb, px, gap);
  int tx = r.x + (r.w - tw) / 2;
  if (align == "right") tx = r.x + r.w - 16 - tw;
  if (align == "left") tx = r.x + 16;
  drawPixelDigits(tb, tx, r.y + 16, px, gap);

  // 日期行（模拟器：基线 r.y+102，避点阵数字下缘 ~79）
  static const char* kWd[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  char db[24];
  snprintf(db, sizeof(db), "%04d-%02d-%02d %s", tmv.tm_year + 1900,
           tmv.tm_mon + 1, tmv.tm_mday, kWd[tmv.tm_wday]);
  d.setTextColor(GxEPD_BLACK);
  d.setTextSize(2);
  int dx = r.x + (r.w - strW(db, 2)) / 2;
  if (align == "right") dx = r.x + r.w - 16 - strW(db, 2);
  if (align == "left") dx = r.x + 16;
  d.setCursor(dx, r.y + 88);  // 模拟器基线 y+102 的顶线等效（size2 高 14）
  d.print(db);

  if (!w["calendar"] | false) return;
  // 日历：矮槽（<220px，概念图上层右块）渲染"周日历条"——表头 Su..Sa + 本周 7 天，
  // 今日红圈（概念图原始措辞即"周日历红圈当日"；整月历与时钟在 148px 内必然重叠，
  // 只有高槽才装得下整月）
  if (r.h < 220) {
    const int cw2 = 40, ch2 = 24;
    const int gw2 = cw2 * 7;
    int bx2 = r.x + (r.w - gw2) / 2;
    if (align == "right") bx2 = r.x + r.w - 16 - gw2;
    const int by2 = r.y + r.h - 8 - 2 * ch2;
    static const char* kHead2[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    for (int i = 0; i < 7; i++) {
      const String h = kHead2[i];
      d.setTextColor(GxEPD_BLACK);
      d.setTextSize(1);
      d.setCursor(bx2 + i * cw2 + (cw2 - strW(h, 1)) / 2, by2 + 3);
      d.print(h);
    }
    // 本周 = 以今天为锚，回退到本周日
    struct tm week0 = tmv;
    week0.tm_mday -= tmv.tm_wday;
    mktime(&week0);
    for (int i = 0; i < 7; i++) {
      struct tm day = week0;
      day.tm_mday += i;
      mktime(&day);
      const int cx = bx2 + i * cw2 + cw2 / 2, cy = by2 + ch2 + ch2 / 2;
      const String ds = String(day.tm_mday);
      const bool today = (day.tm_mday == tmv.tm_mday);
      d.setTextColor(GxEPD_BLACK);
      d.setTextSize(2);
      d.setCursor(cx - strW(ds, 2) / 2, cy - 7);
      d.print(ds);
      if (today) d.drawCircle(cx, cy, 13, GxEPD_RED);
    }
    return;
  }
  // 高槽：整月日历（cw=40 ch=24 贴槽底，今日红圈）
  const int cw = 40, ch = 24;
  struct tm first = tmv;
  first.tm_mday = 1;
  mktime(&first);
  struct tm last = first;
  last.tm_mon += 1;
  last.tm_mday = 0;
  const int days = mktime(&last) != (time_t)-1 ? last.tm_mday : 30;
  const int rows = (first.tm_wday + days + 6) / 7;
  const int gw = cw * 7, gh = ch * (rows + 1);
  int bx = r.x + (r.w - gw) / 2;
  if (align == "right") bx = r.x + r.w - 16 - gw;
  const int by = r.y + r.h - 12 - gh;
  static const char* kHead[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
  for (int i = 0; i < 7; i++) {
    const String h = kHead[i];
    d.setTextColor(GxEPD_BLACK);
    d.setTextSize(1);
    d.setCursor(bx + i * cw + (cw - strW(h, 1)) / 2, by + 4);
    d.print(h);
  }
  for (int i = 1; i <= days; i++) {
    const int col = (first.tm_wday + i - 1) % 7, row = (first.tm_wday + i - 1) / 7;
    const int cx = bx + col * cw + cw / 2, cy = by + ch + row * ch + ch / 2;
    const String day = String(i);
    const bool today = (i == tmv.tm_mday);
    d.setTextColor(GxEPD_BLACK);
    d.setTextSize(2);
    d.setCursor(cx - strW(day, 2) / 2, cy - 7);
    d.print(day);
    if (today) d.drawCircle(cx, cy, 14, GxEPD_RED);
  }
}

// 统计图标：GitHub Octicons 22px 栅格化位图（与模拟器 drawIcon 同源同缩放）
void drawStatIcon(int cx, int cy, int kind, uint16_t c) {
  auto& d = canvas::get();
  const uint8_t* icon = octicons::of(kind);
  const int ox = cx - octicons::SIZE / 2, oy = cy - octicons::SIZE / 2;
  for (int y = 0; y < octicons::SIZE; y++)
    for (int x = 0; x < octicons::SIZE; x++)
      if (icon[y * octicons::BYTES + (x >> 3)] & (0x80 >> (x & 7)))
        d.fillRect(ox + x, oy + y, 1, 1, c);
}

void drawStats(Rect r, JsonObject w) {
  auto& d = canvas::get();
  JsonArray fields = w["fields"];
  JsonArray labels = w["labels"];
  const int n = fields.size();
  // 竖卡（模拟器规则：h > w*0.55 且 ≥3 项）——概念图右下形态
  if (r.h > r.w * 0.55f && n >= 3) {
    // 数据源标识行（用户 · 仓库，来自缓存；跨 user/repo 两源聚合时必读）
    {
      String srcLine;
      github::UserStats u;
      if (github::loadCachedUser(u)) srcLine = u.login;
      github::RepoStats rp;
      if (github::loadCachedRepo(rp) && rp.fullName.length()) {
        // fullName = "owner/repo"，去掉与用户名重复的 owner 前缀
        String repo = rp.fullName.substring(rp.fullName.indexOf('/') + 1);
        srcLine += " / " + repo;
      }
      if (srcLine.length())
        drawGlyphText(r.x + 4, r.y + 2, srcLine, 1.0f, 1.0f, GxEPD_BLACK);
    }
    Rect cards = {r.x, r.y + 14, r.w, r.h - 14};  // 卡片区让出标识行
    r = cards;
    const int rh = (r.h - 8 * (n - 1)) / n;
    for (int i = 0; i < n; i++) {
      const int y = r.y + i * (rh + 8);
      d.drawRect(r.x + 2, y, r.w - 4, rh, GxEPD_BLACK);
      const bool red = (i == 0 || i == n - 1);
      const uint16_t vc = red ? GxEPD_RED : GxEPD_BLACK;
      drawStatIcon(r.x + 20, y + rh / 2, i, vc);
      String label = labels ? (labels[i] | String((const char*)fields[i])) : String((const char*)fields[i]);
      d.setTextColor(GxEPD_BLACK);
      d.setTextSize(2);
      d.setCursor(r.x + 46, y + rh / 2 - 7);
      d.print(label);
      const long v = fieldOf(fields[i]);
      const String vs = v < 0 ? "-" : fmtK(v);
      d.setTextColor(vc);
      d.setTextSize(3);
      d.setCursor(r.x + r.w - 12 - strW(vs, 3), y + rh / 2 - 11);
      d.print(vs);
    }
    return;
  }
  // 横排 chips
  const int gap = 12;
  const int cw = (r.w - gap * (n - 1)) / n;
  const int y = r.y + 4, h = r.h - 8;
  for (int i = 0; i < n; i++) {
    const int x = r.x + i * (cw + gap);
    d.drawRect(x, y, cw, h, GxEPD_BLACK);
    const bool red = (i == 0 || i == n - 1);
    const uint16_t vc = red ? GxEPD_RED : GxEPD_BLACK;
    d.fillRect(x + 10, y + h / 2 - 4, 8, 8, vc);
    String label = labels ? (labels[i] | String((const char*)fields[i])) : String((const char*)fields[i]);
    d.setTextColor(GxEPD_BLACK);
    d.setTextSize(1);
    d.setCursor(x + 24, y + 8);
    d.print(label);
    const long v = fieldOf(fields[i]);
    const String vs = v < 0 ? "-" : fmtK(v);
    d.setTextColor(vc);
    d.setTextSize(3);
    d.setCursor(x + cw - 10 - strW(vs, 3), y + h - 30);
    d.print(vs);
  }
}

// 26 周 × 7 天演示矩阵：与模拟器同源（LCG 种子 20260520，确定性）。
// 按日贡献需 GraphQL（二期）；一期与模拟器恒用演示数据口径一致
uint8_t heatDemoAt(int wi, int di) {
  static uint8_t grid[26][7];
  static bool built = false;
  if (!built) {
    uint32_t seed = 20260520;
    for (int w = 0; w < 26; w++)
      for (int dd = 0; dd < 7; dd++) {
        seed = (seed * 1103515245u + 12345u) % 2147483648u;
        const float rnd = (float)((uint64_t)seed * 1000000ull / 2147483648u) / 1000000.0f;
        const bool weekend = w % 7 >= 5;
        uint8_t lv;
        if (rnd < (weekend ? 0.55f : 0.25f)) lv = 0;
        else if (rnd < (weekend ? 0.80f : 0.55f)) lv = 1;
        else if (rnd < (weekend ? 0.93f : 0.78f)) lv = 2;
        else lv = rnd < 0.94f ? 3 : 4;
        grid[w][dd] = lv;
      }
    built = true;
  }
  return grid[wi][di];
}

void drawHeatMap(Rect r, JsonObject w) {
  auto& d = canvas::get();
  // 上缘分割线（概念图关键细节）
  d.fillRect(r.x, r.y + 1, r.w, 2, GxEPD_BLACK);
  const String title = w["title"] | "GitHub Contribution";
  d.setTextColor(GxEPD_BLACK);
  d.setTextSize(2);
  d.setCursor(r.x + 4, r.y + 8);
  d.print(title);
  const int weeks = 26;
  // 方格边长按槽宽定（模拟器同款）；真实版式 bl=500x160 → cw=18、格阵 468x126
  const int cw = (r.w - 8) / weeks;
  const int cell = cw - 2;
  const int gx = r.x + 4, gy = r.y + 38;
  for (int wi = 0; wi < weeks; wi++)
    for (int di = 0; di < 7; di++) {
      const int lv = heatDemoAt(wi, di);
      const int x = gx + wi * cw, y = gy + di * cw;
      if (lv == 0) d.drawRect(x, y, cell, cell, GxEPD_BLACK);
      else if (lv == 4) d.fillRect(x, y, cell, cell, GxEPD_RED);
      else d.fillRect(x, y, cell, cell, GxEPD_BLACK);
    }
  // 月份轴（月份变化列标注）
  static const char* kMon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  const time_t now = time(nullptr);
  struct tm tv;
  int lastM = -1;
  d.setTextSize(1);
  for (int wi = 0; wi < weeks; wi++) {
    time_t t = now - (time_t)(weeks - 1 - wi) * 7 * 86400;
    localtime_r(&t, &tv);
    if (tv.tm_mon != lastM && wi > 0) {
      lastM = tv.tm_mon;
      d.setTextColor(GxEPD_BLACK);
      d.setCursor(gx + wi * cw, r.y + 28);
      d.print(kMon[tv.tm_mon]);
    }
  }
  // Less..More 图例
  const int ly = gy + 7 * cw + 4;
  d.setTextColor(GxEPD_BLACK);
  d.setTextSize(1);
  d.setCursor(gx, ly + 1);
  d.print("Less");
  int lx = gx + 30;
  d.drawRect(lx, ly + 2, cell, cell, GxEPD_BLACK);
  d.fillRect(lx + cw, ly + 2, cell, cell, GxEPD_BLACK);
  d.fillRect(lx + 2 * cw, ly + 2, cell, cell, GxEPD_RED);
  d.setCursor(lx + 3 * cw + 6, ly + 1);
  d.print("More");
}

void drawBarChart(Rect r, JsonObject w) {
  auto& d = canvas::get();
  github::CommitActivity c;
  const bool has = github::loadCachedCommits(c);
  const String title = w["title"] | "Commits";
  d.setTextColor(GxEPD_BLACK);
  d.setTextSize(2);
  d.setCursor(r.x + 12, r.y + 10);
  d.print(title);
  if (!has || c.weeklyTotals.empty()) return;
  const int weeks = w["weeks"] | 12;
  const int n = weeks < (int)c.weeklyTotals.size() ? weeks : c.weeklyTotals.size();
  long maxV = 1;
  for (int i = 0; i < n; i++) maxV = c.weeklyTotals[c.weeklyTotals.size() - n + i] > maxV ? c.weeklyTotals[c.weeklyTotals.size() - n + i] : maxV;
  const int top = r.y + 34, bot = r.y + r.h - 26;
  const int areaH = bot - top;
  const float bw = (float)(r.w - 24) / n;
  for (int i = 0; i < n; i++) {
    const long v = c.weeklyTotals[c.weeklyTotals.size() - n + i];
    const int h = 3 + (int)((float)v / maxV * areaH);
    d.fillRect(r.x + 12 + (int)(i * bw) + 2, bot - h, (int)bw - 4, h, GxEPD_BLACK);
  }
  // 目标线（黄，虚线示意为断续段）
  if (w["goal"] | 0) {
    const int goal = w["goal"];
    const int gy = bot - (int)((float)goal / maxV * areaH);
    for (int x = r.x + 12; x < r.x + r.w - 12; x += 10)
      d.fillRect(x, gy, 6, 2, colorOf(w["goalColor"] | "yellow"));
  }
  d.setTextSize(1);
  d.setCursor(r.x + 12, bot + 6);
  d.print("W-" + String(n + 1));
}

void drawText(Rect r, JsonObject w) {
  drawTextAt(r, w["text"] | "", sizePx(w["size"] | "m"), w["align"] | "center",
             w["color"] | "black");
}

// 1bpp 位图按槽适配缩放绘制（目标驱动最近邻采样，任意比例无空隙）。
// scale 上限 8（模拟器同规则，防止小图放成马赛克巨画）
void drawBitmapResource(Rect r, const uint8_t* bits, int bw, int bh, uint16_t color = GxEPD_BLACK, int xBias = 0) {
  auto& d = canvas::get();
  const int stride = (bw + 7) / 8;
  float scale = (float)(r.w - 16) / bw < (float)(r.h - 16) / bh
                    ? (float)(r.w - 16) / bw
                    : (float)(r.h - 16) / bh;
  if (scale > 8) scale = 8;
  const int dw = (int)(bw * scale), dh = (int)(bh * scale);
  const int ox = r.x + (r.w - dw) / 2 + xBias, oy = r.y + (r.h - dh) / 2;
  for (int y = 0; y < dh; y++) {
    const int sy = (int)(y / scale);
    for (int x = 0; x < dw; x++) {
      const int sx = (int)(x / scale);
      if (bits[sy * stride + (sx >> 3)] & (0x80 >> (sx & 7)))
        d.fillRect(ox + x, oy + y, 1, 1, color);
    }
  }
}

void drawPet(Rect r, JsonObject) {
  drawBitmapResource(r, whale_pixel::BITS, whale_pixel::W, whale_pixel::H);
}

// image：resources 内联 1bpp 位图。resBW（黑）必有；resR/resY 可选，各自独立
// 资源与尺寸上红/黄平面；资源缺失/尺寸不符回退内置鲸鱼
void drawImageRes(Rect r, JsonObject w, JsonDocument& doc) {
  auto findRes = [&](const char* id) -> JsonObject {
    for (JsonObject res : doc["resources"].as<JsonArray>())
      if (id && !strcmp(res["id"] | "", id)) return res;
    return JsonObject();
  };
  // 解码一个资源到 heap（失败返回 nullptr）
  auto decode = [&](JsonObject res, int& ow, int& oh) -> uint8_t* {
    if (res.isNull()) return nullptr;
    ow = res["w"] | 0;
    oh = res["h"] | 0;
    const String b64 = res["data"] | "";
    const int stride = (ow + 7) / 8;
    const size_t expect = (size_t)stride * oh;
    if (ow <= 0 || oh <= 0 || !expect) return nullptr;
    uint8_t* buf = (uint8_t*)malloc(expect);
    size_t got = 0;
    if (!buf || mbedtls_base64_decode(buf, expect, &got,
                                      (const uint8_t*)b64.c_str(), b64.length()) != 0 ||
        got != expect) {
      free(buf);
      return nullptr;
    }
    return buf;
  };
  int rw = 0, rh = 0;
  uint8_t* bwBits = decode(findRes(w["resBW"] | ""), rw, rh);
  if (!bwBits) {
    drawBitmapResource(r, whale_pixel::BITS, whale_pixel::W, whale_pixel::H);
    return;
  }
  drawBitmapResource(r, bwBits, rw, rh, GxEPD_BLACK, 8);  // 视觉重心右移（位图右侧喷水留白偏多）
  free(bwBits);
  static const char* kColorKeys[2] = {"resR", "resY"};
  static const uint16_t kColors[2] = {GxEPD_RED, GxEPD_YELLOW};
  for (int i = 0; i < 2; i++) {
    int cw2 = 0, ch2 = 0;
    uint8_t* bits = decode(findRes(w[kColorKeys[i]] | ""), cw2, ch2);
    if (bits) {
      drawBitmapResource(r, bits, cw2, ch2, kColors[i], 8);
      free(bits);
    }
  }
}

void drawTicker(Rect r, JsonObject w) {
  drawTextAt(r, w["text"] | "", 3, "center", w["color"] | "black");
}

void drawQr(Rect r, JsonObject) {  // 占位（B3 接生成库）
  auto& d = canvas::get();
  const int size = r.w < r.h ? r.w - 16 : r.h - 16;
  d.drawRect(r.x + (r.w - size) / 2, r.y + (r.h - size) / 2, size, size, GxEPD_BLACK);
  drawTextAt(r, "QR (pending)", 1, "center", "black");
}

}  // namespace

bool render(const String& layoutJson) {
  JsonDocument doc;
  if (deserializeJson(doc, layoutJson)) return false;
  JsonObject layoutO = doc["layout"];
  if (layoutO.isNull()) return false;
  auto& d = canvas::get();
  if (!d.begin()) return false;  // 三平面 PSRAM 分配（漏分配则 flush 直接跳过）
  d.fillScreen(GxEPD_WHITE);
  for (JsonObject w : layoutO["widgets"].as<JsonArray>()) {
    const Rect r = rectOf(w);
    const String type = w["type"] | "";
    if (type == "clock") drawClock(r, w);
    else if (type == "stats") drawStats(r, w);
    else if (type == "barChart") drawBarChart(r, w);
    else if (type == "heatMap") drawHeatMap(r, w);
    else if (type == "text") drawText(r, w);
    else if (type == "pet") drawPet(r, w);
    else if (type == "ticker") drawTicker(r, w);
    else if (type == "qr") drawQr(r, w);
    else if (type == "image") drawImageRes(r, w, doc);
  }
  return true;
}

}  // namespace widgets
