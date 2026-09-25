// 鲸屿 WhaleDock · 显示协议 v1.2 模拟器（零依赖静态页）
// 校验规则与 docs/显示协议-v1.md §9、firmware/src/app/layout.cpp 同源；
// 默认示例 = 实机 C2 版式的组件分解版（v1.2：slot 可选、clock 拆分、新增 date/calendar/repo/title）。
// 三方（固件/编辑器/模拟器）规则改动必须同步这三处。

const $ = (id) => document.getElementById(id);

// ---- 协议 §3 槽位几何（与固件一致；v1.2 起 slot 仅为默认几何提示，不再查重） ----
const SLOTS = {
  tl:     { x: 12,  y: 12,  w: 382, h: 192 },
  tr:     { x: 406, y: 12,  w: 382, h: 192 },
  bl:     { x: 12,  y: 216, w: 382, h: 192 },
  br:     { x: 406, y: 216, w: 382, h: 192 },
  ticker: { x: 12,  y: 420, w: 776, h: 48 },
};

const COL = { black: '#1c1c1c', red: '#b3382c', yellow: '#d4a017' };
const PAPER = '#f5f4ef';
const TYPES = ['clock', 'date', 'calendar', 'stats', 'barChart', 'text', 'image',
               'repo', 'title', 'qr', 'ticker', 'heatMap'];
const FIELDS = ['public_repos', 'followers', 'stars', 'forks'];

// ---- 默认示例：实机在显版式（v1.2，用户调优：标题/鲸鱼错位构图 + 红黑黑红统计条 + 放大日期条）----
// v1.2 起 slot 可省略（有 slotRect 即可）；鲸鱼 = datawhalelogo.png 转 1bpp 位图资源（协议 image 类型）
const SAMPLE = `{
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
  "resources": [],
  "layout": {
    "resolution": [800, 480],
    "widgets": [
      { "slotRect": { "x": 63, "y": 14, "w": 330, "h": 46 }, "type": "title" },
      { "slotRect": { "x": 89, "y": 60, "w": 253, "h": 196 }, "type": "image", "resBW": "whale_pixel" },
      { "slotRect": { "x": 12, "y": 266, "w": 330, "h": 22 }, "type": "repo", "align": "left" },
      { "slotRect": { "x": 524, "y": 14, "w": 264, "h": 62 }, "type": "clock", "align": "right" },
      { "slotRect": { "x": 456, "y": 83, "w": 314, "h": 45 }, "type": "date", "align": "right", "size": "m" },
      { "slotRect": { "x": 524, "y": 116, "w": 264, "h": 176 }, "type": "calendar" },
      { "slotRect": { "x": 524, "y": 296, "w": 264, "h": 38 }, "type": "stats", "color": "red", "variant": "chips", "fields": ["public_repos"], "labels": ["Repositories"] },
      { "slotRect": { "x": 524, "y": 338, "w": 264, "h": 38 }, "type": "stats", "variant": "chips", "fields": ["stars"], "labels": ["Stars"] },
      { "slotRect": { "x": 524, "y": 380, "w": 264, "h": 38 }, "type": "stats", "color": "black", "variant": "chips", "fields": ["forks"], "labels": ["Forks"] },
      { "slotRect": { "x": 524, "y": 422, "w": 264, "h": 38 }, "type": "stats", "color": "red", "variant": "chips", "fields": ["followers"], "labels": ["Followers"] },
      { "slotRect": { "x": 12, "y": 296, "w": 500, "h": 170 }, "type": "heatMap", "source": "repo", "title": "GitHub Contribution" }
    ]
  }
}`;

// ---- 演示数据（内置快照；联网按钮会覆盖 user/repo；热力图需 GraphQL，恒用演示） ----
const demoData = {
  gh:   { login: 'datawhalechina', public_repos: 42, followers: 316 },
  repo: { fullName: 'datawhalechina/leeml-notes', stars: 1240, forks: 358,
          participation: [18, 25, 31, 22, 40, 35, 28, 33, 45, 38, 52, 47] },
};
// 26 周 × 7 天热力图演示矩阵（0-4 级；确定性伪随机，刷新不变）
const heatDemo = (() => {
  let seed = 20260520;
  const rnd = () => (seed = (seed * 1103515245 + 12345) % 2147483648) / 2147483648;
  return Array.from({ length: 26 }, (_, w) =>
    Array.from({ length: 7 }, () => {
      const r = rnd();
      const weekend = w % 7 >= 5;
      if (r < (weekend ? 0.55 : 0.25)) return 0;
      if (r < (weekend ? 0.8 : 0.55)) return 1;
      if (r < (weekend ? 0.93 : 0.78)) return 2;
      return r < 0.94 ? 3 : 4;
    }));
})();

let data = { ...demoData };

// ---- 校验（协议 §9 规则 1-10 + v1.1 追加；规则 11 为 token 优先级，非静态可判） ----
function validate(doc) {
  const errors = [];
  if (doc.version !== 1) errors.push('规则1: version 必须为 1');
  const hasLayout = doc.layout && typeof doc.layout === 'object';
  const hasDisplay = doc.display && typeof doc.display === 'object';
  if (hasLayout === hasDisplay) errors.push('规则2: layout 与 display 必须恰有一个');
  if (hasDisplay) {
    for (const plane of ['bitmapBW', 'bitmapRed', 'bitmapYellow']) {
      const b64 = doc.display[plane];
      if (typeof b64 !== 'string' || !b64) continue;
      const bytes = Math.floor(b64.length / 4) * 3 - (b64.endsWith('==') ? 2 : b64.endsWith('=') ? 1 : 0);
      if (bytes !== 48000) errors.push(`规则9: ${plane} 解码 ${bytes} B，应为 48000 B`);
    }
    return { errors, mode: 'bitmap', widgets: [] };
  }
  if (!hasLayout) return { errors, mode: 'none', widgets: [] };

  const res = doc.layout.resolution;
  if (!Array.isArray(res) || res[0] !== 800 || res[1] !== 480)
    errors.push('规则3: resolution 必须为 [800,480]');

  const sourceIds = (doc.dataSources || []).map((d) => d.id);
  const widgets = doc.layout.widgets || [];
  if (!widgets.length) errors.push('规则4: widgets 不能为空');

  for (const w of widgets) {
    // 规则 4（v1.2）：slot 可选（= 默认几何提示，不再查重）；无 slot 时必须给出 slotRect
    const hasSlot = w.slot !== undefined;
    if (hasSlot && !SLOTS[w.slot]) { errors.push(`规则4: 非法 slot '${w.slot}'`); continue; }
    if (!hasSlot && !w.slotRect) { errors.push('规则4: 无 slot 时必须给出 slotRect'); continue; }
    if (!TYPES.includes(w.type)) { errors.push(`规则5: 未注册类型 '${w.type}'`); continue; }
    // v1.1：slotRect 边界
    if (w.slotRect) {
      const r = w.slotRect;
      const ok = [r.x, r.y, r.w, r.h].every((v) => Number.isInteger(v)) &&
                 r.x >= 0 && r.y >= 0 && r.w > 0 && r.h > 0 &&
                 r.x + r.w <= 800 && r.y + r.h <= 480;
      if (!ok) errors.push(`规则12: slotRect 越界或非法（'${w.slot}'）`);
    }
    if (w.color !== undefined) {
      if (w.type === 'qr') errors.push('规则7: qr 恒黑，不允许 color');
      else if (!COL[w.color]) errors.push(`规则7: 非法颜色 '${w.color}'`);
    }
    if (w.goalColor !== undefined && !COL[w.goalColor]) errors.push('规则7: 非法 goalColor');
    if (['barChart', 'heatMap'].includes(w.type)) {
      if (!w.source) errors.push(`规则6: ${w.type} 缺 source`);
      else if (!sourceIds.includes(w.source)) errors.push(`规则6: source '${w.source}' 未声明`);
    }
    if (w.type === 'stats' && w.source && !sourceIds.includes(w.source))
      errors.push(`规则6: source '${w.source}' 未声明`);  // stats 可省略 source = 聚合全部数据源（v1.1）
    if (w.type === 'stats') {
      const f = w.fields || [];
      if (!f.length) errors.push('规则8: stats.fields 不能为空');
      f.forEach((x) => { if (!FIELDS.includes(x)) errors.push(`规则8: 字段不在白名单 '${x}'`); });
      if (w.labels && w.labels.length !== f.length)
        errors.push('规则8: labels 与 fields 数量不一致');
      if (w.values && w.values.length !== f.length)
        errors.push('规则8: values 与 fields 数量不一致');
      if (w.variant !== undefined && !['list', 'chips'].includes(w.variant))
        errors.push('规则7: stats.variant 仅 list|chips');
    }
    if (w.align !== undefined && !['left', 'center', 'right'].includes(w.align))
      errors.push('规则7: align 仅 left|center|right');
  }
  return { errors, mode: 'layout', widgets, sources: sourceIds };
}

// ---- 渲染 ----
const cv = $('screen'), ctx = cv.getContext('2d');

function font(px, bold = false, mono = false) {
  const fam = mono ? '"Cascadia Mono", Consolas, monospace'
                   : '"Microsoft YaHei", "Segoe UI", sans-serif';
  return `${bold ? '600 ' : ''}${px}px ${fam}`;
}
function rectOf(w) {
  const base = SLOTS[w.slot];
  return w.slotRect ? { ...w.slotRect } : { ...base };
}
function fmtK(v) {
  if (v == null) return '—';
  return v >= 1000 ? `${(v / 1000).toFixed(1).replace(/\.0$/, '')}k` : String(v);
}

function drawGuides() {
  ctx.save();
  ctx.setLineDash([4, 4]);
  ctx.strokeStyle = 'rgba(28,28,28,0.28)';
  for (const [name, r] of Object.entries(SLOTS)) {
    ctx.strokeRect(r.x, r.y, r.w, r.h);
    ctx.fillStyle = 'rgba(28,28,28,0.5)';
    ctx.font = font(11); ctx.textAlign = 'left'; ctx.textBaseline = 'top';
    ctx.fillText(`${name} ${r.w}×${r.h}`, r.x + 4, r.y + 3);
  }
  ctx.restore();
}

// 5×7 点阵字模（概念图时钟数字风：粗实体、微斜切）
const GLYPH5x7 = {
  '0': ['01110', '10001', '10011', '10101', '11001', '10001', '01110'],
  '1': ['00100', '01100', '00100', '00100', '00100', '00100', '01110'],
  '2': ['01110', '10001', '00001', '00110', '01100', '11000', '11111'],
  '3': ['11111', '00010', '00100', '00010', '00001', '10001', '01110'],
  '4': ['00010', '00110', '01010', '10010', '11111', '00010', '00010'],
  '5': ['11111', '10000', '11110', '00001', '00001', '10001', '01110'],
  '6': ['00110', '01000', '10000', '11110', '10001', '10001', '01110'],
  '7': ['11111', '00001', '00010', '00100', '01000', '01000', '01000'],
  '8': ['01110', '10001', '10001', '01110', '10001', '10001', '01110'],
  '9': ['01110', '10001', '10001', '01111', '00001', '00010', '01100'],
  ':': ['00', '10', '00', '00', '00', '10', '00'],
};
// 时钟（v1.2 拆分后 = 纯点阵时间数字；日期/日历独立为 date/calendar 组件）
function drawClock(r, w) {
  const now = new Date();
  const align = w.align || 'center';
  const px = 9, gap = 7;
  const timeStr = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`;
  const totalW = [...timeStr].reduce((a, ch) => a + GLYPH5x7[ch][0].length, 0) * px + (timeStr.length - 1) * gap;
  let tx = align === 'right' ? r.x + r.w - 16 - totalW : align === 'left' ? r.x + 16 : r.x + (r.w - totalW) / 2;
  const ty = r.y + (r.h - 7 * px) / 2;  // 槽内垂直居中（拆分后不再固定贴顶）
  ctx.fillStyle = COL.black;
  for (const ch of timeStr) {
    GLYPH5x7[ch].forEach((row, y) => [...row].forEach((v, x) => {
      if (v === '1') ctx.fillRect(tx + x * px, ty + y * px, px - 1, px - 1);
    }));
    tx += GLYPH5x7[ch][0].length * px + gap;
  }
}

// 日期：YYYY-MM-DD 周几（原 clock 日期行独立成组件）
function drawDate(r, w) {
  const now = new Date();
  const wd = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'][now.getDay()];
  const s = `${now.getFullYear()}-${String(now.getMonth() + 1).padStart(2, '0')}-${String(now.getDate()).padStart(2, '0')} ${wd}`;
  const size = { s: 15, m: 20, l: 26 }[w.size || 'm'];
  ctx.fillStyle = COL[w.color || 'black'];
  ctx.font = font(size);
  ctx.textAlign = w.align || 'center';
  ctx.textBaseline = 'middle';
  ctx.fillText(s, w.align === 'right' ? r.x + r.w - 6 : w.align === 'left' ? r.x + 6 : r.x + r.w / 2,
               r.y + r.h / 2);
}

// 日历：槽高 ≥150px 渲染整月阵（Su–Sa 表头 + 当月日期，今日红圈），矮槽 = 周日历条
// （表头 + 本周 7 天）。列宽按槽宽自适应（窄槽不再溢出），贴槽底排布。
function drawCalendar(r) {
  const now = new Date();
  const month = r.h >= 150;
  const cw = Math.max(24, Math.min(40, Math.floor((r.w - 8) / 7)));
  const ch = month ? 24 : Math.max(18, Math.min(24, Math.floor((r.h - 10) / 2)));
  const bx = r.x + (r.w - cw * 7) / 2;
  const first = month ? new Date(now.getFullYear(), now.getMonth(), 1) : null;
  const dim = month ? new Date(now.getFullYear(), now.getMonth() + 1, 0).getDate() : 7;
  const rows = month ? Math.ceil((first.getDay() + dim) / 7) : 1;
  const by = r.y + r.h - 6 - ch * (rows + 1);
  ctx.textBaseline = 'top'; ctx.textAlign = 'center';
  ctx.font = font(12);
  ['Su', 'Mo', 'Tu', 'We', 'Th', 'Fr', 'Sa'].forEach((d, i) => {
    ctx.fillStyle = 'rgba(28,28,28,0.6)';
    ctx.fillText(d, bx + i * cw + cw / 2, by);
  });
  const dayAt = month
    ? (i) => new Date(now.getFullYear(), now.getMonth(), i + 1)
    : (i) => new Date(now.getFullYear(), now.getMonth(), now.getDate() - now.getDay() + i);
  for (let i = 0; i < dim; i++) {
    const d = dayAt(i);
    const col = month ? (first.getDay() + i) % 7 : i;
    const row = month ? Math.floor((first.getDay() + i) / 7) : 0;
    const cx = bx + col * cw + cw / 2, cy = by + ch + row * ch + ch / 2;
    const today = d.toDateString() === now.toDateString();
    ctx.fillStyle = COL.black; ctx.font = font(16, today);
    ctx.textBaseline = 'middle';
    ctx.fillText(String(d.getDate()), cx, cy);
    if (today) {
      ctx.strokeStyle = COL.red; ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.ellipse(cx, cy, Math.min(14, cw * 0.36), 11, 0, 0, Math.PI * 2);
      ctx.stroke();
    }
  }
}

// 仓库标识：Octicons repo 图标（红 = 一级强调小面积）+ "login / repo"（数据来自 user/repo 缓存）
function drawRepo(r, w) {
  const login = (data.gh && data.gh.login) || '';
  const full = (data.repo && data.repo.fullName) || '';
  const name = full ? full.substring(full.indexOf('/') + 1) : '';
  const label = [login, name].filter(Boolean).join(' / ') || 'github';
  drawIcon(r.x + 13, r.y + r.h / 2, 0, 'red');
  ctx.fillStyle = COL[w.color || 'black'];
  ctx.font = font(16);
  ctx.textAlign = w.align === 'right' ? 'right' : 'left';
  ctx.textBaseline = 'middle';
  ctx.fillText(label, w.align === 'right' ? r.x + r.w - 6 : r.x + 32, r.y + r.h / 2);
}

// 标题字标：Whale-Dock 艺术体 1bpp 位图（tools/make_title.py 生成；console/js/title_art.js
// 内嵌、与固件 resources/title_wordmark.* 同源）。满盒适配（字标自带留白，不用 image 的 16px 内缩）。
// 标题字标：默认 "Whale-Dock" = 内置 Corsiva 位图（与设备逐像素一致）；
// 自定义文字 = 斜体字体 2x 渲染后 50% 阈值二值化（预览贴近墨水屏 1bpp 效果；
// 设备端由编辑器导出/推送时栅格化为内联资源，见 editor.js prepareForDevice）。
function drawTitle(r, w) {
  const text = (w && w.text) || '';
  const color = COL[(w && w.color) || 'black'];
  if (!text || text === 'Whale-Dock') {
    if (!window.TITLE_ART) {
      ctx.fillStyle = color; ctx.font = font(14);
      ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
      ctx.fillText('title_art.js 未加载', r.x + r.w / 2, r.y + r.h / 2);
      return;
    }
    const { w: bw, h: bh, data } = window.TITLE_ART;
    const bytes = Uint8Array.from(atob(data), (c) => c.charCodeAt(0));
    const stride = Math.ceil(bw / 8);
    const scale = Math.min(r.w / bw, r.h / bh, 4);
    const ox = r.x + (r.w - bw * scale) / 2, oy = r.y + (r.h - bh * scale) / 2;
    ctx.fillStyle = color;
    for (let y = 0; y < bh; y++)
      for (let x = 0; x < bw; x++)
        if (bytes[y * stride + (x >> 3)] & (0x80 >> (x & 7)))
          ctx.fillRect(ox + x * scale, oy + y * scale, Math.ceil(scale), Math.ceil(scale));
    return;
  }
  // 自定义文字：离屏 2x 渲染 → 阈值二值化 → 目标区绘制
  const cw = Math.max(40, r.w), chh = Math.max(16, r.h);
  const off = document.createElement('canvas');
  off.width = cw * 2; off.height = chh * 2;
  const g = off.getContext('2d');
  let px = Math.floor(chh * 2 * 0.9);
  g.textBaseline = 'middle'; g.textAlign = 'center';
  do {
    g.font = `italic 700 ${px}px "Monotype Corsiva", Gabriola, "Segoe Script", Georgia, italic serif`;
    if (g.measureText(text).width <= off.width - 8 || px <= 10) break;
    px -= 2;
  } while (true);
  g.fillStyle = '#000';
  g.fillText(text, off.width / 2, off.height / 2);
  const d = g.getImageData(0, 0, off.width, off.height).data;
  const stepX = off.width / cw, stepY = off.height / chh;
  ctx.fillStyle = color;
  for (let y = 0; y < chh; y++)
    for (let x = 0; x < cw; x++) {
      const i = (Math.floor(y * stepY) * off.width + Math.floor(x * stepX)) * 4;
      if ((d[i] + d[i + 1] + d[i + 2]) / 3 < 128)
        ctx.fillRect(r.x + x, r.y + y, 1, 1);
    }
}

// 字段聚合：stats 省略 source 时跨全部数据源取字段（v1.1；概念图四卡跨 user/repo 两源）
function fieldOf(f) {
  for (const k of Object.keys(data)) if (data[k] && data[k][f] != null) return data[k][f];
  return undefined;
}

// 统计：单字段 = 独立卡（v1.2：四栏拆分为独立组件）；chips 变体 = icon + 标签 +
// 数值 横排小卡（红 icon 仅首末枚，对齐概念图）；list = 左标签右数值行
function drawStats(r, w) {
  const d = w.source ? (data[w.source] || {}) : { public_repos: fieldOf('public_repos'), stars: fieldOf('stars'), forks: fieldOf('forks'), followers: fieldOf('followers') };
  const ICON_OF = { public_repos: 0, stars: 1, forks: 2, followers: 3 };
  if (w.variant === 'chips' && w.fields.length === 1) {  // 单字段独立卡（自适应：矮宽条单行 / 高卡上下排）
    const f = w.fields[0];
    const one = r.h < 56;
    // 数值：values 手动覆盖优先（协议 v1.2：mockup/演示用），否则实时数据
    const v0 = (w.values && w.values[0] != null) ? String(w.values[0]) : fmtK(d[f]);
    ctx.strokeStyle = 'rgba(28,28,28,0.35)'; ctx.lineWidth = 1;
    ctx.strokeRect(r.x + 2, r.y + 2, r.w - 4, r.h - 4);
    drawIcon(r.x + 20, r.y + r.h / 2, ICON_OF[f] ?? 0, w.color || 'black');  // 图标随 color
    if (one) {  // 矮宽条：图标 + 标签 + 数值同行
      ctx.fillStyle = 'rgba(28,28,28,0.75)'; ctx.font = font(17);
      ctx.textAlign = 'left'; ctx.textBaseline = 'middle';
      ctx.fillText(w.labels?.[0] ?? f, r.x + 38, r.y + r.h / 2);
      ctx.fillStyle = COL[w.color || 'black']; ctx.font = font(20, true, true);
      ctx.textAlign = 'right';
      ctx.fillText(v0, r.x + r.w - 12, r.y + r.h / 2);
    } else {   // 高卡：标签上、大数值下
      ctx.fillStyle = 'rgba(28,28,28,0.75)'; ctx.font = font(15);
      ctx.textAlign = 'left'; ctx.textBaseline = 'top';
      ctx.fillText(w.labels?.[0] ?? f, r.x + 38, r.y + 12);
      ctx.fillStyle = COL[w.color || 'black']; ctx.font = font(24, true, true);
      ctx.textBaseline = 'bottom';
      ctx.fillText(v0, r.x + r.w - 10, r.y + r.h - 10);
    }
    return;
  }
  if (w.variant === 'chips') {
    const rows = w.fields.map((f, i) => ({
      label: w.labels?.[i] ?? f,
      v: (w.values && w.values[i] != null) ? String(w.values[i]) : fmtK(d[f]),
      red: i === 0 || i === w.fields.length - 1,  // 概念图：Repositories/Followers 红
    }));
    if (r.h > r.w * 0.55 && rows.length >= 3) {
      // 竖卡（概念图右侧竖排）：图标左 + 标签上 + 数值下
      const rh = Math.floor((r.h - 8 * (rows.length - 1)) / rows.length);
      rows.forEach((row, i) => {
        const y = r.y + i * (rh + 8);
        ctx.strokeStyle = 'rgba(28,28,28,0.35)'; ctx.lineWidth = 1;
        ctx.strokeRect(r.x + 2, y, r.w - 4, rh);
        drawIcon(r.x + 26, y + rh / 2, i, row.red ? 'red' : 'black');
        ctx.fillStyle = 'rgba(28,28,28,0.75)'; ctx.font = font(13);
        ctx.textAlign = 'left'; ctx.textBaseline = 'middle';
        ctx.fillText(row.label, r.x + 48, y + rh / 2);
        ctx.fillStyle = COL[row.red ? 'red' : 'black']; ctx.font = font(22, true, true);
        ctx.textAlign = 'right'; ctx.textBaseline = 'middle';
        ctx.fillText(row.v, r.x + r.w - 14, y + rh / 2);
      });
      return;
    }
    const cw = Math.floor((r.w - 12 * (rows.length - 1)) / rows.length);
    rows.forEach((row, i) => {
      const x = r.x + i * (cw + 12), y = r.y + 4, h = r.h - 8;
      ctx.strokeStyle = 'rgba(28,28,28,0.35)'; ctx.lineWidth = 1;
      ctx.strokeRect(x, y, cw, h);
      drawIcon(x + 14, y + h / 2, i, row.red ? 'red' : 'black');
      ctx.fillStyle = 'rgba(28,28,28,0.6)'; ctx.font = font(12);
      ctx.textAlign = 'left'; ctx.textBaseline = 'alphabetic';
      ctx.fillText(row.label, x + 42, y + h / 2 - 6);
      ctx.fillStyle = COL.black; ctx.font = font(24, true, true);
      ctx.fillText(row.v, x + 42, y + h / 2 + 20);
    });
    return;
  }
  const rows = w.fields.map((f, i) => ({ label: w.labels?.[i] ?? f,
      v: (w.values && w.values[i] != null) ? String(w.values[i]) : (d[f] ?? '—') }));
  ctx.textAlign = 'left'; ctx.textBaseline = 'middle';
  const lh = Math.min(44, (r.h - 20) / rows.length);
  rows.forEach((row, i) => {
    const y = r.y + 14 + i * lh + lh / 2;
    ctx.fillStyle = COL.black; ctx.font = font(19);
    ctx.fillText(String(row.label), r.x + 14, y);
    ctx.fillStyle = COL[w.color || 'black']; ctx.font = font(30, true);
    ctx.textAlign = 'right';
    ctx.fillText(String(row.v), r.x + r.w - 14, y);
    ctx.textAlign = 'left';
  });
}

// GitHub 官方 Octicons（primer/octicons 16px，MIT）路径，Path2D 绘制：0 仓库 / 1 星 / 2 fork / 3 关注者
const OCTI = {
  repo: 'M2 2.5A2.5 2.5 0 0 1 4.5 0h8.75a.75.75 0 0 1 .75.75v12.5a.75.75 0 0 1-.75.75h-2.5a.75.75 0 0 1 0-1.5h1.75v-2h-8a1 1 0 0 0-.714 1.7.75.75 0 1 1-1.072 1.05A2.495 2.495 0 0 1 2 11.5Zm10.5-1h-8a1 1 0 0 0-1 1v6.708A2.486 2.486 0 0 1 4.5 9h8ZM5 12.25a.25.25 0 0 1 .25-.25h3.5a.25.25 0 0 1 .25.25v3.25a.25.25 0 0 1-.4.2l-1.45-1.087a.249.249 0 0 0-.3 0L5.4 15.7a.25.25 0 0 1-.4-.2Z',
  star: 'M8 .25a.75.75 0 0 1 .673.418l1.882 3.815 4.21.612a.75.75 0 0 1 .416 1.279l-3.046 2.97.719 4.192a.751.751 0 0 1-1.088.791L8 12.347l-3.766 1.98a.75.75 0 0 1-1.088-.79l.72-4.194L.818 6.374a.75.75 0 0 1 .416-1.28l4.21-.611L7.327.668A.75.75 0 0 1 8 .25Zm0 2.445L6.615 5.5a.75.75 0 0 1-.564.41l-3.097.45 2.24 2.184a.75.75 0 0 1 .216.664l-.528 3.084 2.769-1.456a.75.75 0 0 1 .698 0l2.77 1.456-.53-3.084a.75.75 0 0 1 .216-.664l2.24-2.183-3.096-.45a.75.75 0 0 1-.564-.41L8 2.694Z',
  fork: 'M5 5.372v.878c0 .414.336.75.75.75h4.5a.75.75 0 0 0 .75-.75v-.878a2.25 2.25 0 1 1 1.5 0v.878a2.25 2.25 0 0 1-2.25 2.25h-1.5v2.128a2.251 2.251 0 1 1-1.5 0V8.5h-1.5A2.25 2.25 0 0 1 3.5 6.25v-.878a2.25 2.25 0 1 1 1.5 0ZM5 3.25a.75.75 0 1 0-1.5 0 .75.75 0 0 0 1.5 0Zm6.75.75a.75.75 0 1 0 0-1.5.75.75 0 0 0 0 1.5Zm-3 8.75a.75.75 0 1 0-1.5 0 .75.75 0 0 0 1.5 0Z',
  people: 'M2 5.5a3.5 3.5 0 1 1 5.898 2.549 5.508 5.508 0 0 1 3.034 4.084.75.75 0 1 1-1.482.235 4 4 0 0 0-7.9 0 .75.75 0 0 1-1.482-.236A5.507 5.507 0 0 1 3.102 8.05 3.493 3.493 0 0 1 2 5.5ZM11 4a3.001 3.001 0 0 1 2.22 5.018 5.01 5.01 0 0 1 2.56 3.012.749.749 0 0 1-.885.954.752.752 0 0 1-.549-.514 3.507 3.507 0 0 0-2.522-2.372.75.75 0 0 1-.574-.73v-.352a.75.75 0 0 1 .416-.672A1.5 1.5 0 0 0 11 5.5.75.75 0 0 1 11 4Zm-5.5-.5a2 2 0 1 0-.001 3.999A2 2 0 0 0 5.5 3.5Z',
};
const OCTI_PATHS = [OCTI.repo, OCTI.star, OCTI.fork, OCTI.people];
function drawIcon(x, cy, kind, colorName) {
  ctx.save();
  ctx.translate(x - 11, cy - 11);   // 16px path 放大 1.375 → 22px，居中于 (x,cy)
  ctx.scale(1.375, 1.375);
  ctx.fillStyle = colorName === 'red' ? COL.red
    : colorName === 'yellow' ? COL.yellow : 'rgba(28,28,28,0.75)';
  ctx.fill(new Path2D(OCTI_PATHS[kind]));
  ctx.restore();
}

// 热力图（概念图中层）：上缘细分割线 + 标题 + 月份轴（点阵上方，概念图样式）
// + 26 周 × 7 天点阵 + Less..More 图例；格子统一等大，选择性填充：0=空框 1/2/3=实心黑 4=实心红
function drawHeatMap(r, w) {
  const grid = heatDemo;
  // 上缘横贯分割线（上下层版式对齐基准，概念图关键细节）
  ctx.strokeStyle = 'rgba(28,28,28,0.8)'; ctx.lineWidth = 2;
  ctx.beginPath(); ctx.moveTo(r.x, r.y + 2); ctx.lineTo(r.x + r.w, r.y + 2); ctx.stroke();
  // 标题（左对齐，概念图样式）
  ctx.fillStyle = COL.black; ctx.font = font(17, true);
  ctx.textAlign = 'left'; ctx.textBaseline = 'top';
  ctx.fillText(w.title || 'GitHub Contribution', r.x + 4, r.y + 10);
  // 点阵：格距按区宽自适应铺满（7 行 × ~16px）
  const weeks = grid.length, days = 7;
  const cw = Math.floor((r.w - 8) / weeks);
  const cell = cw - 2, gap = 2;
  const gw = weeks * cw;
  const gx = r.x + 4;
  const gy = r.y + 38;
  const now = new Date();
  const monthOfCol = (wi) => {
    const d = new Date(now); d.setDate(d.getDate() - (weeks - 1 - wi) * 7);
    return d.getMonth();
  };
  grid.forEach((week, wi) => week.forEach((lv, di) => {
    const x = gx + wi * cw, y = gy + di * cw;
    // 统一等大方块，选择性填充：0=空框 1/2/3=实心黑 4=实心红（四色屏无灰阶，不玩点径）
    if (lv === 0) {
      ctx.strokeStyle = 'rgba(28,28,28,0.18)';
      ctx.strokeRect(x + 0.5, y + 0.5, cell - 1, cell - 1);
    } else {
      ctx.fillStyle = lv === 4 ? COL.red : COL.black;
      ctx.fillRect(x, y, cell, cell);
    }
  }));
  // 月份轴（与列一一对齐）独占一行，Less..More 图例再下一行——两者不重叠
  const axisY = r.y + 26;
  ctx.fillStyle = 'rgba(28,28,28,0.7)'; ctx.font = font(11);
  ctx.textBaseline = 'top';
  const names = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
  let last = -1, lastEnd = -1e9;
  for (let wi = 0; wi < weeks; wi++) {
    const m = monthOfCol(wi);
    if (m !== last) {
      last = m;
      const x = gx + wi * cw;
      if (x > lastEnd + 6) {  // 防相邻月标签挤靠（短月+列相位可致间距过近）
        ctx.textAlign = 'left';
        // 末列标签右缘对齐，防溢出画布
        if (x + ctx.measureText(names[m]).width > r.x + r.w) {
          ctx.textAlign = 'right';
          ctx.fillText(names[m], r.x + r.w, axisY);
          ctx.textAlign = 'left';
        } else {
          ctx.fillText(names[m], x, axisY);
        }
        lastEnd = x + ctx.measureText(names[m]).width;
      }
    }
  }
  // Less…More 图例：点阵下方、与点阵左缘对齐（概念图图例在左下）
  // Less…More 图例：与点阵同款等大方块（空框 → 黑 → 红），Less 左 More 右
  const ly = gy + days * cw + 4;
  const legendW = 3 * cw;
  const lx = gx;
  ctx.fillStyle = 'rgba(28,28,28,0.7)'; ctx.font = font(11);
  ctx.textAlign = 'left';
  ctx.fillText('Less', lx, ly + 1);
  [0, 3, 4].forEach((lv, i) => {
    const x = lx + 34 + i * cw;
    if (lv === 0) { ctx.strokeStyle = 'rgba(28,28,28,0.18)'; ctx.strokeRect(x + 0.5, ly + 1.5, cell - 1, cell - 1); }
    else { ctx.fillStyle = lv === 4 ? COL.red : COL.black; ctx.fillRect(x, ly + 2, cell, cell); }
  });
  ctx.fillStyle = 'rgba(28,28,28,0.7)';
  ctx.textAlign = 'left';
  ctx.fillText('More', lx + 34 + legendW + 6, ly + 1);
}

function drawBarChart(r, w) {
  const d = data[w.source] || {};
  const all = d.participation || [];
  const weeks = Math.min(w.weeks || 12, all.length);
  const arr = all.slice(-weeks);
  const title = w.title || '近提交';
  const top = r.y + 12, bot = r.y + r.h - 26;
  ctx.fillStyle = COL.black; ctx.font = font(17, true);
  ctx.textAlign = 'left'; ctx.textBaseline = 'top';
  ctx.fillText(title, r.x + 12, top);
  const maxV = Math.max(...arr, w.goal || 0, 1);
  const areaTop = top + 26, areaH = bot - areaTop;
  const bw = (r.w - 24) / arr.length;
  arr.forEach((v, i) => {
    const h = Math.max(3, Math.round((v / maxV) * areaH));
    ctx.fillStyle = COL.black;
    ctx.fillRect(r.x + 12 + i * bw + 2, bot - h, bw - 4, h);
  });
  if (w.goal) {
    const gy = bot - Math.round((w.goal / maxV) * areaH);
    ctx.save();
    ctx.setLineDash([6, 4]);
    ctx.strokeStyle = COL[w.goalColor || 'yellow'];
    ctx.lineWidth = 2.5;
    ctx.beginPath(); ctx.moveTo(r.x + 12, gy); ctx.lineTo(r.x + r.w - 12, gy); ctx.stroke();
    ctx.fillStyle = COL[w.goalColor || 'yellow']; ctx.font = font(12);
    ctx.textBaseline = 'bottom'; ctx.textAlign = 'right';
    ctx.fillText(`目标 ${w.goal}`, r.x + r.w - 12, gy - 2);
    ctx.restore();
  }
  ctx.fillStyle = 'rgba(28,28,28,0.55)'; ctx.font = font(11);
  ctx.textBaseline = 'top'; ctx.textAlign = 'left';
  ctx.fillText(`W-${weeks + 1}`, r.x + 12, bot + 6);
  ctx.textAlign = 'right';
  ctx.fillText('本周', r.x + r.w - 12, bot + 6);
}

function drawText(r, w) {
  const size = { s: 14, m: 22, l: 32 }[w.size || 'm'];
  ctx.fillStyle = COL[w.color || 'black'];
  ctx.font = font(size);
  ctx.textAlign = w.align || 'center'; ctx.textBaseline = 'middle';
  wrap(String(w.text || ''), r.w - 16, size).forEach((line, i, a) =>
    ctx.fillText(line, w.align === 'right' ? r.x + r.w - 6 : w.align === 'left' ? r.x + 6 : r.x + r.w / 2,
                 r.y + r.h / 2 + (i - (a.length - 1) / 2) * size * 1.4));
}

function wrap(text, maxW, size) {
  ctx.font = font(size);
  const out = [];
  let line = '';
  for (const ch of text) {
    if (ctx.measureText(line + ch).width > maxW) { out.push(line); line = ch; }
    else line += ch;
  }
  if (line) out.push(line);
  return out;
}

function drawImage(r, w, doc) {
  // 保留 id whale_pixel = 内置资源（协议 v1.2 附录 B；console/js/whale_art.js 与固件同源）
  const res = w.resBW === 'whale_pixel'
    ? window.WHALE_ART
    : (doc.resources || []).find((x) => x.id === w.resBW);
  ctx.fillStyle = COL.black; ctx.font = font(14);
  ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
  if (!res) {
    ctx.fillText(`资源 '${w.resBW || ''}' 未随布局内联`, r.x + r.w / 2, r.y + r.h / 2);
    return;
  }
  const bytes = Uint8Array.from(atob(res.data), (c) => c.charCodeAt(0));
  const stride = Math.ceil(res.w / 8), expect = stride * res.h;
  if (bytes.length !== expect) {
    ctx.fillText(`资源尺寸不符（${bytes.length}B ≠ ${expect}B）`, r.x + r.w / 2, r.y + r.h / 2);
    return;
  }
  const planes = [['resBW', 'black'], ['resR', 'red'], ['resY', 'yellow']];
  // v1.2：横纵独立缩放铺满槽位（横向/纵向拉伸即时可见——等比适配在高度受限时
  // 拉宽只加留白、小槽位缩放甚至为负，是"拉伸不起作用"的根因）；上限 8 防马赛克巨画
  const sx = Math.min(r.w / res.w, 8), sy = Math.min(r.h / res.h, 8);
  const ox = r.x, oy = r.y;
  for (const [key, col] of planes) {
    const rr = key === 'resBW' ? res : (doc.resources || []).find((x) => x.id === w[key]);
    if (!rr) continue;
    const bb = Uint8Array.from(atob(rr.data), (c) => c.charCodeAt(0));
    ctx.fillStyle = COL[col];
    for (let y = 0; y < rr.h; y++)
      for (let x = 0; x < rr.w; x++)
        if (bb[y * stride + (x >> 3)] & (0x80 >> (x & 7))) {
          const dx0 = Math.round(x * sx), dx1 = Math.round((x + 1) * sx);
          const dy0 = Math.round(y * sy), dy1 = Math.round((y + 1) * sy);
          ctx.fillRect(ox + dx0, oy + dy0, Math.max(1, dx1 - dx0), Math.max(1, dy1 - dy0));
        }
  }
}

function drawQr(r, w) {
  const size = Math.min(w.size || 128, r.w - 16, r.h - 16);
  const x = r.x + (r.w - size) / 2, y = r.y + (r.h - size) / 2;
  ctx.strokeStyle = COL.black; ctx.lineWidth = 2;
  ctx.strokeRect(x, y, size, size);
  ctx.fillStyle = COL.black; ctx.font = font(13);
  ctx.textAlign = 'center'; ctx.textBaseline = 'bottom';
  ctx.fillText('QR 占位（恒黑色 · B3 接入生成库）', r.x + r.w / 2, y - 4);
}

function drawTicker(r, w) {
  ctx.fillStyle = COL[w.color || 'black'];
  ctx.font = font(22);
  ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
  ctx.fillText(String(w.text || ''), r.x + r.w / 2, r.y + r.h / 2);
}

function render(doc) {
  ctx.fillStyle = PAPER;
  ctx.fillRect(0, 0, cv.width, cv.height);
  const v = validate(doc);
  if (v.mode === 'bitmap') {
    ctx.fillStyle = COL.black; ctx.font = font(18);
    ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    ctx.fillText('位图通道（display）：三平面解码上屏由 B3 实装，模拟器暂不渲染', cv.width / 2, cv.height / 2);
  } else {
    for (const w of v.widgets) {
      const r = rectOf(w);
      ({ clock: drawClock, date: drawDate, calendar: drawCalendar, stats: drawStats,
         barChart: drawBarChart, heatMap: drawHeatMap, text: drawText, repo: drawRepo,
         title: drawTitle, image: (rr, ww) => drawImage(rr, ww, doc),
         qr: drawQr, ticker: drawTicker })[w.type]?.(r, w);
    }
  }
  if ($('guides').checked) drawGuides();
}

function setVerdict(ok, text) {
  const el = $('verdict');
  el.className = 'verdict ' + (ok ? 'ok' : 'bad');
  el.textContent = text;
}

function go() {
  let doc;
  try { doc = JSON.parse($('json').value); }
  catch (e) { setVerdict(false, `规则0: JSON 解析失败（${e.message}）`); return; }
  const v = validate(doc);
  if (v.errors.length) {
    setVerdict(false, `未通过（${v.errors.length} 项）：\n` + v.errors.join('\n'));
  } else {
    setVerdict(true, `校验通过：${v.widgets.length} widgets · ${v.sources?.length ?? 0} 数据源 · ${v.mode === 'bitmap' ? '位图' : 'Widget'} 模式\n` +
      v.widgets.map((w) => `  ${w.slot || '-'}${w.slotRect ? '*' : ''} → ${w.type}`).join('\n') +
      '\n（* = slotRect 覆写几何；v1.2：slot 可省略）');
  }
  render(doc);
}

// ---- 联网拉取（api.github.com 允许跨域；participation 首次可能 202，稍后重试） ----
async function fetchLive() {
  $('btn-live').disabled = true;
  $('data-state').textContent = '拉取中…（api.github.com）';
  try {
    const [u, rep] = await Promise.all([
      fetch('https://api.github.com/users/datawhalechina').then((r) => r.json()),
      fetch('https://api.github.com/repos/datawhalechina/leeml-notes').then((r) => r.json()),
    ]);
    let part = null;
    for (let i = 0; i < 3 && !part; i++) {
      const r = await fetch('https://api.github.com/repos/datawhalechina/leeml-notes/stats/participation');
      if (r.status === 200) part = (await r.json()).all;
      else await new Promise((res) => setTimeout(res, 2000));
    }
    data = {
      gh: { login: u.login, public_repos: u.public_repos, followers: u.followers },
      repo: { fullName: rep.full_name, stars: rep.stargazers_count, forks: rep.forks_count,
              participation: part || data.repo.participation },
    };
    $('data-state').textContent =
      `已连接真实数据：仓库 ${data.gh.public_repos} · 关注 ${data.gh.followers} · Stars ${data.repo.stars}` +
      `（热力图为演示数据——按日贡献需 GraphQL，二期接入）`;
    go();
  } catch (e) {
    $('data-state').textContent = '拉取失败（网络/限额）：' + e.message + '，继续用演示数据。';
  } finally {
    $('btn-live').disabled = false;
  }
}

// ---- 装配 ----
$('btn-render').addEventListener('click', go);
$('btn-sample').addEventListener('click', () => { $('json').value = SAMPLE; go(); });
$('btn-demo').addEventListener('click', () => {
  data = { ...demoData, repo: { ...demoData.repo, participation: [...demoData.repo.participation] } };
  $('data-state').textContent = '当前：演示数据（内置快照，不联网）。';
  go();
});
$('btn-live').addEventListener('click', fetchLive);
$('guides').addEventListener('change', () => {
  try { render(JSON.parse($('json').value)); } catch { /* 未渲染时忽略 */ }
});
$('json').value = SAMPLE;
go();
