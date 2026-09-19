// ============================================================
// 鲸屿 WhaleDock · 配网调机台逻辑
// UI 接线 + 设备状态解析：
//   - 徽标由固件 [WiFi] 日志行驱动
//   - wifi-state 行显性化"设备已存凭据/自动联网"状态
//   - 已授权串口全部在本页下拉框管理（getPorts），仅首次授权
//     需要浏览器系统弹窗（包一层页内引导卡，Chrome 安全模型无法替代）
// ============================================================

import { SerialLink } from './serial.js';

const $ = (id) => document.getElementById(id);
const link = new SerialLink();

// ---- 设备状态（由固件日志驱动） ----
const STATE_TEXT = {
  off: '设备未连接',
  unprovisioned: '未配网',
  connecting: '连接中…',
  connected: '已连接',
  retrying: '断开 · 重连中',
};
let deviceState = 'off';
const wifi = { saved: '', ip: '' };

function setState(s) {
  deviceState = s;
  $('badge').textContent = s === 'connected' && wifi.ip ? `已连接 ${wifi.ip}` : STATE_TEXT[s];
  $('badge').className = 'badge ' + s;
  renderWifiState();
}

// "设备凭据"状态行：连接后明确告知已存凭据与自动联网情况
function renderWifiState() {
  const el = $('wifi-state');
  if (deviceState === 'off') {
    el.textContent = '连接设备后显示凭据状态';
    el.className = 'wifi-state';
    return;
  }
  if (!wifi.saved) {
    el.textContent = '设备未配网——在下方表单填入 Wi-Fi 完成配置';
    el.className = 'wifi-state warn';
    return;
  }
  if (deviceState === 'connected') {
    el.textContent = `设备已存凭据「${wifi.saved}」，开机自动联网 · 当前 IP ${wifi.ip}`;
    el.className = 'wifi-state ok';
  } else {
    el.textContent = `设备已存凭据「${wifi.saved}」，自动连接中…（无需重新配置）`;
    el.className = 'wifi-state';
  }
  if (wifi.saved && !$('ssid').value) $('ssid').value = wifi.saved;  // SSID 回填
}

// ---- 日志面板 ----
function appendLog(text) {
  const div = document.createElement('div');
  if (text.startsWith('[心跳]')) div.className = 'dim';
  else if (text.startsWith('[WiFi]')) div.className = 'wifi';
  else if (text.startsWith('[页面]')) div.className = 'page';
  div.textContent = text === '' ? ' ' : text;
  const log = $('log');
  log.appendChild(div);
  while (log.childElementCount > 500) log.removeChild(log.firstChild);
  if ($('autoscroll').checked) log.scrollTop = log.scrollHeight;
}

// 固件日志行 → 页面状态与卡片渲染（兼容日志/状态查询两种格式）
let scanMode = false;   // [WiFi] 共 N 个 AP： 之后进入，遇到下一行 [ 开头退出
let commitMode = false;  // gh commits 的数字行
const gh = { weeks: [], total: 0 };

function parseLine(line) {
  // ---- 设备时间 ----
  let m = line.match(/^\[时间\] (.+)$/);
  if (m) {
    $('dev-time').textContent = `设备时间：${m[1]}`;
    return;
  }

  // ---- GitHub 三类结果 ----
  if ((m = line.match(/用户 (.+?)：公开仓库 (\d+) · 关注者 (\d+)/))) {
    $('gh-result').textContent = `用户 ${m[1]}：公开仓库 ${m[2]} · 关注者 ${m[3]}`;
    $('gh-result').className = 'wifi-state ok';
    return;
  }
  if ((m = line.match(/仓库 (.+?)：Stars (\d+) · Forks (\d+)/))) {
    $('gh-result').textContent = `仓库 ${m[1]}：Stars ${m[2]} · Forks ${m[3]}`;
    $('gh-result').className = 'wifi-state ok';
    return;
  }
  if (/近 \d+ 周提交/.test(line)) {
    commitMode = true;
    gh.weeks = [];
    gh.total = 0;
    return;
  }
  if (commitMode && /^\s*\d+(\s+\d+)*\s*$/.test(line)) {
    gh.weeks = line.trim().split(/\s+/).map(Number);
    return;
  }
  if ((m = line.match(/合计 (\d+) 次/))) {
    commitMode = false;
    gh.total = Number(m[1]);
    renderGhBars();
    return;
  }
  if ((m = line.match(/^\[GitHub\] 失败：(.+)$/))) {
    $('gh-result').textContent = `失败：${m[1]}`;
    $('gh-result').className = 'wifi-state warn';
    return;
  }
  if (/格式：gh repo/.test(line) || /^\[GitHub\] Wi-Fi 未连接/.test(line)) {
    $('gh-result').textContent = line.replace(/^\[(GitHub|CLI)\] /, '');
    $('gh-result').className = 'wifi-state warn';
    return;
  }

  // ---- Wi-Fi 扫描结果 ----
  if ((m = line.match(/^\[WiFi\] 共 (\d+) 个 AP/))) {
    scanMode = true;
    $('scan-list').innerHTML = '';
    if (Number(m[1]) === 0) $('scan-list').textContent = '（未扫到任何 AP）';
    return;
  }
  if (scanMode && !line.startsWith('[') &&
      (m = line.match(/^\s*\d+\s+ch\s*(\d+)\s+(-?\d+)\s*dBm\s+(.+)$/))) {
    const ssid = m[3].replace(/\s*\(hex:[0-9A-F]*\)\s*$/, '');
    addScanItem(ssid, m[1], m[2]);
    return;
  }
  if (scanMode && line.startsWith('[')) scanMode = false;

  // ---- Wi-Fi 连接状态（原有逻辑） ----
  if (!line.startsWith('[WiFi]')) return;
  if ((m = line.match(/凭据已保存（NVS）：(.+)$/))) {
    wifi.saved = m[1];
  } else if ((m = line.match(/已连接：(\S+)\s+IP=(\S+)/))) {
    wifi.saved = m[1];
    wifi.ip = m[2];
    setState('connected');
  } else if (line.includes('SSID=') && line.includes('已连接')) {
    const s = line.match(/SSID=(\S*)/);
    const ip = line.match(/IP=(\S+)/);
    if (s) wifi.saved = s[1];
    if (ip) wifi.ip = ip[1];
    setState('connected');
  } else if ((m = line.match(/连接中：(.+)$/))) {
    wifi.saved = m[1];
    setState('connecting');
  } else if (line.includes('连接中')) {
    setState('connecting');
  } else if (line.includes('断开')) {
    wifi.ip = '';
    setState('retrying');
  } else if (line.includes('无已存凭据')) {
    wifi.saved = '';
    wifi.ip = '';
    setState('unprovisioned');
  }
  renderWifiState();
}

link.onLine = (line) => { appendLog(line); parseLine(line); };
link.onClose = () => {
  setState('off');
  $('btn-connect').disabled = false;
  $('btn-disconnect').disabled = true;
};

// ---- 已授权串口管理（全部页面内完成） ----
let grantedEntries = [];

const hex4 = (n) => (n ?? 0).toString(16).padStart(4, '0');
const portLabel = (e) =>
  e.info.usbVendorId === 0x303a
    ? `乐鑫设备 ${hex4(e.info.usbVendorId)}:${hex4(e.info.usbProductId)}`
    : `串口设备 ${hex4(e.info.usbVendorId)}:${hex4(e.info.usbProductId)}`;

async function refreshGranted() {
  if (!SerialLink.supported()) return;
  grantedEntries = await SerialLink.grantedPorts();
  const sel = $('port-select');
  const prev = sel.value;
  sel.innerHTML = '';
  grantedEntries.forEach((e, i) => {
    const o = document.createElement('option');
    o.value = String(i);
    o.textContent = portLabel(e);
    sel.appendChild(o);
  });
  $('port-area').hidden = grantedEntries.length === 0;
  if (grantedEntries.length) {
    const keep = prev && [...sel.options].some((o) => o.value === prev);
    if (keep) {
      sel.value = prev;
    } else {
      const esp = SerialLink.espPortOf(grantedEntries);
      sel.value = String(grantedEntries.indexOf(esp));
    }
  }
}

function afterConnected() {
  appendLog('[页面] 串口已打开（115200）。查询设备状态…');
  setState('unprovisioned');
  link.send('wifi status');
  link.send('time');
  $('btn-connect').disabled = true;
  $('btn-disconnect').disabled = false;
}

// 首次授权：页内引导卡 → 浏览器系统弹窗（安全模型无法替代，包一层体验）
function pickAndConnect() {
  return new Promise((resolve, reject) => {
    $('pick-modal').hidden = false;
    $('btn-pick-go').onclick = async () => {
      $('pick-modal').hidden = true;
      try {
        await link.connectViaPicker();
        await refreshGranted();
        resolve();
      } catch (e) {
        reject(e);
      }
    };
    $('btn-pick-cancel').onclick = () => {
      $('pick-modal').hidden = true;
      reject(new Error('已取消'));
    };
  });
}

async function doConnect() {
  if (!SerialLink.supported()) {
    appendLog('[页面] 当前浏览器不支持 Web Serial，请使用 Chrome / Edge / Opera（Chromium 内核）');
    return;
  }
  try {
    if (grantedEntries.length > 0) {
      const e = grantedEntries[Number($('port-select').value)] || grantedEntries[0];
      appendLog(`[页面] 直连已授权串口（${portLabel(e)}）…`);
      await link.connectTo(e);
    } else {
      await pickAndConnect();
    }
    afterConnected();
  } catch (e) {
    if (/已取消/.test(e.message)) {
      appendLog('[页面] 已取消选择');
    } else if (/no port selected/i.test(e.message)) {
      appendLog('[页面] 未选择串口：若没有弹出选择框，是内嵌浏览器不支持串口选择器——请改用 Chrome 或 Edge 打开 http://localhost:8765');
    } else {
      appendLog(`[页面] 连接失败：${e.message}（常见：COM 口被其它程序占用）`);
    }
    await refreshGranted();
  }
}

// ---- 事件 ----
$('btn-connect').addEventListener('click', doConnect);

// 添加/换串口：走一次系统授权框（引导卡包裹），成功后立即连接
$('btn-add').addEventListener('click', async () => {
  if (!SerialLink.supported()) return;
  try {
    await pickAndConnect();
    afterConnected();
  } catch (e) {
    if (!/已取消/.test(e.message)) appendLog(`[页面] 添加失败：${e.message}`);
  }
});

$('btn-disconnect').addEventListener('click', async () => {
  appendLog('[页面] 正在断开…');
  await link.disconnect();
});

$('wifi-form').addEventListener('submit', async (e) => {
  e.preventDefault();
  const ssid = $('ssid').value.trim();
  const pass = $('pass').value;
  if (!ssid || pass.length < 8) return;
  if (ssid.includes(',') || pass.includes(',')) {
    appendLog('[页面] SSID / 密码不能包含英文逗号（串口 CLI 协议限制）');
    return;
  }
  try {
    await link.send(`wifi set ${ssid},${pass}`);
    $('pass').value = '';
  } catch (e) {
    appendLog(`[页面] 发送失败：${e.message}`);
  }
});

const sendCmd = async () => {
  const text = $('cmd').value.trim();
  if (!text) return;
  $('cmd').value = '';
  appendLog(`> ${text}`);
  try { await link.send(text); } catch (e) { appendLog(`[页面] 发送失败：${e.message}`); }
};
$('btn-send').addEventListener('click', sendCmd);
$('cmd').addEventListener('keydown', (e) => { if (e.key === 'Enter') sendCmd(); });

const cmd = (c) => async () => {
  appendLog(`> ${c}`);
  try { await link.send(c); } catch (e) { appendLog(`[页面] 发送失败：${e.message}`); }
};
$('btn-status').addEventListener('click', cmd('wifi status'));
$('btn-clear').addEventListener('click', cmd('wifi clear'));
$('btn-reboot').addEventListener('click', cmd('reboot'));

$('show-pass').addEventListener('change', (e) => {
  $('pass').type = e.target.checked ? 'text' : 'password';
});

// ---- 扫描结果 / 柱状图渲染 ----
function addScanItem(ssid, ch, rssi) {
  const item = document.createElement('div');
  item.className = 'scan-item';
  const name = document.createElement('span');
  name.textContent = ssid;
  const meta = document.createElement('span');
  meta.className = 'meta';
  meta.textContent = `ch${ch} · ${rssi} dBm`;
  item.append(name, meta);
  item.title = '点击回填到配网表单';
  item.addEventListener('click', () => {
    $('ssid').value = ssid;
    appendLog(`[页面] 已回填 SSID：${ssid}`);
  });
  $('scan-list').appendChild(item);
}

function renderGhBars() {
  $('gh-result').textContent =
    `近 ${gh.weeks.length} 周提交（旧→新）：合计 ${gh.total} 次（悬停看每周数值）`;
  $('gh-result').className = 'wifi-state ok';
  const max = Math.max(...gh.weeks, 1);
  const box = $('gh-bars');
  box.innerHTML = '';
  gh.weeks.forEach((w, i) => {
    const b = document.createElement('div');
    b.className = 'bar';
    b.style.height = `${Math.max(4, Math.round((w / max) * 100))}%`;
    b.title = `第 ${i + 1} 周：${w} 次`;
    box.appendChild(b);
  });
}

// 仓库输入规范化：容忍完整 URL / @前缀 / 多余路径，统一为 owner/repo
function normRepo(v) {
  v = v.trim().replace(/^@+/, '');
  v = v.replace(/^(https?:\/\/)?(www\.)?github\.com\//i, '');
  const parts = v.split(/[\/\s]+/).filter(Boolean);
  return parts.length >= 2 ? `${parts[0]}/${parts[1]}` : null;
}

function ghWarn(text) {
  $('gh-result').textContent = text;
  $('gh-result').className = 'wifi-state warn';
}

$('btn-time').addEventListener('click', cmd('time'));
$('btn-scan').addEventListener('click', () => {
  $('scan-list').innerHTML = '<div class="scan-item"><span>扫描中（约 2-5s）…</span></div>';
  cmd('wifi scan')();
});
$('btn-gh-user').addEventListener('click', () => {
  const v = $('gh-user').value.trim().replace(/^@+/, '');
  if (!v) { ghWarn('请输入用户 / 组织登录名'); return; }
  cmd(`gh user ${v}`)();
});
$('btn-gh-repo').addEventListener('click', () => {
  const r = normRepo($('gh-repo').value);
  if (!r) { ghWarn('格式：owner/repo（也可直接粘贴仓库页地址）'); return; }
  $('gh-repo').value = r;  // 规范化回显
  cmd(`gh repo ${r}`)();
});
$('btn-gh-commits').addEventListener('click', () => {
  const r = normRepo($('gh-repo').value);
  if (!r) { ghWarn('格式：owner/repo（也可直接粘贴仓库页地址）'); return; }
  $('gh-repo').value = r;
  cmd(`gh commits ${r}`)();
});

appendLog('[页面] 就绪。已授权过串口的话直接点「连接设备」（免弹框）');
refreshGranted();
