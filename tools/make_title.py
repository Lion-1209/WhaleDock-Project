# -*- coding: utf-8 -*-
"""生成 Whale-Dock 艺术体标题字标 1bpp 位图（固件 / 模拟器三方同源）。

用法：python tools/make_title.py [字体文件 ...]
默认对比 Gabriola / Segoe Script Bold / Ink Free 三候选，输出到 build/title/：
  preview-<name>.png          目视挑选用
  title_wordmark.h/.cpp       固件 resources（仅 --emit 时写出）
  title_art.js                console/js（仅 --emit 时写出）

流程：6x 超采样渲染 → 盒式降采样 → 45% 阈值二值化 → 紧裁剪（含 2px 边距）
     → MSB-first 按行打包。目标墨高 ~44px（编辑器默认槽 300x46 原生比例 1:1）。
"""
import base64
import os
import sys

from PIL import Image, ImageDraw, ImageFont

TEXT = "Whale-Dock"
TARGET_H = 44          # 目标墨高（px）
SS = 6                 # 超采样倍数
PAD = 2                # 裁剪边距
THRESH = 0.45          # 二值化阈值（覆盖率）
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "build", "title")
# Windows 系统字体目录（经 WINDIR 环境变量拼接，不落绝对盘符）
FONTS = os.path.join(os.environ.get("WINDIR") or os.environ.get("SystemRoot") or ".", "Fonts")


def font(name):
    return os.path.join(FONTS, name)


CANDIDATES = [
    ("gabriola", font("Gabriola.ttf")),
    ("segoescb", font("segoescb.ttf")),
    ("inkfree", font("Inkfree.ttf")),
]


def ink_bbox(img):
    """白底黑字的墨迹包围盒（getbbox 吃白底全图，须先反转）。"""
    return img.point(lambda v: 255 - v).getbbox()


def render(font_path):
    """返回二值位图 (list[str] '0'/'1')，墨高约 TARGET_H。"""
    # 先小尺寸探测墨高，再按比例放大到目标
    probe = ImageFont.truetype(font_path, 200)
    img = Image.new("L", (2000, 600), 255)
    d = ImageDraw.Draw(img)
    d.text((100, 100), TEXT, font=probe, fill=0)
    bb = ink_bbox(img)
    ink_h = bb[3] - bb[1]
    size = int(200 * TARGET_H * SS / ink_h)
    font = ImageFont.truetype(font_path, size)
    # 画布留足余量
    big = Image.new("L", (size * len(TEXT), size * 3), 255)
    d = ImageDraw.Draw(big)
    d.text((size, size), TEXT, font=font, fill=0)
    big = big.crop(ink_bbox(big))

    w6, h6 = big.size
    w, h = w6 // SS, h6 // SS
    # 盒式降采样（灰度平均）→ 阈值
    small = big.resize((w, h), Image.BOX)
    px = small.load()
    rows = []
    for y in range(h):
        rows.append("".join("1" if px[x, y] < int(255 * THRESH) else "0" for x in range(w)))
    return rows


def crop_pad(rows, pad):
    """紧裁剪后加 pad 白边。"""
    ys = [y for y, r in enumerate(rows) if "1" in r]
    xs = [x for r in rows for x, c in enumerate(r) if c == "1"]
    y0, y1, x0, x1 = ys[0], ys[-1], min(xs), max(xs)
    cut = [r[x0:x1 + 1] for r in rows[y0:y1 + 1]]
    h, w = len(cut), len(cut[0])
    out = [["0"] * (w + 2 * pad) for _ in range(h + 2 * pad)]
    for y in range(h):
        out[pad + y][pad:pad + w] = list(cut[y])
    return ["".join(r) for r in out]


def pack(rows):
    """MSB-first 按行打包 → (w, h, bytes)。"""
    w, h = len(rows[0]), len(rows)
    stride = (w + 7) // 8
    buf = bytearray(stride * h)
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c == "1":
                buf[y * stride + (x >> 3)] |= 0x80 >> (x & 7)
    return w, h, bytes(buf)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    fonts = []
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if args:
        fonts = [(os.path.splitext(os.path.basename(p))[0], p) for p in args]
    else:
        fonts = [(n, p) for n, p in CANDIDATES if os.path.exists(p)]

    results = []
    for name, path in fonts:
        rows = crop_pad(render(path), PAD)
        w, h, data = pack(rows)
        # PNG 预览（放大 2x 目视用）
        img = Image.new("1", (w, h), 1)
        for y, r in enumerate(rows):
            for x, c in enumerate(r):
                if c == "1":
                    img.putpixel((x, y), 0)
        img.resize((w * 2, h * 2), Image.NEAREST).save(
            os.path.join(OUT_DIR, f"preview-{name}.png"))
        results.append((name, w, h, data))
        print(f"{name}: {w}x{h}, {len(data)} B, {len(base64.b64encode(data))} b64chars")

    if "--emit" in sys.argv:
        # 以最后一个候选为准（挑选后把中选字体作为唯一参数重跑 --emit）
        name, w, h, data = results[-1]
        stride = (w + 7) // 8
        lines = []
        for i in range(0, len(data), 16):
            lines.append("    " + ", ".join(f"0x{b:02X}" for b in data[i:i + 16]) + ",")
        cpp = (
            '// Whale-Dock 艺术体标题字标 1bpp 位图（MSB-first 按行打包）\n'
            f'// 由 tools/make_title.py 生成（字体 {name}，{w}x{h}，勿手改）；\n'
            '// 同源产物 console/js/title_art.js，两者必须一起更新。\n'
            '#include "title_wordmark.h"\n\n'
            'namespace title_wordmark {\n'
            f'const uint8_t BITS[BYTES] = {{\n' + "\n".join(lines).rstrip(",") + "\n};\n"
            "}  // namespace title_wordmark\n"
        )
        hdr = (
            '#pragma once\n\n'
            '#include <Arduino.h>\n\n'
            '// Whale-Dock 艺术体标题字标 1bpp 位图（生成物，勿手改；见 title_wordmark.cpp 头注释）\n'
            'namespace title_wordmark {\n'
            f'constexpr int W = {w};\n'
            f'constexpr int H = {h};\n'
            f'constexpr int STRIDE = {stride};\n'
            f'constexpr int BYTES = {len(data)};\n'
            'extern const uint8_t BITS[BYTES];\n'
            "}  // namespace title_wordmark\n"
        )
        js = (
            "// Whale-Dock 艺术体标题字标 1bpp 位图（MSB-first 按行打包）\n"
            f"// 由 tools/make_title.py 生成（字体 {name}，{w}x{h}，勿手改）；\n"
            "// 同源产物 firmware/src/app/resources/title_wordmark.*，两者必须一起更新。\n"
            f"window.TITLE_ART = {{ w: {w}, h: {h}, data: \"{base64.b64encode(data).decode()}\" }};\n"
        )
        with open(os.path.join(OUT_DIR, "title_wordmark.h"), "w", encoding="utf-8") as f:
            f.write(hdr)
        with open(os.path.join(OUT_DIR, "title_wordmark.cpp"), "w", encoding="utf-8") as f:
            f.write(cpp)
        with open(os.path.join(OUT_DIR, "title_art.js"), "w", encoding="utf-8") as f:
            f.write(js)
        print(f"emitted: title_wordmark.{{h,cpp}} + title_art.js (from {name})")


if __name__ == "__main__":
    main()
