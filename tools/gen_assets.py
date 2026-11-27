#!/usr/bin/env python3
"""Deterministically generates external CC BY 4.0 art, WAVs, shaders, SIL OFL 1.1 Noto font subsets, and manifest.json."""
import json
import math
import os
import random
import struct
import wave
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(ROOT, "assets")


def write_png(path: str, width: int, height: int, rgba: bytearray) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    raw = bytearray()
    stride = width * 4
    for y in range(height):
        raw.append(0)
        raw.extend(rgba[y * stride : (y + 1) * stride])

    def chunk(tag: bytes, data: bytes) -> bytes:
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def blend_px(buf: bytearray, w: int, h: int, x: int, y: int, r: int, g: int, b: int, a: int) -> None:
    if not (0 <= x < w and 0 <= y < h) or a <= 0:
        return
    idx = (y * w + x) * 4
    sa = a / 255.0
    da = buf[idx + 3] / 255.0
    oa = sa + da * (1.0 - sa)
    if oa <= 1e-6:
        return
    buf[idx] = int((r * sa + buf[idx] * da * (1.0 - sa)) / oa)
    buf[idx + 1] = int((g * sa + buf[idx + 1] * da * (1.0 - sa)) / oa)
    buf[idx + 2] = int((b * sa + buf[idx + 2] * da * (1.0 - sa)) / oa)
    buf[idx + 3] = int(min(255, round(oa * 255.0)))


def aa_round_rect(buf: bytearray, w: int, h: int, x0: float, y0: float, rw: float, rh: float, rad: float, col: tuple) -> None:
    cx = x0 + rw * 0.5
    cy = y0 + rh * 0.5
    hx = rw * 0.5 - rad
    hy = rh * 0.5 - rad
    r, g, b, a = col
    for y in range(max(0, int(y0) - 2), min(h, int(y0 + rh) + 3)):
        for x in range(max(0, int(x0) - 2), min(w, int(x0 + rw) + 3)):
            dx = max(abs(x + 0.5 - cx) - hx, 0.0)
            dy = max(abs(y + 0.5 - cy) - hy, 0.0)
            dist = math.hypot(dx, dy) - rad
            cov = max(0.0, min(1.0, 0.5 - dist))
            if cov > 0.0:
                blend_px(buf, w, h, x, y, r, g, b, int(a * cov))


def aa_circle(buf: bytearray, w: int, h: int, cx: float, cy: float, rad: float, col: tuple) -> None:
    r, g, b, a = col
    for y in range(max(0, int(cy - rad) - 2), min(h, int(cy + rad) + 3)):
        for x in range(max(0, int(cx - rad) - 2), min(w, int(cx + rad) + 3)):
            dist = math.hypot(x + 0.5 - cx, y + 0.5 - cy) - rad
            cov = max(0.0, min(1.0, 0.5 - dist))
            if cov > 0.0:
                blend_px(buf, w, h, x, y, r, g, b, int(a * cov))


def generate_sprites() -> None:
    # Modern anti-aliased 512x256 culinary equipment & brand atlas (4x2 tiles of 128x128)
    w, h = 512, 256
    buf = bytearray(w * h * 4)

    # Tile (0,0): Cast-iron skillet with warm highlight & brass rivet
    aa_round_rect(buf, w, h, 10, 10, 108, 108, 22, (32, 40, 56, 255))
    aa_round_rect(buf, w, h, 18, 66, 68, 22, 9, (68, 75, 88, 255))
    aa_round_rect(buf, w, h, 22, 62, 60, 7, 3, (124, 134, 152, 255))
    aa_round_rect(buf, w, h, 82, 71, 30, 10, 5, (92, 100, 116, 255))
    aa_circle(buf, w, h, 106, 76, 3.0, (235, 168, 56, 255))
    aa_circle(buf, w, h, 42, 48, 8.0, (235, 168, 56, 175))
    aa_circle(buf, w, h, 56, 40, 11.0, (245, 118, 56, 165))

    # Tile (1,0): Brushed stainless stockpot with simmering broth
    ox = 128
    aa_round_rect(buf, w, h, ox + 10, 10, 108, 108, 22, (32, 40, 56, 255))
    aa_round_rect(buf, w, h, ox + 28, 44, 72, 52, 12, (162, 174, 192, 255))
    aa_round_rect(buf, w, h, ox + 34, 52, 60, 38, 8, (70, 146, 224, 230))
    aa_round_rect(buf, w, h, ox + 18, 52, 12, 14, 5, (122, 134, 152, 255))
    aa_round_rect(buf, w, h, ox + 98, 52, 12, 14, 5, (122, 134, 152, 255))
    aa_circle(buf, w, h, ox + 52, 66, 5.0, (210, 236, 255, 200))
    aa_circle(buf, w, h, ox + 72, 72, 6.5, (210, 236, 255, 190))
    aa_circle(buf, w, h, ox + 64, 30, 8.0, (215, 228, 244, 120))

    # Tile (2,0): Modern convection oven with glowing hearth
    ox = 256
    aa_round_rect(buf, w, h, ox + 10, 10, 108, 108, 22, (32, 40, 56, 255))
    aa_round_rect(buf, w, h, ox + 22, 24, 84, 80, 12, (76, 86, 104, 255))
    aa_round_rect(buf, w, h, ox + 32, 44, 64, 48, 8, (238, 138, 52, 235))
    aa_round_rect(buf, w, h, ox + 40, 54, 48, 28, 6, (252, 196, 92, 220))
    aa_circle(buf, w, h, ox + 40, 34, 4.0, (218, 226, 238, 255))
    aa_circle(buf, w, h, ox + 54, 34, 4.0, (218, 226, 238, 255))
    aa_circle(buf, w, h, ox + 86, 34, 4.5, (235, 168, 56, 255))

    # Tile (3,0): Precision Sous-Vide water bath & immersion circulator
    ox = 384
    aa_round_rect(buf, w, h, ox + 10, 10, 108, 108, 22, (32, 40, 56, 255))
    aa_round_rect(buf, w, h, ox + 24, 46, 80, 54, 10, (74, 162, 232, 200))
    aa_round_rect(buf, w, h, ox + 32, 20, 18, 72, 7, (54, 62, 78, 255))
    aa_round_rect(buf, w, h, ox + 35, 25, 12, 14, 3, (52, 208, 146, 255))
    aa_round_rect(buf, w, h, ox + 58, 62, 36, 24, 6, (218, 126, 102, 230))

    # Row 1: Flame (0,128), Steam (128,128), Emulsion Bowl (256,128), FatDuck Emblem (384,128)
    oy = 128
    aa_circle(buf, w, h, 64, oy + 72, 28, (238, 104, 44, 245))
    aa_circle(buf, w, h, 64, oy + 78, 17, (252, 208, 78, 255))

    aa_circle(buf, w, h, 180, oy + 74, 18, (214, 228, 245, 160))
    aa_circle(buf, w, h, 202, oy + 58, 22, (214, 228, 245, 140))

    aa_circle(buf, w, h, 320, oy + 72, 34, (238, 202, 104, 250))
    aa_circle(buf, w, h, 320, oy + 66, 24, (250, 226, 144, 255))

    # Modern stylized FatDuck golden emblem
    ox = 384
    aa_circle(buf, w, h, ox + 64, oy + 64, 52, (36, 46, 66, 255))
    aa_circle(buf, w, h, ox + 64, oy + 64, 46, (235, 166, 52, 255))
    aa_circle(buf, w, h, ox + 58, oy + 74, 22, (252, 224, 134, 255))
    aa_circle(buf, w, h, ox + 72, oy + 48, 15, (252, 224, 134, 255))
    aa_round_rect(buf, w, h, ox + 82, oy + 45, 18, 9, 4, (228, 102, 42, 255))
    aa_circle(buf, w, h, ox + 75, oy + 44, 3.2, (20, 26, 38, 255))

    write_png(os.path.join(ASSETS, "graphics", "kitchen_atlas.png"), w, h, buf)

    # Smooth status icons (256x64)
    iw, ih = 256, 64
    ibuf = bytearray(iw * ih * 4)
    aa_circle(ibuf, iw, ih, 32, 32, 22, (52, 199, 133, 255))
    aa_circle(ibuf, iw, ih, 96, 32, 22, (235, 94, 85, 255))
    aa_circle(ibuf, iw, ih, 160, 32, 22, (77, 163, 245, 255))
    aa_circle(ibuf, iw, ih, 224, 32, 22, (235, 166, 52, 255))
    write_png(os.path.join(ASSETS, "graphics", "icons.png"), iw, ih, ibuf)


def generate_fonts() -> None:
    """Subset SIL OFL 1.1 Noto fonts for all 20 world languages if system Noto + fontTools are installed."""
    fonts_dir = os.path.join(ASSETS, "fonts")
    os.makedirs(fonts_dir, exist_ok=True)

    # Remove legacy retro pixel bitmap font if present
    for old_fn in ("fatduck_font.png", "fatduck_font.lua"):
        old_p = os.path.join(fonts_dir, old_fn)
        if os.path.exists(old_p):
            os.remove(old_p)

    try:
        from fontTools import subset  # type: ignore
        from fontTools.ttLib import TTCollection, TTFont  # type: ignore
    except ImportError:
        print("fontTools not installed; keeping committed external Noto fonts in assets/fonts/.")
        return

    noto_dir = "/usr/share/fonts/truetype/noto"
    cjk_ttc = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
    if not os.path.exists(os.path.join(noto_dir, "NotoSans-Regular.ttf")):
        print("System Noto fonts not present; keeping committed external Noto fonts in assets/fonts/.")
        return

    locales_dir = os.path.join(ASSETS, "locales")
    locale_text = {}
    all_text = ""
    if os.path.isdir(locales_dir):
        for fn in sorted(os.listdir(locales_dir)):
            if fn.endswith(".lua"):
                with open(os.path.join(locales_dir, fn), "r", encoding="utf-8") as f:
                    txt = f.read()
                locale_text[fn[:-4]] = txt
                all_text += txt

    def subset_ttf(src_path: str, dst_path: str, unicodes: set, font_number: int = -1) -> None:
        opts = subset.Options()
        opts.layout_features = ["*"]
        opts.hinting = True
        opts.desubroutinize = False
        opts.drop_tables += ["DSIG"]
        if font_number >= 0:
            ttc = TTCollection(src_path)
            font = ttc.fonts[font_number]
        else:
            font = TTFont(src_path)
        sub = subset.Subsetter(options=opts)
        sub.populate(unicodes=unicodes)
        sub.subset(font)
        font.save(dst_path)

    base_cps = set(range(0x0020, 0x007F)) | set(range(0x00A0, 0x0250)) | set(range(0x0400, 0x0500)) | set(range(0x1E00, 0x1F00))
    base_cps |= {0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2026, 0x2190, 0x2192, 0x2260, 0x2264, 0x2265}
    for ch in all_text:
        cp = ord(ch)
        if cp < 0x0500 or (0x1E00 <= cp <= 0x22FF):
            base_cps.add(cp)

    subset_ttf(os.path.join(noto_dir, "NotoSans-Regular.ttf"), os.path.join(fonts_dir, "NotoSans-UI.ttf"), base_cps)
    subset_ttf(os.path.join(noto_dir, "NotoSans-Bold.ttf"), os.path.join(fonts_dir, "NotoSans-Bold.ttf"), base_cps)

    ar_cps = set(range(0x0600, 0x0700)) | set(range(0xFB50, 0xFC00)) | set(range(0xFE70, 0xFEFD)) | {0x0020}
    subset_ttf(os.path.join(noto_dir, "NotoSansArabic-Regular.ttf"), os.path.join(fonts_dir, "NotoSans-Arabic.ttf"), ar_cps)

    dev_cps = set(range(0x0900, 0x0980)) | {0x0020}
    subset_ttf(os.path.join(noto_dir, "NotoSansDevanagari-Regular.ttf"), os.path.join(fonts_dir, "NotoSans-Devanagari.ttf"), dev_cps)

    bn_cps = set(range(0x0980, 0x0A00)) | {0x0020}
    subset_ttf(os.path.join(noto_dir, "NotoSansBengali-Regular.ttf"), os.path.join(fonts_dir, "NotoSans-Bengali.ttf"), bn_cps)

    te_cps = set(range(0x0C00, 0x0C80)) | {0x0020}
    subset_ttf(os.path.join(noto_dir, "NotoSansTelugu-Regular.ttf"), os.path.join(fonts_dir, "NotoSans-Telugu.ttf"), te_cps)

    ta_cps = set(range(0x0B80, 0x0C00)) | {0x0020}
    subset_ttf(os.path.join(noto_dir, "NotoSansTamil-Regular.ttf"), os.path.join(fonts_dir, "NotoSans-Tamil.ttf"), ta_cps)

    if os.path.exists(cjk_ttc):
        cjk_punct = {0x0020, 0x3000, 0x3001, 0x3002, 0x300C, 0x300D, 0xFF01, 0xFF08, 0xFF09, 0xFF0C, 0xFF1A, 0xFF1B, 0xFF1F}
        zh_cps = {ord(c) for c in locale_text.get("zh", "") + "中文简体" if ord(c) >= 0x0500} | cjk_punct
        ja_cps = {ord(c) for c in locale_text.get("ja", "") + "日本語" if ord(c) >= 0x0500} | set(range(0x3041, 0x30FF)) | cjk_punct
        ko_cps = {ord(c) for c in locale_text.get("ko", "") + "한국어" if ord(c) >= 0x0500} | cjk_punct
        subset_ttf(cjk_ttc, os.path.join(fonts_dir, "NotoSans-CJK-SC.otf"), zh_cps, font_number=2)
        subset_ttf(cjk_ttc, os.path.join(fonts_dir, "NotoSans-CJK-JP.otf"), ja_cps, font_number=0)
        subset_ttf(cjk_ttc, os.path.join(fonts_dir, "NotoSans-CJK-KR.otf"), ko_cps, font_number=1)

    ofl_text = """SIL OPEN FONT LICENSE Version 1.1 - 26 February 2007

The Noto Sans and Noto Sans CJK font subsets in this directory are derived from
Google Noto Fonts (https://github.com/notofonts), Copyright (c) Google LLC,
licensed under the SIL Open Font License, Version 1.1 (OFL-1.1).

Permission is hereby granted, free of charge, to any person obtaining a copy of
the Font Software, to use, study, copy, merge, embed, modify, redistribute, and
sell modified and unmodified copies of the Font Software, subject to the full
terms of the SIL Open Font License 1.1: https://openfontlicense.org/
"""
    with open(os.path.join(fonts_dir, "OFL-1.1.txt"), "w", encoding="utf-8") as f:
        f.write(ofl_text)


def write_wav(path: str, samples: list, rate: int = 22050) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with wave.open(path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(rate)
        packed = bytearray()
        for s in samples:
            v = max(-32767, min(32767, int(s * 32767.0)))
            packed.extend(struct.pack("<h", v))
        wf.writeframes(bytes(packed))


def generate_sounds() -> None:
    rng = random.Random(42069)
    rate = 22050
    n_siz = int(rate * 0.6)
    sizzle = []
    prev = 0.0
    for i in range(n_siz):
        white = rng.uniform(-1.0, 1.0)
        hp = white - prev
        prev = white * 0.82
        burst = 1.0 + 1.4 * (1.0 if rng.random() < 0.03 else 0.0)
        env = math.sin(math.pi * min(1.0, i / 400.0)) * math.sin(math.pi * min(1.0, (n_siz - i) / 400.0)) if (i < 400 or i > n_siz - 400) else 1.0
        sizzle.append(max(-0.9, min(0.9, hp * 0.22 * burst * env)))
    write_wav(os.path.join(ASSETS, "sounds", "sizzle_loop.wav"), sizzle, rate)

    boil = []
    phase = 0.0
    freq = 160.0
    amp = 0.0
    for i in range(n_siz):
        if i % 920 == 0:
            freq = rng.uniform(110.0, 260.0)
            amp = 0.35
        freq *= 1.0012
        amp *= 0.994
        phase += 2.0 * math.pi * freq / rate
        boil.append(math.sin(phase) * amp)
    write_wav(os.path.join(ASSETS, "sounds", "boil_loop.wav"), boil, rate)

    n_drop = int(rate * 0.25)
    drop = [rng.uniform(-1.0, 1.0) * 0.45 * math.exp(-6.0 * i / n_drop) for i in range(n_drop)]
    write_wav(os.path.join(ASSETS, "sounds", "sear_drop.wav"), drop, rate)

    n_chop = int(rate * 0.12)
    chop = [
        (math.sin(2.0 * math.pi * 195.0 * i / rate) * 0.5 + rng.uniform(-0.4, 0.4))
        * math.exp(-18.0 * i / n_chop)
        for i in range(n_chop)
    ]
    write_wav(os.path.join(ASSETS, "sounds", "chop.wav"), chop, rate)

    n_bell = int(rate * 0.35)
    bell = [
        (0.45 * math.sin(2.0 * math.pi * 880.0 * i / rate) + 0.22 * math.sin(2.0 * math.pi * 1760.0 * i / rate))
        * math.exp(-7.0 * i / n_bell)
        for i in range(n_bell)
    ]
    write_wav(os.path.join(ASSETS, "sounds", "timer.wav"), bell, rate)


def generate_shaders() -> None:
    shader_dir = os.path.join(ASSETS, "shaders")
    os.makedirs(shader_dir, exist_ok=True)
    vert = """layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;

uniform vec2 u_screen;

out vec2 v_uv;
out vec4 v_color;

void main() {
    vec2 ndc = (a_pos / u_screen) * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
    v_uv = a_uv;
    v_color = a_color;
}
"""
    frag = """in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_tex;

out vec4 frag_color;

void main() {
    frag_color = texture(u_tex, v_uv) * v_color;
}
"""
    with open(os.path.join(shader_dir, "quad_ui.vert"), "w", encoding="utf-8") as f:
        f.write(vert)
    with open(os.path.join(shader_dir, "quad_ui.frag"), "w", encoding="utf-8") as f:
        f.write(frag)


def update_manifest() -> None:
    rel_files = []
    for root, _, files in os.walk(ASSETS):
        for fn in sorted(files):
            if fn == "manifest.json":
                continue
            rel = os.path.relpath(os.path.join(root, fn), ASSETS).replace("\\", "/")
            rel_files.append(rel)
    rel_files.sort()
    with open(os.path.join(ASSETS, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(rel_files, f, indent=2)
        f.write("\n")


def main() -> None:
    generate_fonts()
    generate_sprites()
    generate_sounds()
    generate_shaders()
    update_manifest()
    print("Generated FatDuck external assets, Noto font subsets, and manifest.json.")


if __name__ == "__main__":
    main()
