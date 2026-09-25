// ============================================================
// D3 布局编辑器（editor.html）
// 画布 = 复用 sim.js 渲染器；交互 = Moveable（唯一外部依赖）
// 拖拽/缩放写回 widget.slotRect（协议 v1.1）；属性面板按类型出字段；
// 导出走 sim.js validate()（与固件校验同源）；推送走设备 HTTP API。
// ============================================================

const $e = (id) => document.getElementById(id);
const holder = $e('holder');

// ---- 文档模型（编辑器持有的布局对象）----
let doc = null;          // { version, dataSources, layout:{resolution, widgets}, resources }
let selIdx = -1;         // 选中 widget 下标

function defaultDoc() {
  return JSON.parse(SAMPLE);
}

// ---- 渲染（复用模拟器）----
function paint() {
  if (!doc) return;
  try { render(doc); } catch (e) { msg('渲染异常：' + e.message); }
  // 空海（潮汐）：无挂件 = 鲸影在屏中缓游 + 题句
  if (doc.layout.widgets.length === 0) startEmptySea();
  else stopEmptySea();
}

let seaRaf = 0;
function stopEmptySea() {
  if (seaRaf) cancelAnimationFrame(seaRaf);
  seaRaf = 0;
}
function startEmptySea() {
  const c = $e('screen'), g = c.getContext('2d');
  const reduce = matchMedia('(prefers-reduced-motion: reduce)').matches;
  const step = () => {
    g.fillStyle = '#f5f4ef';
    g.fillRect(0, 0, c.width, c.height);
    const dx = reduce ? 0 : Math.sin(performance.now() / 1000 * 2 * Math.PI / 7) * 10;
    window.__drawWhaleAt && __drawWhaleAt(g, c.width / 2 - 61 + dx, c.height / 2 - 53,
                                         0.55, 'rgba(35, 34, 28, .5)');
    g.fillStyle = 'rgba(35, 34, 28, .45)';
    g.font = '13px Georgia, "Microsoft YaHei", serif';
    g.textAlign = 'center';
    g.fillText('海域空空如也，拖一个挂件上岛。', c.width / 2, c.height / 2 + 78);
    if (!reduce) seaRaf = requestAnimationFrame(step);
  };
  stopEmptySea();
  step();
}

// ---- slot 覆盖 div + Moveable ----
let mv = null;
const hits = [];  // 与 widgets 同序的覆盖 div
const ratioLock = new Set();  // 锁宽高比的 widget（编辑器行为，不入协议 JSON）

function rectOfIdx(i) {
  const w = doc.layout.widgets[i];
  return rectOf(w);  // sim.js 的槽位几何函数（含 slotRect 覆写）
}

function rebuildHits() {
  hits.forEach((h) => h.remove());
  hits.length = 0;
  if (!doc) return;
  doc.layout.widgets.forEach((w, i) => {
    const r = rectOfIdx(i);
    const el = document.createElement('div');
    el.className = 'slot-hit' + (i === selIdx ? ' sel' : '');
    el.style.left = r.x + 'px';
    el.style.top = r.y + 'px';
    el.style.width = r.w + 'px';
    el.style.height = r.h + 'px';
    el.addEventListener('mousedown', () => select(i));
    holder.appendChild(el);
    hits.push(el);
  });
  syncMoveable();
}

function syncMoveable() {
  // 销毁重建：Moveable 对 target 属性赋值不保证生效（空数组 = 零目标多选，
  // 实测 mv.target = el 后控制框仍不挂载），重建是最可靠的绑定方式
  if (mv) { mv.destroy(); mv = null; }
  const target = selIdx >= 0 ? hits[selIdx] : null;
  if (!target) return;  // 未选中：不挂交互层
  mv = new Moveable(holder, {
    target,
    draggable: true,
    resizable: true,
    // 默认自由拉伸（横/纵独立）；位图类组件可在属性面板勾选"锁宽高比"按需锁定
    keepRatio: ratioLock.has(doc.layout.widgets[selIdx]),
    origin: false,
    throttleDrag: 1,
    throttleResize: 1,
    snappable: true,
    snapThreshold: 5,
    elementGuidelines: hits.filter((_, i) => i !== selIdx),
    horizontalGuidelines: [0, 240, 480],
    verticalGuidelines: [0, 400, 800],
  });
  bindMvEvents();
  requestAnimationFrame(() => mv && mv.updateRect());  // 构造后控制框默认 1×1，需显式测量
}

function writeRect(i, x, y, w, h) {
  // 越界钳制 + 写回 slotRect（与默认几何一致时可删除覆写）
  x = Math.max(0, Math.min(800 - w, Math.round(x)));
  y = Math.max(0, Math.min(480 - h, Math.round(y)));
  const widget = doc.layout.widgets[i];
  widget.slotRect = { x, y, w: Math.round(w), h: Math.round(h) };
}

function bindMvEvents() {
  // 注意 0.53 事件形状：drag/resize 进行中事件带 beforeTranslate / width / height；
  // End 事件（dragEnd/resizeEnd）只带 lastEvent——几何信息须从 ev.lastEvent 取，
  // 直接读 ev.beforeTranslate 会 TypeError 掐断后续监听（拖拽"没反应"的根因）
  mv.on('drag', (ev) => {
    const r = rectOfIdx(selIdx);
    const h = hits[selIdx];
    h.style.left = r.x + ev.beforeTranslate[0] + 'px';
    h.style.top = r.y + ev.beforeTranslate[1] + 'px';
  });
  mv.on('dragEnd', (ev) => {
    const bt = (ev.lastEvent && ev.lastEvent.beforeTranslate) || [0, 0];
    const r = rectOfIdx(selIdx);
    writeRect(selIdx, r.x + bt[0], r.y + bt[1], r.w, r.h);
    afterEdit();
  });
  mv.on('resize', (ev) => {
    const r = rectOfIdx(selIdx);
    const h = hits[selIdx];
    h.style.width = ev.width + 'px';
    h.style.height = ev.height + 'px';
    h.style.left = r.x + ev.drag.beforeTranslate[0] + 'px';
    h.style.top = r.y + ev.drag.beforeTranslate[1] + 'px';
  });
  mv.on('resizeEnd', (ev) => {
    const le = ev.lastEvent || {};
    const bt = (le.drag && le.drag.beforeTranslate) || [0, 0];
    const r = rectOfIdx(selIdx);
    writeRect(selIdx, r.x + bt[0], r.y + bt[1],
              le.width ?? r.w, le.height ?? r.h);
    afterEdit();
  });
}

function afterEdit() {
  updateHits();   // 只更新几何位置（不销毁 DOM，Moveable target 保持有效）
  if (mv) mv.updateRect();  // 几何被外部改写（拖拽落笔/数值输入）后同步控制框
  paint();        // 画布重渲染
  buildProps();   // 属性面板数值刷新
  dirty = true;
  autosave();     // localStorage 自动存档（刷新/误关恢复）
}

// ---- 布局存档：打开 / 保存 / 自动存档 ----
const AUTOSAVE_KEY = 'whaledock_editor_layout_v1';

function autosave() {
  try { localStorage.setItem(AUTOSAVE_KEY, JSON.stringify(doc)); } catch (_) { /* 配额满等，静默 */ }
}

function restoreAutosave() {
  try {
    const s = localStorage.getItem(AUTOSAVE_KEY);
    if (!s) return false;
    loadLayoutFromText(s);
    return true;
  } catch (_) { return false; }
}

// 载入布局文本（打开文件 / 自动存档共用）：解析 → 校验 → 生效；失败保留原 doc
function loadLayoutFromText(text) {
  let d;
  try { d = JSON.parse(text); }
  catch (e) { msg('布局文件不是合法 JSON：' + e.message); return false; }
  const v = validate(d);
  if (v.errors.length) {
    msg('布局校验未通过，未载入：\n' + v.errors.join('\n'));
    return false;
  }
  doc = d;
  selIdx = -1;
  afterEdit();
  buildLayers();
  renderSources();
  return true;
}

const layoutFile = document.createElement('input');
layoutFile.type = 'file'; layoutFile.accept = '.json,application/json';
layoutFile.style.display = 'none';
document.body.appendChild(layoutFile);

layoutFile.addEventListener('change', async () => {
  const f = layoutFile.files && layoutFile.files[0];
  if (!f) return;
  const text = await f.text();
  if (loadLayoutFromText(text)) msg(`已载入 ${f.name}（编辑态原文，设备交付走「导出 JSON」）`);
});

$e('btn-open').addEventListener('click', () => { layoutFile.value = ''; layoutFile.click(); });

$e('btn-save').addEventListener('click', () => {
  const v = validate(doc);
  if (v.errors.length) { msg('当前布局有校验错误，仍可保存（设备会拒绝）：\n' + v.errors.join('\n')); }
  const now = new Date();
  const p = (n) => String(n).padStart(2, '0');
  const stamp = `${now.getFullYear()}${p(now.getMonth() + 1)}${p(now.getDate())}-${p(now.getHours())}${p(now.getMinutes())}`;
  const json = JSON.stringify(doc, null, 2);
  const a = document.createElement('a');
  a.href = URL.createObjectURL(new Blob([json], { type: 'application/json' }));
  a.download = `whaledock-layout-${stamp}.json`;
  a.click();
  URL.revokeObjectURL(a.href);
  msg(`已保存编辑态布局（${json.length}B）。提示：「导出 JSON」才是设备交付格式`);
});

// 几何更新：现有覆盖 div 直接改 left/top/width/height
function updateHits() {
  if (!doc) return;
  if (hits.length !== doc.layout.widgets.length) {
    rebuildHits();  // 数量变了（增删）→ 全量重建
    return;
  }
  doc.layout.widgets.forEach((w, i) => {
    const r = rectOfIdx(i);
    const h = hits[i];
    h.style.left = r.x + 'px';
    h.style.top = r.y + 'px';
    h.style.width = r.w + 'px';
    h.style.height = r.h + 'px';
  });
}

// ---- 选中 / 图层面板 ----
let dirty = false;

function select(i) {
  if (i === selIdx) return;  // 已选中：不重绑——Moveable 的拖拽手势挂在 target 上，
                             // mousedown 中途销毁重建会掐死正在开始的拖拽
  selIdx = i;
  hits.forEach((h, k) => h.classList.toggle('sel', k === selIdx));
  syncMoveable();
  buildLayers();
  buildProps();
}

// 图层显示名：优先 widget.name（协议 v1.2 可选字段，设备端忽略），
// 否则按内容自动派生（stats 用标签 / text 用文案 / image 用资源名……）
function autoName(w) {
  if (w.name) return w.name;
  const t = TYPE_LABEL[w.type] || w.type;
  if (w.type === 'stats') {
    const n = (w.fields || []).length;
    if (n === 1) return `${t}·${(w.labels && w.labels[0]) || w.fields[0]}`;
    return n ? `${t}·${n}项` : t;
  }
  if (w.type === 'text' || w.type === 'ticker')
    return `${t}·${String(w.text || '').slice(0, 8)}`;
  if (w.type === 'image') return w.resBW ? `${t}·${w.resBW}` : t;
  if (w.type === 'heatMap' || w.type === 'barChart')
    return w.title ? `${t}·${w.title}` : t;
  return t;
}

function buildLayers() {
  const ul = $e('layers');
  ul.innerHTML = '';
  if (!doc) return;
  doc.layout.widgets.forEach((w, i) => {
    const li = document.createElement('li');
    if (i === selIdx) li.classList.add('sel');
    li.innerHTML = `<span>${i + 1}. ${autoName(w)} <span style="color:var(--dim)">${w.slot || '-'}${w.slotRect ? '*' : ''}</span></span>`;
    const del = document.createElement('span');
    del.className = 'del'; del.textContent = '✕';
    del.addEventListener('click', (e) => { e.stopPropagation(); removeWidget(i); });
    li.appendChild(del);
    li.addEventListener('click', () => select(i));
    ul.appendChild(li);
  });
}

function removeWidget(i) {
  doc.layout.widgets.splice(i, 1);
  if (selIdx === i) selIdx = -1;
  else if (selIdx > i) selIdx--;
  afterEdit();
  buildLayers();
}

// ---- 数据源卡（协议 §5：声明即生效，整点逐源拉数；id 净化后作设备 per-id 缓存名）----
function srcParamText(ds) {
  return ds.type === 'github.repo'
    ? `${ds.params?.owner || ''}/${ds.params?.repo || ''}`
    : (ds.params?.user || '');
}

function renderSources() {
  const box = $e('src-list');
  box.innerHTML = '';
  if (!doc) return;
  (doc.dataSources || []).forEach((ds, i) => {
    const item = document.createElement('div');
    item.className = 'src-item';
    const row1 = document.createElement('div');
    row1.className = 'src-row';
    const idIn = document.createElement('input');
    idIn.type = 'text'; idIn.value = ds.id;
    idIn.title = '数据源 id（挂件 source 引用它；设备端净化为缓存名）';
    idIn.addEventListener('change', () => {
      const nv = idIn.value.trim().replace(/\s+/g, '_');
      if (!nv) { msg('数据源 id 不能为空'); idIn.value = ds.id; return; }
      if (nv !== ds.id && doc.dataSources.some((d) => d.id === nv)) {
        msg(`数据源 id「${nv}」已存在`);
        idIn.value = ds.id; return;
      }
      // 级联改名：引用旧 id 的挂件一并指向新 id
      for (const w of doc.layout.widgets) if (w.source === ds.id) w.source = nv;
      ds.id = nv;
      afterEdit();
      renderSources();
    });
    const typeSel = document.createElement('select');
    typeSel.title = '数据源类型';
    for (const [v, t] of [['github.user', '用户'], ['github.repo', '仓库']])
      typeSel.add(new Option(t, v));
    typeSel.value = ds.type === 'github.repo' ? 'github.repo' : 'github.user';
    typeSel.addEventListener('change', () => {
      ds.type = typeSel.value;
      ds.params = {};  // 参数按类型重填
      afterEdit();
      renderSources();
    });
    const del = document.createElement('button');
    del.type = 'button'; del.className = 'flat'; del.textContent = '✕';
    del.title = '删除数据源';
    del.addEventListener('click', () => {
      const used = doc.layout.widgets.filter((w) => w.source === ds.id);
      if (used.length) {
        msg(`「${ds.id}」被 ${used.length} 个挂件引用（${used.map(autoName).join('、')}），先在属性面板改绑再删除`);
        return;
      }
      doc.dataSources.splice(i, 1);
      afterEdit();
      renderSources();
    });
    row1.append(idIn, typeSel, del);
    const row2 = document.createElement('div');
    row2.className = 'src-row';
    // 参数固定格式：仓库源 = owner / repo 两框（常驻斜杠分隔），用户源 = 单框；
    // owner 框兼容整段粘贴（https://github.com/o/r 或 o/r 自动拆分填充）
    const mkParam = (key, ph, title) => {
      const i = document.createElement('input');
      i.type = 'text'; i.value = ds.params?.[key] || '';
      i.placeholder = ph;
      if (title) i.title = title;
      i.addEventListener('change', () => {
        let v = i.value.trim();
        if (key === 'owner') {
          // 粘贴容错：github.com 链接或 owner/repo 整段 → 拆开填两框
          const m = v.match(/github\.com\/([^/\s]+)(?:\/([^/\s#?]+))?/i);
          if (m) {
            v = m[1];
            if (m[2] && !ds.params?.repo) {
              ds.params = { ...ds.params, repo: m[2] };
              repoIn.value = m[2];
            }
          } else if (v.includes('/')) {
            const [a, b] = v.split('/');
            v = a;
            if (b && !ds.params?.repo) {
              ds.params = { ...ds.params, repo: b };
              repoIn.value = b;
            }
          }
        }
        if (!v) { msg('参数不能为空'); i.value = ds.params?.[key] || ''; return; }
        ds.params = { ...ds.params, [key]: v };
        i.value = v;
        afterEdit();
      });
      return i;
    };
    let repoIn = null;
    if (ds.type === 'github.repo') {
      const oIn = mkParam('owner', 'owner（可贴仓库链接）');
      const slash = document.createElement('span');
      slash.textContent = '/';
      slash.style.cssText = 'color:var(--dim);flex:none;padding:0 1px';
      repoIn = mkParam('repo', 'repo');
      row2.append(oIn, slash, repoIn);
    } else {
      row2.appendChild(mkParam('user', 'GitHub 用户名'));
    }
    item.append(row1, row2);
    box.appendChild(item);
  });
}

$e('btn-src-add').addEventListener('click', () => {
  if (!doc) return;
  doc.dataSources = doc.dataSources || [];
  let n = 1, id;
  do { id = `src${n++}`; } while (doc.dataSources.some((d) => d.id === id));
  doc.dataSources.push({ id, type: $e('src-add-type').value, params: {} });
  afterEdit();
  renderSources();
});

// ---- 属性面板（按 widget 类型出字段）----
const TYPE_LABEL = { clock: '时钟', date: '日期', calendar: '日历', stats: '统计',
                     barChart: '柱状图', heatMap: '热力图', text: '文字', image: '图片',
                     repo: '仓库标识', title: '标题字标', ticker: '滚动条' };

function buildProps() {
  const box = $e('props');
  box.innerHTML = '';
  $e('prop-title').textContent = selIdx >= 0
      ? `· ${doc.layout.widgets[selIdx].type}（${TYPE_LABEL[doc.layout.widgets[selIdx].type] || ''}）`
      : '（未选中）';
  if (selIdx < 0 || !doc) return;
  const w = doc.layout.widgets[selIdx];
  const add = (label, input) => {
    const l = document.createElement('label'); l.textContent = label;
    box.appendChild(l); box.appendChild(input);
  };
  const text = (val, cb, ph) => {
    const i = document.createElement('input'); i.type = 'text'; i.value = val ?? '';
    if (ph) i.placeholder = ph;
    i.addEventListener('change', () => cb(i.value));
    return i;
  };
  const num = (val, cb) => {
    const i = document.createElement('input'); i.type = 'number'; i.value = val;
    i.addEventListener('change', () => cb(+i.value));
    return i;
  };
  const sel = (val, opts, cb) => {
    const s = document.createElement('select');
    for (const [v, t] of opts) {
      const o = document.createElement('option'); o.value = v; o.textContent = t;
      if (v === val) o.selected = true;
      s.appendChild(o);
    }
    s.addEventListener('change', () => cb(s.value));
    return s;
  };
  const chk = (val, cb) => {
    const i = document.createElement('input'); i.type = 'checkbox'; i.checked = !!val;
    i.addEventListener('change', () => cb(i.checked));
    return i;
  };

  // 几何四件套
  const sr = w.slotRect || rectOf(w);
  const geo = (key) => text(String(sr[key]), (v) => {
    w.slotRect = { ...sr };
    w.slotRect[key] = Math.max(0, +v | 0);
    afterEdit();
  });
  add('x', geo('x'));
  add('y', geo('y'));
  add('宽', geo('w'));
  add('高', geo('h'));

  // 名称（协议 v1.2 可选字段 name：图层显示用，设备端忽略；留空 = 按内容自动派生）
  const nameInput = text(w.name || '', (v) => {
    if (v.trim()) w.name = v.trim();
    else delete w.name;
    buildLayers();
    dirty = true;
  }, '留空自动命名');
  nameInput.addEventListener('input', () => { if (!nameInput.value) { delete w.name; buildLayers(); } });
  add('名称', nameInput);

  // 按类型
  const alignSel = (v) => sel(v || 'center', [['left', '左'], ['center', '中'], ['right', '右']],
      (nv) => { w.align = nv; afterEdit(); });
  const colorSel = (v) => sel(v || 'black', [['black', '黑'], ['red', '红'], ['yellow', '黄']],
      (nv) => { w.color = nv; afterEdit(); });
  // 数据源绑定（协议 §5）：绑定 → 读该源 per-id 缓存；缺省 → 全局缓存（首个声明源口径）
  const srcSel = () => {
    const opts = [['', '全局（首个声明源）'],
                  ...(doc.dataSources || []).map((d) => [d.id,
                    `${d.id} · ${d.type === 'github.repo'
                      ? `${d.params?.owner || '?'}/${d.params?.repo || '?'}`
                      : (d.params?.user || '?')}`])];
    return sel(w.source || '', opts, (v) => {
      if (v) w.source = v;
      else delete w.source;
      afterEdit();
    });
  };
  if (w.type === 'clock') {
    add('对齐', alignSel(w.align));
  } else if (w.type === 'date') {
    add('对齐', alignSel(w.align));
    add('字号', sel(w.size || 'm', [['s', '小'], ['m', '中'], ['l', '大']],
        (v) => { w.size = v; afterEdit(); }));
    add('颜色', colorSel(w.color));
  } else if (w.type === 'calendar') {
    // 纯几何组件（整月/周日历按槽高自适应），无专有字段
  } else if (w.type === 'repo') {
    add('对齐', alignSel(w.align));
    add('颜色', colorSel(w.color));
    add('数据源', srcSel());
  } else if (w.type === 'title') {
    add('文本', text(w.text || '', (v) => {
      if (v.trim()) w.text = v.trim();
      else delete w.text;
      afterEdit();
    }, '留空 = Whale-Dock（内置字标）'));
    add('颜色', colorSel(w.color));
    add('锁宽高比', chk(ratioLock.has(w), (v) => {
      v ? ratioLock.add(w) : ratioLock.delete(w);
      syncMoveable();
    }));
  } else if (w.type === 'stats') {
    add('变体', sel(w.variant || 'chips', [['chips', '卡片'], ['list', '列表']],
        (v) => { w.variant = v; afterEdit(); }));
    add('标签', text((w.labels || []).join(', '), (v) => {
      w.labels = v.split(',').map((s) => s.trim()).filter(Boolean);
      afterEdit();
    }, '逗号分隔'));
    add('数值', text((w.values || []).join(', '), (v) => {
      const vals = v.split(',').map((s) => s.trim());
      if (vals.some(Boolean)) w.values = vals;
      else delete w.values;
      afterEdit();
    }, '留空 = 实时 GitHub 数据'));
    add('颜色', colorSel(w.color));
    add('数据源', srcSel());
  } else if (w.type === 'barChart') {
    add('标题', text(w.title || '', (v) => { w.title = v; afterEdit(); }));
    add('目标线', num(w.goal || 0, (v) => { w.goal = v; afterEdit(); }));
    add('数据源', srcSel());
  } else if (w.type === 'heatMap') {
    add('标题', text(w.title || '', (v) => { w.title = v; afterEdit(); }));
    add('数据源', srcSel());
  } else if (w.type === 'text' || w.type === 'ticker') {
    add('文本', text(w.text || '', (v) => { w.text = v; afterEdit(); }));
    add('字号', sel(w.size || 'm', [['s', '小'], ['m', '中'], ['l', '大']],
        (v) => { w.size = v; afterEdit(); }));
    add('对齐', alignSel(w.align));
    add('颜色', colorSel(w.color));
  } else if (w.type === 'image') {
    // 资源下拉：内置 whale_pixel + 布局内联资源；旁路导入按钮接文件选择
    const resOpts = [['whale_pixel', 'whale_pixel（内置）'],
                     ...(doc.resources || []).map((r) => [r.id, `${r.id}（${r.w}×${r.h}）`])];
    add('资源', sel(w.resBW || 'whale_pixel', resOpts, (v) => { w.resBW = v; afterEdit(); }));
    const btn = document.createElement('button');
    btn.type = 'button'; btn.className = 'flat'; btn.textContent = '导入图片…';
    btn.addEventListener('click', () => pickImage(selIdx));
    add('本地图片', btn);
    add('二值化', sel(binarizeMode, [['dither', 'Bayer 抖动'], ['otsu', 'Otsu 阈值']],
        (v) => { binarizeMode = v; }));
    add('锁宽高比', chk(ratioLock.has(w), (v) => {
      v ? ratioLock.add(w) : ratioLock.delete(w);
      syncMoveable();
    }));
  }
}

// ---- 添加 widget ----
$e('btn-add').addEventListener('click', () => {
  if (!doc) return;
  const type = $e('add-type').value;
  const base = {
    slotRect: { x: 40, y: 40, w: 300, h: 160 }, type,
  };
  if (type === 'clock') Object.assign(base, { slotRect: { x: 524, y: 14, w: 264, h: 62 }, align: 'right' });
  if (type === 'date') Object.assign(base, { slotRect: { x: 40, y: 40, w: 264, h: 26 }, size: 'm', align: 'left' });
  if (type === 'calendar') Object.assign(base, { slotRect: { x: 40, y: 40, w: 264, h: 62 } });
  if (type === 'title') Object.assign(base, { slotRect: { x: 40, y: 40, w: 300, h: 46 } });
  if (type === 'repo') Object.assign(base, { slotRect: { x: 40, y: 40, w: 330, h: 22 }, align: 'left' });
  if (type === 'stats') Object.assign(base, { variant: 'chips', fields: ['public_repos', 'stars', 'forks', 'followers'],
                                              labels: ['Repositories', 'Stars', 'Forks', 'Followers'] });
  if (type === 'heatMap' || type === 'barChart') Object.assign(base, { source: doc.dataSources?.[0]?.id || 'repo' });
  if (type === 'image') Object.assign(base, { slotRect: { x: 40, y: 40, w: 214, h: 196 }, resBW: 'whale_pixel' });
  if (type === 'text') Object.assign(base, { text: '双击属性面板修改文字', size: 'm', align: 'center' });
  doc.layout.widgets.push(base);
  selIdx = doc.layout.widgets.length - 1;
  afterEdit();
  buildLayers();
});

// ---- 导出 / 加载示例 / 删除 ----
$e('btn-load-sample').addEventListener('click', () => {
  doc = defaultDoc();
  selIdx = -1;
  renderSources();
  afterEdit();
  buildLayers();
  msg('已加载示例布局');
});

$e('btn-del').addEventListener('click', () => {
  if (selIdx >= 0) removeWidget(selIdx);
});

$e('btn-export').addEventListener('click', () => {
  if (!doc) return;
  const dev = prepareForDevice(doc);   // 自定义标题文字 → 设备端内联位图资源
  const v = validate(dev);  // 与固件同源的校验器
  if (v.errors.length) {
    msg('校验未通过：\n' + v.errors.join('\n'));
    return;
  }
  const json = JSON.stringify(dev, null, 2);
  const blob = new Blob([json], { type: 'application/json' });
  const a = document.createElement('a');
  a.href = URL.createObjectURL(blob);
  a.download = 'layout.json';
  a.click();
  URL.revokeObjectURL(a.href);
  msg(`校验通过 ✓ 已下载 layout.json（${json.length}B${dev.resources.length ? `，含 ${dev.resources.length} 内联资源` : ''}）`);
});

// ---- 中文字库子集（协议 §6）----
// 打包器 buildFontSubset / FONT_CELL / b64Of 定义已迁至 sim.js：
// 预览与设备共用一份位图（drawCnText 同算法绘制），编辑器此处仅按
// prepareForDevice 收集用字并引用，保证推送内容与预览所见一致。

// ---- 设备交付准备：自定义标题文字 → 栅格化 1bpp 内联资源 ----
// 预览端用斜体字体实时渲染，设备端没有艺术字体——导出/推送前把自定义文字
// 按标题槽 2x 渲染、50% 阈值二值化、MSB-first 打包为资源，title.resBW 指向之
// （默认 "Whale-Dock" 走固件内置 Corsiva 位图，无需资源）。
function rasterizeTitle(w) {
  const sr = w.slotRect || rectOf(w);
  const cw = Math.max(40, sr.w) * 2, ch = Math.max(16, sr.h) * 2;
  const c = document.createElement('canvas');
  c.width = cw; c.height = ch;
  const g = c.getContext('2d');
  let px = Math.floor(ch * 0.9);
  g.textBaseline = 'middle'; g.textAlign = 'center';
  for (;;) {
    g.font = `italic 700 ${px}px "Monotype Corsiva", Gabriola, "Segoe Script", Georgia, italic serif`;
    if (g.measureText(w.text).width <= cw - 8 || px <= 10) break;
    px -= 2;
  }
  g.fillStyle = '#000';
  g.fillText(w.text, cw / 2, ch / 2);
  const w1 = Math.floor(cw / 2), h1 = Math.floor(ch / 2);
  const d = g.getImageData(0, 0, cw, ch).data;
  const stride = Math.ceil(w1 / 8);
  const bytes = new Uint8Array(stride * h1);
  for (let y = 0; y < h1; y++)
    for (let x = 0; x < w1; x++) {
      const i = (Math.floor(y * 2) * cw + Math.floor(x * 2)) * 4;
      if ((d[i] + d[i + 1] + d[i + 2]) / 3 < 128)
        bytes[y * stride + (x >> 3)] |= 0x80 >> (x & 7);
    }
  return { id: null, w: w1, h: h1, data: b64Of(bytes) };
}

function prepareForDevice(src) {
  const out = JSON.parse(JSON.stringify(src));
  out.resources = (out.resources || []).filter((r) => !/^title_raster/.test(r.id));
  out.fonts = [];  // 中文字库子集按需重建（协议 §6）
  let n = 0;
  for (const w of out.layout.widgets) {
    if (w.type === 'title' && w.text && w.text !== 'Whale-Dock') {
      const res = rasterizeTitle(w);
      res.id = `title_raster_${++n}`;
      out.resources.push(res);
      w.resBW = res.id;
    } else if (w.type === 'title') {
      delete w.resBW;  // 默认文字走固件内置位图
    }
    // 文本类组件含非 ASCII（中文等）→ 生成字库子集并挂 font 引用
    if ((w.type === 'text' || w.type === 'ticker') && w.text &&
        /[^\x00-\x7F]/.test(w.text)) {
      const cellH = FONT_CELL[w.size || 'm'] || FONT_CELL.m;
      const id = `cn${cellH}`;
      if (!out.fonts.find((f) => f.id === id)) {
        const all = out.layout.widgets
          .filter((x) => (x.type === 'text' || x.type === 'ticker') && x.text &&
                          (FONT_CELL[x.size || 'm'] || FONT_CELL.m) === cellH)
          .map((x) => x.text).join('');
        out.fonts.push({ id, size: cellH, glyphs: buildFontSubset(all, cellH) });
      }
      w.font = id;
    } else if (w.type === 'text' || w.type === 'ticker') {
      delete w.font;  // 纯 ASCII 用设备默认 5x7
    }
  }
  return out;
}

// ---- USB 探测（Web Serial 连设备发 CLI ip+key 指令，地址与配对码一键填齐）----
// 设备回显：[IP] state=online ip=192.168.x.x mdns=whaledock-XXXXXX.local
//          [配对码] 123456（HTTP 写操作须带请求头 X-Device-Key: 123456）
// 换芯片 = 换 MAC = 自动换 mDNS 名与配对码，页面不写死任何设备信息。
async function probeDevice() {
  if (!('serial' in navigator)) {
    msg('此浏览器不支持 Web Serial（需 Chrome/Edge，且页面须 localhost 或 HTTPS）');
    return;
  }
  let port;
  try {
    // 先按乐鑫厂商 ID 过滤（VID 0x303A = ESP32 原生 USB）——选择器只列设备的
    // 原生串口，避开同名"USB 串行设备"的桥接口（CLI 只在原生口应答）；
    // 无匹配端口时退回不过滤（老接线/仅桥接口场景）
    try {
      port = await navigator.serial.requestPort({ filters: [{ usbVendorId: 0x303A }] });
    } catch (e) {
      port = await navigator.serial.requestPort();
    }
    await port.open({ baudRate: 115200 });
  } catch (e) {
    msg('未选择串口或打开失败：' + e.message);
    return;
  }
  msg('已连接，查询 IP/mDNS/配对码…');
  try {
    const writer = port.writable.getWriter();
    await writer.write(new TextEncoder().encode('ip\r\nkey\r\n'));
    writer.releaseLock();
    const reader = port.readable.getReader();
    const timer = setTimeout(() => reader.cancel().catch(() => {}), 4000);
    let buf = '';
    const dec = new TextDecoder();
    for (;;) {
      const { value, done } = await reader.read();
      if (done) break;
      buf += dec.decode(value);
      if (buf.includes('mdns=') && buf.includes('[配对码]')) break;
    }
    clearTimeout(timer);
    reader.releaseLock().catch(() => {});
    await port.close();
    const m = buf.match(/\[IP\]\s*state=(\w+)\s+ip=(\S+)\s+mdns=(\S+)/);
    const k = buf.match(/\[配对码\]\s*(\d{6})/);
    if (k) $e('dev-key').value = k[1];
    if (m && m[1] === 'online') {
      $e('dev-addr').value = m[3];
      msg(`✓ 设备在线：${m[3]}（IP ${m[2]}）${k ? ' 与配对码' : ''}已自动填入，可直接「推送上屏」`);
    } else if (m) {
      msg(`设备未联网（state=${m[1]}，IP ${m[2]}），请先在工作台配网${k ? '；配对码已填入' : ''}`);
    } else {
      msg('未收到 [IP] 应答——选的是鲸屿设备串口吗？回显：' + buf.slice(0, 60));
    }
  } catch (e) {
    msg('探测失败：' + e.message + '（若串口被监视器占用请先断开）');
    try { await port.close(); } catch (_) { /* 已关闭 */ }
  }
}
$e('btn-probe').addEventListener('click', probeDevice);

// ---- 推送上屏（固件 POST /api/layout：校验 → 落盘 → ?render=1 即时渲染）----
$e('btn-push').addEventListener('click', async () => {
  const addr = $e('dev-addr').value.trim();
  const key = $e('dev-key').value.trim();
  if (!addr) { msg('请填设备地址'); return; }
  if (!doc) return;
  const v = validate(doc);
  if (v.errors.length) { msg('校验未通过，不能上屏：\n' + v.errors.join('\n')); return; }
  const json = JSON.stringify(prepareForDevice(doc));  // 自定义标题 → 内联位图资源
  msg(`推送中（${(json.length / 1024).toFixed(1)}KB）…`);
  try {
    const res = await fetch(`http://${addr}/api/layout?render=1`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', 'X-Device-Key': key },
      body: json,
    });
    const out = await res.json();
    if (out.ok) {
      msg(`✓ ${out.msg}（widgets ${out.widgets} · 数据源 ${out.sources}）`);
      stampSeal(true);   // 印章确认（潮汐）：成功盖「已上屏」屏红印
    } else {
      msg('✗ 设备校验未通过：\n' + (out.errors || [out.msg || res.status]).join('\n'));
      stampSeal(false);
    }
  } catch (e) {
    msg('推送失败：' + e.message +
        '（检查设备地址/同一局域网/配对码；线上版页面推局域网设备需浏览器允许本地网络访问）');
    stampSeal(false);
  }
});

// 盖印：成功 = 屏红「已上屏」；失败 = 墨黑「未上屏」（潮汐）
function stampSeal(ok) {
  const seal = $e('push-seal');
  if (!seal) return;
  seal.classList.remove('show', 'fail');
  seal.innerHTML = ok ? '已<br>上<br>屏' : '未<br>上<br>屏';
  if (!ok) seal.classList.add('fail');
  void seal.offsetWidth;  // 重启动画
  seal.classList.add('show');
}

// ---- 图片导入：文件 → 缩放（守协议附录 B 单资源 16KB 预算）→ Bayer 抖动/Otsu
// 阈值二值化 → 1bpp MSB-first 内联资源（协议：黑白图抖动由编辑器完成） ----
const imgFile = document.createElement('input');
imgFile.type = 'file'; imgFile.accept = 'image/*'; imgFile.style.display = 'none';
document.body.appendChild(imgFile);
let importTarget = -1;         // 导入目标 widget 下标
let binarizeMode = 'dither';   // dither | otsu（导入期选项，编辑器侧状态，不入协议）

// b64Of 定义在 sim.js（编辑器页与模拟器页共用；此处勿再声明，const 跨脚本重名会抛错）

function pickImage(i) {
  importTarget = i;
  imgFile.value = '';
  imgFile.click();
}

imgFile.addEventListener('change', () => {
  const f = imgFile.files && imgFile.files[0];
  if (!f || importTarget < 0) return;
  const url = URL.createObjectURL(f);
  const img = new Image();
  img.onload = () => { URL.revokeObjectURL(url); importImage(img, f.name); };
  img.src = url;
});

function importImage(img, fileName) {
  const MAX_BYTES = 16000;
  let w = img.naturalWidth, h = img.naturalHeight;
  const scale = Math.min(1, Math.sqrt((MAX_BYTES * 8) / (w * h)));  // 超预算按面积等比缩
  w = Math.max(1, Math.round(w * scale));
  h = Math.max(1, Math.round(h * scale));
  const c = document.createElement('canvas');
  c.width = w; c.height = h;
  const g = c.getContext('2d');
  g.fillStyle = '#fff';                 // 透明底铺白（四色屏无透明）
  g.fillRect(0, 0, w, h);
  g.drawImage(img, 0, 0, w, h);
  const px = g.getImageData(0, 0, w, h).data;
  const gray = (x, y) => { const i = (y * w + x) * 4; return 0.299 * px[i] + 0.587 * px[i + 1] + 0.114 * px[i + 2]; };
  // Bayer 4×4 有序抖动（默认，照片/渐变友好）或 Otsu 全局阈值（logo/线稿友好）
  const BAYER = [0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5].map((v) => (v + 0.5) / 16);
  let thr = 128, how = 'Bayer 抖动';
  if (binarizeMode === 'otsu') {
    const hist = new Array(256).fill(0);
    for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) hist[gray(x, y) | 0]++;
    let sum = 0; for (let i = 0; i < 256; i++) sum += i * hist[i];
    let sumB = 0, nB = 0, bestV = -1;
    for (let t = 0; t < 256; t++) {
      nB += hist[t]; if (!nB) continue;
      const nF = w * h - nB; if (!nF) break;
      sumB += t * hist[t];
      const v = nB * nF * ((sumB / nB) - ((sum - sumB) / nF)) ** 2;
      if (v > bestV) { bestV = v; thr = t; }
    }
    how = `Otsu 阈值 ${thr}`;
  }
  const stride = Math.ceil(w / 8);
  const bytes = new Uint8Array(stride * h);
  for (let y = 0; y < h; y++)
    for (let x = 0; x < w; x++) {
      const ink = binarizeMode === 'dither'
        ? gray(x, y) < BAYER[(y % 4) * 4 + (x % 4)] * 256
        : gray(x, y) < thr;
      if (ink) bytes[y * stride + (x >> 3)] |= 0x80 >> (x & 7);
    }
  // 唯一资源 id（文件名净化，冲突自增）
  const baseId = (fileName || '').replace(/\.[^.]+$/, '').replace(/[^a-zA-Z0-9_]/g, '').slice(0, 12) || 'img';
  const ids = new Set((doc.resources || []).map((r) => r.id));
  let id = baseId, n = 0;
  while (ids.has(id)) id = `${baseId}_${++n}`;
  doc.resources = doc.resources || [];
  doc.resources.push({ id, w, h, data: b64Of(bytes) });
  doc.layout.widgets[importTarget].resBW = id;
  afterEdit();
  buildLayers();
  const totalKB = Math.round(doc.resources.reduce((a, r) => a + r.data.length, 0) / 1024);
  msg(`已导入 ${id}（${w}×${h}，${how}；资源合计 ${totalKB}KB${totalKB > 150 ? '，超设备布局预算 150KB，建议删减' : ''}）`);
}

// ---- 消息 ----
let msgTimer = null;
function msg(s) {
  $e('ed-msg').textContent = s;
  clearTimeout(msgTimer);
  msgTimer = setTimeout(() => { $e('ed-msg').textContent = ''; }, 8000);
}

// ---- 初始化：优先恢复自动存档（刷新/误关不丢工作），否则加载示例 ----
doc = defaultDoc();
renderSources();
if (!restoreAutosave()) {
  afterEdit();
  buildLayers();
  msg('编辑器就绪：拖拽移动、边角缩放、左选图层、右侧改属性');
} else {
  msg('已恢复上次编辑的布局（自动存档）；「加载示例布局」可重置');
}

// ---- JSON 调试卡：与编辑文档互转（sim.js 的渲染/校验按钮在卡内自 bind）----
$e('btn-json-load').addEventListener('click', () => {
  $e('json').value = JSON.stringify(doc, null, 2);
  msg('已将当前编辑布局载入 JSON 调试区');
});
$e('btn-json-apply').addEventListener('click', () => {
  if (loadLayoutFromText($e('json').value)) msg('已应用为当前编辑布局（校验通过）');
});

// 统一控制台切回布局页签时同步 Moveable 控制框（隐藏期间几何可能过期）
window.__editorOnTabShow = () => { if (mv) mv.updateRect(); };
