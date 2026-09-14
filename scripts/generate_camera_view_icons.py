#!/usr/bin/env python3
"""Generate camera-preset and camera-roll icons to match Cam_Preset_*.png."""
from __future__ import annotations

import math
import struct
import zlib
from pathlib import Path

SIZE = 30
SS = 4  # supersample
OFF_FILL = (240, 240, 240, 255)
OFF_EDGE = (148, 148, 148, 255)
ON_FILL = (241, 239, 240, 255)
ON_EDGE = (182, 145, 115, 255)
BACK = (255, 255, 255, 28)

ROOT = Path(__file__).resolve().parents[1]
TEX = ROOT / "indra" / "newview" / "skins" / "default" / "textures"
COPIES = [
    ROOT / "build-vc-64" / "newview" / "Release" / "skins" / "default" / "textures",
    ROOT / "build-vc-64" / "bin" / "Release" / "skins" / "default" / "textures",
]


def paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def write_png(path: Path, w: int, h: int, pixels: list[tuple[int, int, int, int]]) -> None:
    raw = bytearray()
    bpp = 4
    stride = w * bpp
    prev = bytearray(stride)
    for y in range(h):
        row = bytearray()
        for x in range(w):
            row.extend(pixels[y * w + x])
        filt = bytearray(stride)
        for i in range(stride):
            left = row[i - bpp] if i >= bpp else 0
            up = prev[i]
            ul = prev[i - bpp] if i >= bpp else 0
            filt[i] = (row[i] - paeth(left, up, ul)) & 255
        raw.append(4)
        raw.extend(filt)
        prev = row
    compressed = zlib.compress(bytes(raw), 9)
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)

    def chunk(tag: bytes, data: bytes) -> bytes:
        crc = zlib.crc32(tag + data) & 0xFFFFFFFF
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)

    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", compressed)
        + chunk(b"IEND", b"")
    )


def lerp(a, b, t):
    return int(a + (b - a) * t + 0.5)


def blend(dst, src):
    sr, sg, sb, sa = src
    dr, dg, db, da = dst
    if sa <= 0:
        return dst
    if sa >= 255:
        return src
    a = sa / 255.0
    out_a = sa + da * (1 - a)
    if out_a <= 0:
        return (0, 0, 0, 0)
    r = int((sr * sa + dr * da * (1 - a)) / out_a + 0.5)
    g = int((sg * sa + dg * da * (1 - a)) / out_a + 0.5)
    b = int((sb * sa + db * da * (1 - a)) / out_a + 0.5)
    return (min(255, r), min(255, g), min(255, b), min(255, int(out_a + 0.5)))


def cover(px, x, y, color, alpha=1.0):
    if alpha <= 0:
        return
    a = int(color[3] * alpha + 0.5)
    if a <= 0:
        return
    n = len(px)
    if x < 0 or y < 0 or x >= n or y >= n:
        return
    px[y][x] = blend(px[y][x], (color[0], color[1], color[2], a))


def fill_round_rect(px, n, x0, y0, x1, y1, r, color):
    for y in range(n):
        for x in range(n):
            cx = min(max(x, x0 + r), x1 - r)
            cy = min(max(y, y0 + r), y1 - r)
            dx, dy = x - cx, y - cy
            d = (dx * dx + dy * dy) ** 0.5 - r
            if d <= -0.5:
                cover(px, x, y, color)
            elif d < 0.5:
                cover(px, x, y, color, 0.5 - d)


def fill_poly(px, n, pts, color):
    miny = max(0, int(min(p[1] for p in pts)))
    maxy = min(n - 1, int(max(p[1] for p in pts)))
    edges = list(zip(pts, pts[1:] + pts[:1]))
    for y in range(miny, maxy + 1):
        ys = y + 0.5
        xs = []
        for (x0, y0), (x1, y1) in edges:
            if y0 == y1:
                continue
            if y0 > y1:
                x0, y0, x1, y1 = x1, y1, x0, y0
            if ys < y0 or ys >= y1:
                continue
            t = (ys - y0) / (y1 - y0)
            xs.append(x0 + (x1 - x0) * t)
        xs.sort()
        for i in range(0, len(xs) - 1, 2):
            a, b = xs[i], xs[i + 1]
            xa, xb = int(a + 0.5), int(b + 0.5)
            for x in range(max(0, xa), min(n, xb + 1)):
                cover(px, x, y, color)


def fill_ellipse(px, n, cx, cy, rx, ry, color):
    for y in range(n):
        for x in range(n):
            dx = (x + 0.5 - cx) / rx
            dy = (y + 0.5 - cy) / ry
            d = dx * dx + dy * dy
            if d <= 0.85:
                cover(px, x, y, color)
            elif d < 1.15:
                cover(px, x, y, color, (1.15 - d) / 0.3)


def stamp_disk(px, n, cx, cy, radius, color):
    x0 = max(0, int(cx - radius - 1))
    x1 = min(n - 1, int(cx + radius + 1))
    y0 = max(0, int(cy - radius - 1))
    y1 = min(n - 1, int(cy + radius + 1))
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            d = ((x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2) ** 0.5 - radius
            if d <= -0.5:
                cover(px, x, y, color)
            elif d < 0.5:
                cover(px, x, y, color, 0.5 - d)


def stroke_polyline(px, n, pts, width, color):
    if len(pts) < 2:
        return
    radius = width * 0.5
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        dx, dy = x1 - x0, y1 - y0
        dist = (dx * dx + dy * dy) ** 0.5
        steps = max(1, int(dist + 0.5))
        for i in range(steps + 1):
            t = i / steps
            stamp_disk(px, n, x0 + dx * t, y0 + dy * t, radius, color)


def stroke_arc(px, n, cx, cy, radius, a0, a1, width, color, steps=40):
    pts = []
    for i in range(steps + 1):
        t = a0 + (a1 - a0) * i / steps
        pts.append((cx + radius * math.cos(t), cy + radius * math.sin(t)))
    stroke_polyline(px, n, pts, width, color)
    return pts[-1], a1


def arrow_head(px, n, tip, angle, size, color):
    tx, ty = tip
    left = (tx + size * math.cos(angle + 2.5), ty + size * math.sin(angle + 2.5))
    right = (tx + size * math.cos(angle - 2.5), ty + size * math.sin(angle - 2.5))
    fill_poly(px, n, [tip, left, right], color)


def downsample(hi, n, fill, edge):
    out = []
    for y in range(SIZE):
        for x in range(SIZE):
            r = g = b = a = 0
            for oy in range(SS):
                for ox in range(SS):
                    pr, pg, pb, pa = hi[y * SS + oy][x * SS + ox]
                    r += pr * pa
                    g += pg * pa
                    b += pb * pa
                    a += pa
            if a == 0:
                out.append((0, 0, 0, 0))
                continue
            out.append((r // a, g // a, b // a, min(255, a // (SS * SS))))
    return out


def person_front(px, n, s, fill, edge, ox=0.0, oy=0.0, scale=1.0):
    cx, cy = (15 + ox) * s, (11 + oy) * s
    fill_ellipse(px, n, cx, cy, 4.2 * s * scale, 4.2 * s * scale, fill)
    fill_ellipse(px, n, cx - 1.4 * s * scale, cy - 0.4 * s * scale, 0.7 * s * scale, 0.9 * s * scale, edge)
    fill_ellipse(px, n, cx + 1.4 * s * scale, cy - 0.4 * s * scale, 0.7 * s * scale, 0.9 * s * scale, edge)
    body = [
        ((9 + ox) * s, (16 + oy) * s),
        ((21 + ox) * s, (16 + oy) * s),
        ((19.5 + ox) * s, (25 + oy) * s),
        ((10.5 + ox) * s, (25 + oy) * s),
    ]
    fill_poly(px, n, body, fill)


def person_back(px, n, s, fill, ox=0.0, oy=0.0, scale=1.0):
    cx, cy = (15 + ox) * s, (11 + oy) * s
    fill_ellipse(px, n, cx, cy, 4.2 * s * scale, 4.2 * s * scale, fill)
    body = [
        ((9 + ox) * s, (16 + oy) * s),
        ((21 + ox) * s, (16 + oy) * s),
        ((19.5 + ox) * s, (25 + oy) * s),
        ((10.5 + ox) * s, (25 + oy) * s),
    ]
    fill_poly(px, n, body, fill)


def person_side(px, n, s, fill, edge):
    fill_ellipse(px, n, 16 * s, 11 * s, 3.8 * s, 4.2 * s, fill)
    fill_ellipse(px, n, 20.2 * s, 11.6 * s, 1.6 * s, 1.2 * s, fill)
    fill_ellipse(px, n, 18.4 * s, 10.6 * s, 0.7 * s, 0.9 * s, edge)
    fill_poly(
        px,
        n,
        [(12 * s, 16 * s), (18.5 * s, 16 * s), (17.5 * s, 25 * s), (11.2 * s, 25 * s)],
        fill,
    )


def make_icon(kind: str, on: bool) -> list[tuple[int, int, int, int]]:
    n = SIZE * SS
    px = [[(0, 0, 0, 0) for _ in range(n)] for _ in range(n)]
    fill = ON_FILL if on else OFF_FILL
    edge = ON_EDGE if on else OFF_EDGE
    s = float(SS)

    if kind not in ("roll_left", "roll_right"):
        fill_round_rect(px, n, 1 * s, 1 * s, 29 * s, 29 * s, 4 * s, BACK)

    if kind == "object":
        top = [(15 * s, 7.5 * s), (24 * s, 12 * s), (15 * s, 16.5 * s), (6 * s, 12 * s)]
        left = [(6 * s, 12 * s), (15 * s, 16.5 * s), (15 * s, 24 * s), (6 * s, 19.5 * s)]
        right = [(15 * s, 16.5 * s), (24 * s, 12 * s), (24 * s, 19.5 * s), (15 * s, 24 * s)]
        fill_poly(px, n, top, fill)
        fill_poly(px, n, left, edge)
        fill_poly(px, n, right, tuple(lerp(fill[i], edge[i], 0.45) if i < 3 else fill[3] for i in range(4)))
        fill_poly(px, n, [(15 * s, 7.5 * s), (15.8 * s, 8 * s), (15.8 * s, 16.8 * s), (15 * s, 16.5 * s)], edge)
    elif kind == "mouselook":
        fill_ellipse(px, n, 15 * s, 15.2 * s, 10.2 * s, 6.4 * s, edge)
        fill_ellipse(px, n, 15 * s, 15.2 * s, 8.6 * s, 5.1 * s, fill)
        fill_ellipse(px, n, 15 * s, 15.2 * s, 3.3 * s, 3.3 * s, edge)
        fill_ellipse(px, n, 14.2 * s, 14.4 * s, 1.1 * s, 1.1 * s, fill)
    elif kind == "front":
        person_front(px, n, s, fill, edge)
    elif kind == "side":
        person_side(px, n, s, fill, edge)
    elif kind == "rear":
        person_back(px, n, s, fill)
    elif kind == "tpp":
        person_back(px, n, s, fill, ox=3.2, oy=1.2, scale=0.82)
        fill_round_rect(px, n, 6.5 * s, 9.5 * s, 13.5 * s, 16.5 * s, 1.2 * s, edge)
        fill_ellipse(px, n, 10 * s, 13 * s, 1.6 * s, 1.6 * s, fill)
    elif kind == "roll_left":
        tip, ang = stroke_arc(px, n, 15 * s, 16 * s, 8.2 * s, 2.4, -0.6, 2.4 * s, edge)
        arrow_head(px, n, tip, ang, 5.2 * s, edge)
    elif kind == "roll_right":
        tip, ang = stroke_arc(px, n, 15 * s, 16 * s, 8.2 * s, 0.74, 3.74, 2.4 * s, edge)
        arrow_head(px, n, tip, ang, 5.2 * s, edge)

    return downsample(px, n, fill, edge)


def main() -> None:
    TEX.mkdir(parents=True, exist_ok=True)
    files = {
        "Object_View_Off.png": make_icon("object", False),
        "Object_View_On.png": make_icon("object", True),
        "MouseLook_View_Off.png": make_icon("mouselook", False),
        "MouseLook_View_On.png": make_icon("mouselook", True),
        "Cam_Preset_Front_Off.png": make_icon("front", False),
        "Cam_Preset_Front_On.png": make_icon("front", True),
        "Cam_Preset_Side_Off.png": make_icon("side", False),
        "Cam_Preset_Side_On.png": make_icon("side", True),
        "Cam_Preset_TPP_Off.png": make_icon("tpp", False),
        "Cam_Preset_TPP_On.png": make_icon("tpp", True),
        "Cam_Preset_Back_Off.png": make_icon("rear", False),
        "Cam_Preset_Back_On.png": make_icon("rear", True),
        "Cam_Roll_Left_Off.png": make_icon("roll_left", False),
        "Cam_Roll_Left_On.png": make_icon("roll_left", True),
        "Cam_Roll_Right_Off.png": make_icon("roll_right", False),
        "Cam_Roll_Right_On.png": make_icon("roll_right", True),
    }
    for name, pixels in files.items():
        dest = TEX / name
        write_png(dest, SIZE, SIZE, pixels)
        print("wrote", dest, dest.stat().st_size)
        for copy_dir in COPIES:
            if copy_dir.exists():
                copy_dir.mkdir(parents=True, exist_ok=True)
                target = copy_dir / name
                target.write_bytes(dest.read_bytes())
                print("copied", target)


if __name__ == "__main__":
    main()
