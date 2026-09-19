// ============================================================
// 鲸屿 WhaleDock · Web Serial 封装
// 连接/断开、行式接收（UTF-8）、发送文本行。
//
// 两种连接方式：
//   connectTo(port)   —— 直连已授权端口（navigator.serial.getPorts()，
//                        免弹系统选择框，授权一次后续用）
//   connectViaPicker() —— 浏览器原生选择框（仅首次授权需要）
//
// 断开顺序很关键（Web Serial 经典死锁点）：先把 this.port 置空让读循环
// 退出且不再重新 getReader，等它 releaseLock，最后才 port.close()——
// 直接 cancel 后立刻 close 会被读循环重新抢到的锁卡死。
// ============================================================

const BAUD = 115200;
const USB_VID_ESPRESSIF = 0x303a;  // 原生 USB CDC 的厂商 ID

export class SerialLink {
  constructor() {
    this.port = null;
    this.onLine = () => {};   // (line: string) => void
    this.onClose = () => {};  // () => void 意外/正常断开
    this._reader = null;
    this._readDone = null;    // 读循环完全收尾（已 releaseLock）的 Promise
    this._decoder = new TextDecoder('utf-8');
    this._buf = '';
  }

  static supported() {
    return 'serial' in navigator;
  }

  get connected() {
    return this.port !== null;
  }

  // 已授权端口列表，附展示信息；优先返回乐鑫设备
  static async grantedPorts() {
    if (!SerialLink.supported()) return [];
    const ports = await navigator.serial.getPorts();
    return ports.map((p) => ({ port: p, info: p.getInfo() }));
  }

  static espPortOf(entries) {
    return (entries.find((e) => e.info.usbVendorId === USB_VID_ESPRESSIF) || entries[0] || null);
  }

  async connectTo(entry) {
    const port = entry.port;
    await port.open({ baudRate: BAUD });
    this.port = port;
    this._readDone = this._readLoop();
  }

  async connectViaPicker() {
    let port;
    try {
      port = await navigator.serial.requestPort({ filters: [{ usbVendorId: USB_VID_ESPRESSIF }] });
    } catch (e) {
      if (e.name === 'NotFoundError') {
        port = await navigator.serial.requestPort();  // 放开过滤重选
      } else {
        throw e;  // 用户取消等
      }
    }
    await port.open({ baudRate: BAUD });
    this.port = port;
    this._readDone = this._readLoop();
  }

  async disconnect() {
    if (!this.port) return;
    const port = this.port;
    this.port = null;  // ① 关开关：读循环完成当前读取后不再重启
    try {
      if (this._reader) await this._reader.cancel().catch(() => {});
      if (this._readDone) await this._readDone;  // ② 等读循环释放串口锁
      await port.close().catch(() => {});        // ③ 此时 close 不会被锁卡住
    } finally {
      this._reader = null;
      this._readDone = null;
      this.onClose();
    }
  }

  async send(text) {
    if (!this.port) throw new Error('设备未连接');
    const w = this.port.writable.getWriter();
    try {
      await w.write(new TextEncoder().encode(text + '\n'));
    } finally {
      w.releaseLock();
    }
  }

  async _readLoop() {
    while (this.port && this.port.readable) {
      this._reader = this.port.readable.getReader();
      try {
        for (;;) {
          const { value, done } = await this._reader.read();
          if (done) break;
          this._buf += this._decoder.decode(value, { stream: true });
          let nl;
          while ((nl = this._buf.indexOf('\n')) >= 0) {
            const line = this._buf.slice(0, nl).replace(/\r$/, '');
            this._buf = this._buf.slice(nl + 1);
            this.onLine(line);
          }
        }
      } catch (e) {
        if (this.port) this.onLine(`[页面] 读取中断：${e.message}`);
      } finally {
        this._reader.releaseLock();
        this._reader = null;
      }
    }
    // 循环退出但 port 还在 = 设备拔出（非用户主动断开）
    if (this.port) {
      this.port = null;
      this.onLine('[页面] 设备连接已断开（请重新插拔后连接）');
      this.onClose();
    }
  }
}
