// 鲸屿 WhaleDock · 显示协议 v1.1 模拟器（零依赖静态页）
// 校验规则与 docs/显示协议-v1.md §9、firmware/src/app/layout.cpp 同源；
// 默认示例 = 概念图-v2（鲸屿形态）屏幕版式 1:1 复刻。
// 三方（固件/编辑器/模拟器）规则改动必须同步这三处。

const $ = (id) => document.getElementById(id);

// ---- 协议 §3 槽位几何（与固件一致） ----
const SLOTS = {
  tl:     { x: 12,  y: 12,  w: 382, h: 192 },
  tr:     { x: 406, y: 12,  w: 382, h: 192 },
  bl:     { x: 12,  y: 216, w: 382, h: 192 },
  br:     { x: 406, y: 216, w: 382, h: 192 },
  ticker: { x: 12,  y: 420, w: 776, h: 48 },
};

const COL = { black: '#1c1c1c', red: '#b3382c', yellow: '#d4a017' };
const PAPER = '#f5f4ef';
const TYPES = ['clock', 'stats', 'barChart', 'pet', 'text', 'image', 'qr', 'ticker', 'heatMap'];
const FIELDS = ['public_repos', 'followers', 'stars', 'forks'];

// ---- 默认示例：概念图 v2（鲸屿形态）屏幕版式 1:1 复刻 ----
// 三层横带构图用 v1.1 slotRect 覆写象限默认几何；鲸鱼 = datawhalelogo.png 转 1bpp 位图资源（协议 image 类型）
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
}`;

// ---- 演示数据（内置快照；联网按钮会覆盖 user/repo；热力图需 GraphQL，恒用演示） ----
const demoData = {
  gh:   { public_repos: 42, followers: 316 },
  repo: { stars: 1240, forks: 358,
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

  const used = new Set();
  for (const w of widgets) {
    if (!SLOTS[w.slot]) { errors.push(`规则4: 非法 slot '${w.slot}'`); continue; }
    if (used.has(w.slot)) errors.push(`规则4: slot '${w.slot}' 重复占用`);
    used.add(w.slot);
    if (!TYPES.includes(w.type)) { errors.push(`规则5: 未注册类型 '${w.type}'`); continue; }
    if (w.slot === 'ticker' && !['ticker', 'text'].includes(w.type))
      errors.push(`规则4: ticker 槽不允许类型 '${w.type}'`);
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

// 像素鲸鱼（whale_pixel）：概念图 v2 同款造型——头朝右、尾在左上翘、头顶三股喷水、腹部白纹镂空。
// 22×18 矩阵由 tools/pixelize.html 从概念图-v2-鲸屿.jpg 屏幕左上区（原图 x715-995, y160-385）
// 点采样提取（阈值 128），仅剔除屏幕边框线；改动需重新提取并与概念图目视比对。
const WHALE = [
  '......................',
  '.............K........',
  '............K.K.......',
  '............K.........',
  '...........KK.........',
  '...........KK.KK......',
  '.........KKKKKKKKK....',
  '........K.KKKKKKKKK...',
  '...K...KKKKKKKKKKKK...',
  '...K..KKKKKKKKKKKKK...',
  '.KKK..KKKKKKKKKKKKK...',
  '.KKK.KKKKKKKKKKKKKK...',
  '.KKKKKKKKKKKKKKKKKK...',
  '...KKKKKKKKKKKKKKKK...',
  '....KKKKKKKKKKKKKKK...',
  '....KKKK..KKKKKKKK....',
  '.....KKKK..KKKKKKK....',
  '......KKKKK..KKKKK....',
];

function drawPet(r, w) {
  const px = Math.floor(Math.min((r.w - 8) / WHALE[0].length, (r.h - 12) / WHALE.length));
  const ox = Math.round(r.x + (r.w - WHALE[0].length * px) / 2);
  const oy = Math.round(r.y + (r.h - WHALE.length * px) / 2);
  ctx.fillStyle = COL.black;
  for (let y = 0; y < WHALE.length; y++)
    for (let x = 0; x < WHALE[y].length; x++)
      if (WHALE[y][x] === 'K')
        ctx.fillRect(ox + x * px, oy + y * px, px - 1, px - 1);
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
// 时钟：点阵时间 + 日期 + 整月日历阵（Su-Sa 表头 + 当月日期 5×7，今日红圈；概念图样式）
function drawClock(r, w) {
  const now = new Date();
  const align = w.align || 'center';
  const ax = align === 'right' ? r.x + r.w - 16 : align === 'left' ? r.x + 16 : r.x + r.w / 2;
  ctx.textAlign = align; ctx.textBaseline = 'alphabetic';
  // 时间：5×7 点阵字模逐点绘制（概念图点阵数字风）
  const px = 9, gap = 7;
  const timeStr = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`;
  const totalW = [...timeStr].reduce((a, ch) => a + GLYPH5x7[ch][0].length, 0) * px + (timeStr.length - 1) * gap;
  let tx = align === 'right' ? r.x + r.w - 16 - totalW : align === 'left' ? r.x + 16 : r.x + (r.w - totalW) / 2;
  const ty = r.y + 16;
  ctx.fillStyle = COL.black;
  for (const ch of timeStr) {
    GLYPH5x7[ch].forEach((row, y) => [...row].forEach((v, x) => {
      if (v === '1') ctx.fillRect(tx + x * px, ty + y * px, px - 1, px - 1);
    }));
    tx += GLYPH5x7[ch][0].length * px + gap;
  }
  // 日期行
  const wd = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'][now.getDay()];
  ctx.font = font(20);
  ctx.fillText(`${now.getFullYear()}-${String(now.getMonth() + 1).padStart(2, '0')}-${String(now.getDate()).padStart(2, '0')} ${wd}`, ax, r.y + 102);
  // 整月日历阵：表头一行 + 当月日期（首日星期对齐，行数按当月实际占用），贴槽位底部
  const cw = 40, ch = 24;
  const first = new Date(now.getFullYear(), now.getMonth(), 1);
  const dim = new Date(now.getFullYear(), now.getMonth() + 1, 0).getDate();
  const rows = Math.ceil((first.getDay() + dim) / 7);
  const gw = cw * 7, gh = ch * (rows + 1);
  const bx = align === 'right' ? r.x + r.w - 16 - gw : align === 'left' ? r.x + 16 : r.x + (r.w - gw) / 2;
  const by = r.y + r.h - 12 - gh;
  ctx.font = font(12);
  ['Su', 'Mo', 'Tu', 'We', 'Th', 'Fr', 'Sa'].forEach((d, i) => {
    ctx.fillStyle = 'rgba(28,28,28,0.6)';
    ctx.textAlign = 'center'; ctx.textBaseline = 'top';
    ctx.fillText(d, bx + i * cw + cw / 2, by);
  });
  for (let i = 0; i < dim; i++) {
    const col = (first.getDay() + i) % 7, row = Math.floor((first.getDay() + i) / 7);
    const cx = bx + col * cw + cw / 2, cy = by + ch + row * ch + ch / 2;
    const today = i + 1 === now.getDate();
    ctx.fillStyle = COL.black; ctx.font = font(16, today);
    ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    ctx.fillText(String(i + 1), cx, cy);
    if (today) {
      ctx.strokeStyle = COL.red; ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.ellipse(cx, cy, 14, 11, 0, 0, Math.PI * 2);
      ctx.stroke();
    }
  }
}

// 字段聚合：stats 省略 source 时跨全部数据源取字段（v1.1；概念图四卡跨 user/repo 两源）
function fieldOf(f) {
  for (const k of Object.keys(data)) if (data[k] && data[k][f] != null) return data[k][f];
  return undefined;
}

// 统计：chips 变体 = icon + 标签 + 数值 横排小卡（红 icon 仅首末枚，对齐概念图）
function drawStats(r, w) {
  const d = w.source ? (data[w.source] || {}) : { public_repos: fieldOf('public_repos'), stars: fieldOf('stars'), forks: fieldOf('forks'), followers: fieldOf('followers') };
  if (w.variant === 'chips') {
    const rows = w.fields.map((f, i) => ({
      label: w.labels?.[i] ?? f, v: fmtK(d[f]),
      red: i === 0 || i === w.fields.length - 1,  // 概念图：Repositories/Followers 红
    }));
    if (r.h > r.w * 0.55 && rows.length >= 3) {
      // 竖卡（概念图右侧竖排）：图标左 + 标签上 + 数值下
      const rh = Math.floor((r.h - 8 * (rows.length - 1)) / rows.length);
      rows.forEach((row, i) => {
        const y = r.y + i * (rh + 8);
        ctx.strokeStyle = 'rgba(28,28,28,0.35)'; ctx.lineWidth = 1;
        ctx.strokeRect(r.x + 2, y, r.w - 4, rh);
        drawIcon(r.x + 26, y + rh / 2, i, row.red);
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
      drawIcon(x + 14, y + h / 2, i, row.red);
      ctx.fillStyle = 'rgba(28,28,28,0.6)'; ctx.font = font(12);
      ctx.textAlign = 'left'; ctx.textBaseline = 'alphabetic';
      ctx.fillText(row.label, x + 42, y + h / 2 - 6);
      ctx.fillStyle = COL.black; ctx.font = font(24, true, true);
      ctx.fillText(row.v, x + 42, y + h / 2 + 20);
    });
    return;
  }
  const rows = w.fields.map((f, i) => ({ label: w.labels?.[i] ?? f, v: d[f] ?? '—' }));
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
function drawIcon(x, cy, kind, red) {
  ctx.save();
  ctx.translate(x - 11, cy - 11);   // 16px path 放大 1.375 → 22px，居中于 (x,cy)
  ctx.scale(1.375, 1.375);
  ctx.fillStyle = red ? COL.red : 'rgba(28,28,28,0.75)';
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
  const res = (doc.resources || []).find((x) => x.id === w.resBW);
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
  const scale = Math.min((r.w - 16) / res.w, (r.h - 16) / res.h, 8);
  const ox = r.x + (r.w - res.w * scale) / 2, oy = r.y + (r.h - res.h * scale) / 2;
  for (const [key, col] of planes) {
    const rr = key === 'resBW' ? res : (doc.resources || []).find((x) => x.id === w[key]);
    if (!rr) continue;
    const bb = Uint8Array.from(atob(rr.data), (c) => c.charCodeAt(0));
    ctx.fillStyle = COL[col];
    for (let y = 0; y < rr.h; y++)
      for (let x = 0; x < rr.w; x++)
        if (bb[y * stride + (x >> 3)] & (0x80 >> (x & 7)))
          ctx.fillRect(ox + x * scale, oy + y * scale, Math.ceil(scale), Math.ceil(scale));
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
      ({ clock: drawClock, stats: drawStats, barChart: drawBarChart, pet: drawPet,
         heatMap: drawHeatMap, text: drawText, image: (rr, ww) => drawImage(rr, ww, doc),
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
      v.widgets.map((w) => `  ${w.slot}${w.slotRect ? '*' : ''} → ${w.type}`).join('\n') +
      '\n（* = slotRect 覆写几何，v1.1）');
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
      gh: { public_repos: u.public_repos, followers: u.followers },
      repo: { stars: rep.stargazers_count, forks: rep.forks_count,
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
