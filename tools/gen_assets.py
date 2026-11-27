#!/usr/bin/env python3
"""Deterministically generates external CC BY 4.0 PNGs, WAVs, shaders, and manifest.json."""
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
        raw.append(0)  # Filter type 0 (None)
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


def put_px(buf: bytearray, w: int, h: int, x: int, y: int, r: int, g: int, b: int, a: int = 255) -> None:
    if 0 <= x < w and 0 <= y < h:
        idx = (y * w + x) * 4
        buf[idx] = r
        buf[idx + 1] = g
        buf[idx + 2] = b
        buf[idx + 3] = a


def fill_rect(buf: bytearray, w: int, h: int, x0: int, y0: int, rw: int, rh: int, col: tuple) -> None:
    for y in range(y0, y0 + rh):
        for x in range(x0, x0 + rw):
            put_px(buf, w, h, x, y, *col)


def fill_circle(buf: bytearray, w: int, h: int, cx: float, cy: float, rad: float, col: tuple) -> None:
    r2 = rad * rad
    for y in range(int(cy - rad - 1), int(cy + rad + 2)):
        for x in range(int(cx - rad - 1), int(cx + rad + 2)):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r2:
                put_px(buf, w, h, x, y, *col)


# Clean 8x8 bitmap glyphs for ASCII 32..126 + degree symbol (127 mapped as °)
GLYPHS_8X8 = {
    32: [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    33: [0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00],
    34: [0x66, 0x66, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00],
    35: [0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00],
    36: [0x18, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x18, 0x00],
    37: [0x00, 0xC6, 0xCC, 0x18, 0x30, 0x66, 0xC6, 0x00],
    38: [0x38, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0x76, 0x00],
    39: [0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00],
    40: [0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00],
    41: [0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00],
    42: [0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00],
    43: [0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00],
    44: [0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30],
    45: [0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00],
    46: [0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00],
    47: [0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00],
    48: [0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00],
    49: [0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00],
    50: [0x3C, 0x66, 0x06, 0x1C, 0x30, 0x66, 0x7E, 0x00],
    51: [0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00],
    52: [0x0E, 0x1E, 0x36, 0x66, 0x7F, 0x06, 0x0F, 0x00],
    53: [0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00],
    54: [0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00],
    55: [0x7E, 0x66, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x00],
    56: [0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00],
    57: [0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00],
    58: [0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00],
    59: [0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30],
    60: [0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x00],
    61: [0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00],
    62: [0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x00],
    63: [0x3C, 0x66, 0x06, 0x0C, 0x18, 0x00, 0x18, 0x00],
    64: [0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x60, 0x3E, 0x00],
    65: [0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00],
    66: [0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00],
    67: [0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00],
    68: [0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00],
    69: [0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00],
    70: [0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00],
    71: [0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3E, 0x00],
    72: [0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00],
    73: [0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00],
    74: [0x1E, 0x0C, 0x0C, 0x0C, 0x6C, 0x6C, 0x38, 0x00],
    75: [0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00],
    76: [0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00],
    77: [0xC6, 0xEE, 0xFE, 0xD6, 0xC6, 0xC6, 0xC6, 0x00],
    78: [0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00],
    79: [0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00],
    80: [0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00],
    81: [0x3C, 0x66, 0x66, 0x66, 0x6A, 0x6C, 0x36, 0x00],
    82: [0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x00],
    83: [0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00],
    84: [0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00],
    85: [0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00],
    86: [0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00],
    87: [0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00],
    88: [0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00],
    89: [0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00],
    90: [0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00],
    91: [0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00],
    92: [0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02, 0x00],
    93: [0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00],
    94: [0x18, 0x3C, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00],
    95: [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00],
    96: [0x30, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00],
    97: [0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00],
    98: [0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00],
    99: [0x00, 0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C, 0x00],
    100: [0x06, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x00],
    101: [0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00],
    102: [0x1C, 0x36, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x00],
    103: [0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x3C],
    104: [0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00],
    105: [0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00],
    106: [0x06, 0x00, 0x0E, 0x06, 0x06, 0x66, 0x66, 0x3C],
    107: [0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x00],
    108: [0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00],
    109: [0x00, 0x00, 0xEC, 0xFE, 0xD6, 0xC6, 0xC6, 0x00],
    110: [0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00],
    111: [0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00],
    112: [0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60],
    113: [0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x06],
    114: [0x00, 0x00, 0x7C, 0x66, 0x60, 0x60, 0x60, 0x00],
    115: [0x00, 0x00, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00],
    116: [0x30, 0x30, 0x7C, 0x30, 0x30, 0x36, 0x1C, 0x00],
    117: [0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00],
    118: [0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00],
    119: [0x00, 0x00, 0xC6, 0xC6, 0xD6, 0xFE, 0x6C, 0x00],
    120: [0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00],
    121: [0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x3C],
    122: [0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00],
    123: [0x0E, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0E, 0x00],
    124: [0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00],
    125: [0x70, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x70, 0x00],
    126: [0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
    127: [0x38, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00],  # degree symbol
}


def generate_font() -> None:
    # 16x6 grid of 16x16 cells (2x crisp upscale with soft anti-aliased edge) => 256x128 RGBA PNG
    w, h = 256, 128
    buf = bytearray(w * h * 4)
    # Pixel (0,0)-(1,1) solid white texel for untextured UI quads!
    fill_rect(buf, w, h, 0, 0, 2, 2, (255, 255, 255, 255))

    lua_lines = [
        "-- CC BY 4.0 Font metrics generated by tools/gen_assets.py",
        "return {",
        "  atlas = 'fonts/fatduck_font.png',",
        "  tex_w = 256, tex_h = 128, cell_w = 16, cell_h = 16, advance = 9,",
        "  glyphs = {",
    ]
    for code in range(32, 128):
        idx = code - 32
        col = idx % 16
        row = idx // 16
        ox = col * 16
        oy = row * 16 + 16  # Row 0 reserved for solid white & UI markers
        if oy + 16 > h:
            break
        rows = GLYPHS_8X8.get(code, GLYPHS_8X8[63])
        for gy in range(8):
            bits = rows[gy]
            for gx in range(8):
                if (bits >> (7 - gx)) & 1:
                    fill_rect(buf, w, h, ox + gx * 2, oy + gy * 2, 2, 2, (255, 255, 255, 255))
        lua_lines.append(f"    [{code}] = {{ x = {ox}, y = {oy}, w = 16, h = 16 }},")
    lua_lines.append("  }")
    lua_lines.append("}")
    write_png(os.path.join(ASSETS, "fonts", "fatduck_font.png"), w, h, buf)
    with open(os.path.join(ASSETS, "fonts", "fatduck_font.lua"), "w", encoding="utf-8") as f:
        f.write("\n".join(lua_lines) + "\n")


def generate_sprites() -> None:
    # kitchen_atlas.png: 256x128 with 4 vessel tiles of 64x64 + flame/steam/bubble tiles
    w, h = 256, 128
    buf = bytearray(w * h * 4)
    # Tile 0 (0,0): Cast-iron skillet
    fill_rect(buf, w, h, 8, 38, 40, 12, (58, 62, 68, 255))
    fill_rect(buf, w, h, 46, 40, 14, 5, (80, 84, 92, 255))
    fill_rect(buf, w, h, 11, 36, 34, 3, (120, 126, 136, 255))
    # Tile 1 (64,0): Saucepan / Stockpot
    fill_rect(buf, w, h, 76, 22, 38, 30, (170, 180, 194, 255))
    fill_rect(buf, w, h, 72, 26, 5, 6, (110, 118, 128, 255))
    fill_rect(buf, w, h, 113, 26, 5, 6, (110, 118, 128, 255))
    fill_rect(buf, w, h, 79, 25, 32, 24, (74, 134, 212, 220))
    # Tile 2 (128,0): Convection Oven / Baker
    fill_rect(buf, w, h, 136, 10, 48, 46, (68, 72, 80, 255))
    fill_rect(buf, w, h, 142, 18, 36, 28, (235, 145, 56, 235))
    # Tile 3 (192,0): Sous-Vide / Immersion Bath
    fill_rect(buf, w, h, 202, 18, 44, 36, (88, 165, 225, 210))
    fill_rect(buf, w, h, 206, 8, 8, 34, (45, 48, 55, 255))
    # Row 1: Flame (0,64), Steam (64,64), Emulsion Bowl (128,64), Duck Badge (192,64)
    fill_circle(buf, w, h, 32, 104, 14, (245, 110, 35, 240))
    fill_circle(buf, w, h, 32, 107, 8, (255, 220, 80, 255))
    fill_circle(buf, w, h, 88, 96, 8, (225, 235, 245, 150))
    fill_circle(buf, w, h, 102, 88, 10, (225, 235, 245, 130))
    fill_circle(buf, w, h, 160, 100, 18, (238, 208, 112, 245))
    fill_circle(buf, w, h, 220, 96, 14, (245, 198, 66, 255))
    fill_circle(buf, w, h, 226, 88, 8, (245, 198, 66, 255))
    fill_rect(buf, w, h, 232, 87, 7, 4, (235, 120, 40, 255))
    write_png(os.path.join(ASSETS, "graphics", "kitchen_atlas.png"), w, h, buf)

    # icons.png: 128x64 status icons (safe check, warning, touch, reload)
    iw, ih = 128, 64
    ibuf = bytearray(iw * ih * 4)
    fill_circle(ibuf, iw, ih, 16, 16, 12, (46, 194, 126, 255))
    fill_circle(ibuf, iw, ih, 48, 16, 12, (235, 90, 70, 255))
    fill_circle(ibuf, iw, ih, 80, 16, 12, (78, 154, 246, 255))
    fill_circle(ibuf, iw, ih, 112, 16, 12, (240, 184, 64, 255))
    write_png(os.path.join(ASSETS, "graphics", "icons.png"), iw, ih, ibuf)


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

    # 1. sizzle_loop.wav (0.6s filtered micro-steam crackle)
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

    # 2. boil_loop.wav (0.6s low-frequency cavitation bubble pops)
    boil = []
    phase = 0.0
    freq = 160.0
    amp = 0.0
    for i in range(n_siz):
        if i % 920 == 0:
            freq = rng.uniform(110.0, 260.0)
            amp = 0.35
        freq *= 1.0012  # Rising Minnaert bubble pitch
        amp *= 0.994
        phase += 2.0 * math.pi * freq / rate
        boil.append(math.sin(phase) * amp)
    write_wav(os.path.join(ASSETS, "sounds", "boil_loop.wav"), boil, rate)

    # 3. sear_drop.wav (0.25s initial contact steam burst)
    n_drop = int(rate * 0.25)
    drop = [rng.uniform(-1.0, 1.0) * 0.45 * math.exp(-6.0 * i / n_drop) for i in range(n_drop)]
    write_wav(os.path.join(ASSETS, "sounds", "sear_drop.wav"), drop, rate)

    # 4. chop.wav (0.12s wooden prep board transient)
    n_chop = int(rate * 0.12)
    chop = [
        (math.sin(2.0 * math.pi * 195.0 * i / rate) * 0.5 + rng.uniform(-0.4, 0.4))
        * math.exp(-18.0 * i / n_chop)
        for i in range(n_chop)
    ]
    write_wav(os.path.join(ASSETS, "sounds", "chop.wav"), chop, rate)

    # 5. timer.wav (0.35s clean kitchen chime)
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
    # Version preamble (#version 330 core vs #version 300 es) is prepended by gl_renderer.c
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
    generate_font()
    generate_sprites()
    generate_sounds()
    generate_shaders()
    update_manifest()
    print("Generated FatDuck external assets and manifest.json.")


if __name__ == "__main__":
    main()
