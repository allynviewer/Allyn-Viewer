#!/usr/bin/env python3
"""Generate Region/Estate overlay icons without Pillow."""
from __future__ import annotations

import math
import struct
import zlib
from pathlib import Path

OUT = Path(__file__).resolve().parent
SCALE = 4
ICON = 32
SRC = ICON * SCALE

INK = (47, 74, 110, 255)
ACCENT = (70, 130, 180, 255)
WHITE = (255, 255, 255, 255)
GREEN = (46, 125, 70, 255)
RED = (160, 60, 55, 255)


def blank(w=SRC, h=SRC):
    return [[(0, 0, 0, 0) for _ in range(w)] for _ in range(h)], w, h


def blend(dst, x, y, color):
    h = len(dst)
    w = len(dst[0])
    if x < 0 or y < 0 or x >= w or y >= h:
        return
    sr, sg, sb, sa = color
    if sa <= 0:
        return
    dr, dg, db, da = dst[y][x]
    a = sa / 255.0
    ia = 1.0 - a
    out_a = sa + da * ia
    if out_a <= 0:
        dst[y][x] = (0, 0, 0, 0)
        return
    dst[y][x] = (
        int((sr * sa + dr * da * ia) / out_a),
        int((sg * sa + dg * da * ia) / out_a),
        int((sb * sa + db * da * ia) / out_a),
        int(out_a),
    )


def fill_circle(dst, cx, cy, r, color):
    r2 = r * r
    for y in range(int(cy - r - 1), int(cy + r + 2)):
        for x in range(int(cx - r - 1), int(cx + r + 2)):
            d2 = (x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2
            if d2 <= r2:
                cover = 1.0
                if d2 > (r - 1) ** 2:
                    cover = max(0.0, r - math.sqrt(d2))
                if cover > 0:
                    c = (color[0], color[1], color[2], int(color[3] * min(1.0, cover)))
                    blend(dst, x, y, c)


def stroke_circle(dst, cx, cy, r, color, width):
    outer = r + width * 0.5
    inner = r - width * 0.5
    o2 = outer * outer
    i2 = inner * inner
    for y in range(int(cy - outer - 1), int(cy + outer + 2)):
        for x in range(int(cx - outer - 1), int(cx + outer + 2)):
            d2 = (x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2
            if i2 <= d2 <= o2:
                blend(dst, x, y, color)


def fill_rect(dst, x0, y0, x1, y1, color, radius=0):
    if x0 > x1:
        x0, x1 = x1, x0
    if y0 > y1:
        y0, y1 = y1, y0
    for y in range(int(y0), int(y1) + 1):
        for x in range(int(x0), int(x1) + 1):
            if radius:
                dx = min(x - x0, x1 - x)
                dy = min(y - y0, y1 - y)
                if dx < radius and dy < radius and (dx - radius) ** 2 + (dy - radius) ** 2 > radius * radius:
                    continue
            blend(dst, x, y, color)


def stroke_rect(dst, x0, y0, x1, y1, color, width=8, radius=8):
    fill_rect(dst, x0, y0, x1, y0 + width, color, 0)
    fill_rect(dst, x0, y1 - width, x1, y1, color, 0)
    fill_rect(dst, x0, y0, x0 + width, y1, color, 0)
    fill_rect(dst, x1 - width, y0, x1, y1, color, 0)
    if radius:
        fill_circle(dst, x0 + radius, y0 + radius, radius, color)
        fill_circle(dst, x1 - radius, y0 + radius, radius, color)
        fill_circle(dst, x0 + radius, y1 - radius, radius, color)
        fill_circle(dst, x1 - radius, y1 - radius, radius, color)


def draw_line(dst, x0, y0, x1, y1, color, width=10):
    steps = int(max(abs(x1 - x0), abs(y1 - y0), 1) * 2)
    r = width / 2.0
    for i in range(steps + 1):
        t = i / steps
        x = x0 + (x1 - x0) * t
        y = y0 + (y1 - y0) * t
        fill_circle(dst, x, y, r, color)


def fill_poly(dst, pts, color):
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    miny, maxy = int(min(ys)), int(max(ys))
    n = len(pts)
    for y in range(miny, maxy + 1):
        inter = []
        for i in range(n):
            x0, y0 = pts[i]
            x1, y1 = pts[(i + 1) % n]
            if (y0 <= y < y1) or (y1 <= y < y0):
                if y1 != y0:
                    inter.append(x0 + (y - y0) * (x1 - x0) / (y1 - y0))
        inter.sort()
        for i in range(0, len(inter) - 1, 2):
            x0 = int(inter[i])
            x1 = int(inter[i + 1])
            for x in range(x0, x1 + 1):
                blend(dst, x, y, color)


def downsample(src, tw, th):
    sh = len(src)
    sw = len(src[0])
    out = [[(0, 0, 0, 0) for _ in range(tw)] for _ in range(th)]
    for y in range(th):
        for x in range(tw):
            x0 = int(x * sw / tw)
            x1 = int((x + 1) * sw / tw)
            y0 = int(y * sh / th)
            y1 = int((y + 1) * sh / th)
            r = g = b = a = n = 0
            for yy in range(y0, max(y1, y0 + 1)):
                for xx in range(x0, max(x1, x0 + 1)):
                    pr, pg, pb, pa = src[yy][xx]
                    r += pr * pa
                    g += pg * pa
                    b += pb * pa
                    a += pa
                    n += 1
            if a == 0 or n == 0:
                continue
            out[y][x] = (int(r / a), int(g / a), int(b / a), int(a / n))
    return out


def write_png(pixels, path: Path):
    h = len(pixels)
    w = len(pixels[0])
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        for x in range(w):
            r, g, b, a = pixels[y][x]
            raw.extend((max(0, min(255, r)), max(0, min(255, g)), max(0, min(255, b)), max(0, min(255, a))))

    def chunk(tag, data):
        crc = zlib.crc32(tag + data) & 0xFFFFFFFF
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)
    data = b"\x89PNG\r\n\x1a\n"
    data += chunk(b"IHDR", ihdr)
    data += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    data += chunk(b"IEND", b"")
    path.write_bytes(data)
    print("wrote", path.name)


def save(dst, name, size=(ICON, ICON)):
    pixels = downsample(dst, size[0], size[1]) if size != (len(dst[0]), len(dst)) else dst
    write_png(pixels, OUT / name)


def icon_help():
    dst, _, _ = blank()
    fill_circle(dst, 64, 64, 54, ACCENT)
    # question mark as bars
    stroke_circle(dst, 64, 48, 16, WHITE, 10)
    fill_rect(dst, 59, 48, 69, 78, WHITE)
    fill_circle(dst, 64, 92, 6, WHITE)
    fill_rect(dst, 48, 32, 80, 48, ACCENT)  # cut top of q-loop left
    fill_rect(dst, 36, 38, 64, 52, ACCENT)
    save(dst, "icn_region_help.png")


def icon_apply():
    dst, _, _ = blank()
    fill_circle(dst, 64, 64, 54, GREEN)
    draw_line(dst, 40, 66, 56, 86, WHITE, 12)
    draw_line(dst, 56, 86, 92, 44, WHITE, 12)
    save(dst, "icn_region_apply.png")


def icon_add():
    dst, _, _ = blank()
    fill_circle(dst, 64, 64, 54, ACCENT)
    draw_line(dst, 64, 36, 64, 92, WHITE, 12)
    draw_line(dst, 36, 64, 92, 64, WHITE, 12)
    save(dst, "icn_region_add.png")


def icon_remove():
    dst, _, _ = blank()
    fill_circle(dst, 64, 64, 54, RED)
    draw_line(dst, 40, 64, 88, 64, WHITE, 12)
    save(dst, "icn_region_remove.png")


def icon_copy():
    dst, _, _ = blank()
    stroke_rect(dst, 38, 22, 102, 90, INK, 8, 8)
    fill_rect(dst, 22, 42, 86, 110, (240, 246, 252, 255), 8)
    stroke_rect(dst, 22, 42, 86, 110, INK, 8, 8)
    save(dst, "icn_region_copy.png")


def icon_message():
    dst, _, _ = blank()
    fill_rect(dst, 22, 28, 106, 86, ACCENT, 16)
    fill_poly(dst, [(40, 84), (40, 110), (68, 84)], ACCENT)
    fill_circle(dst, 46, 54, 6, WHITE)
    fill_circle(dst, 64, 54, 6, WHITE)
    fill_circle(dst, 82, 54, 6, WHITE)
    save(dst, "icn_region_message.png")


def icon_kick():
    dst, _, _ = blank()
    fill_circle(dst, 48, 40, 16, INK)
    fill_rect(dst, 32, 58, 64, 100, INK, 16)
    fill_poly(dst, [(72, 64), (104, 64), (104, 52), (124, 70), (104, 88), (104, 76), (72, 76)], ACCENT)
    save(dst, "icn_region_kick.png")


def icon_telehub():
    dst, _, _ = blank()
    stroke_circle(dst, 64, 64, 48, INK, 8)
    stroke_circle(dst, 64, 64, 30, INK, 8)
    fill_circle(dst, 64, 64, 12, ACCENT)
    draw_line(dst, 64, 18, 64, 110, INK, 6)
    draw_line(dst, 18, 64, 110, 64, INK, 6)
    save(dst, "icn_region_telehub.png")


def icon_reset():
    dst, _, _ = blank()
    stroke_circle(dst, 64, 64, 40, INK, 12)
    fill_rect(dst, 64, 18, 110, 64, (0, 0, 0, 0))
    # clear a sector then draw arrow
    for y in range(18, 64):
        for x in range(64, 118):
            dst[y][x] = (0, 0, 0, 0)
    stroke_circle(dst, 64, 64, 40, INK, 12)
    for y in range(20, 58):
        for x in range(78, 118):
            d2 = (x - 64) ** 2 + (y - 64) ** 2
            if 34 ** 2 < d2 < 46 ** 2:
                dst[y][x] = (0, 0, 0, 0)
    fill_poly(dst, [(92, 16), (118, 42), (84, 48)], INK)
    save(dst, "icn_region_reset.png")


def icon_download():
    dst, _, _ = blank()
    draw_line(dst, 64, 20, 64, 78, INK, 12)
    fill_poly(dst, [(40, 68), (64, 100), (88, 68)], INK)
    draw_line(dst, 28, 108, 100, 108, INK, 10)
    save(dst, "icn_region_download.png")


def icon_upload():
    dst, _, _ = blank()
    draw_line(dst, 64, 48, 64, 106, INK, 12)
    fill_poly(dst, [(40, 52), (64, 20), (88, 52)], INK)
    draw_line(dst, 28, 108, 100, 108, INK, 10)
    save(dst, "icn_region_upload.png")


def icon_bake():
    dst, _, _ = blank()
    fill_poly(dst, [(20, 88), (44, 40), (72, 70), (92, 28), (112, 88)], ACCENT)
    draw_line(dst, 20, 88, 44, 40, INK, 8)
    draw_line(dst, 44, 40, 72, 70, INK, 8)
    draw_line(dst, 72, 70, 92, 28, INK, 8)
    draw_line(dst, 92, 28, 112, 88, INK, 8)
    draw_line(dst, 20, 96, 112, 96, INK, 10)
    save(dst, "icn_region_bake.png")


def icon_return():
    dst, _, _ = blank()
    fill_poly(dst, [(24, 64), (56, 36), (56, 52), (104, 52), (104, 76), (56, 76), (56, 92)], INK)
    save(dst, "icn_region_return.png")


def icon_colliders():
    dst, _, _ = blank()
    stroke_rect(dst, 22, 38, 78, 94, INK, 8, 8)
    fill_rect(dst, 50, 22, 106, 78, (240, 246, 252, 180), 8)
    stroke_rect(dst, 50, 22, 106, 78, ACCENT, 8, 8)
    save(dst, "icn_region_colliders.png")


def icon_scripts():
    dst, _, _ = blank()
    fill_poly(dst, [(40, 28), (22, 64), (40, 100), (52, 92), (40, 64), (52, 36)], INK)
    fill_poly(dst, [(88, 28), (106, 64), (88, 100), (76, 92), (88, 64), (76, 36)], INK)
    draw_line(dst, 70, 28, 54, 100, ACCENT, 10)
    save(dst, "icn_region_scripts.png")


def icon_restart():
    dst, _, _ = blank()
    stroke_circle(dst, 64, 64, 48, INK, 12)
    fill_rect(dst, 58, 28, 70, 70, INK)
    save(dst, "icn_region_restart.png")


def icon_delay():
    dst, _, _ = blank()
    stroke_circle(dst, 64, 64, 50, INK, 10)
    draw_line(dst, 64, 64, 64, 36, INK, 10)
    draw_line(dst, 64, 64, 88, 76, ACCENT, 10)
    fill_circle(dst, 64, 64, 8, ACCENT)
    save(dst, "icn_region_delay.png")


def icon_choose():
    dst, _, _ = blank()
    fill_circle(dst, 64, 42, 18, INK)
    fill_rect(dst, 40, 64, 88, 108, INK, 18)
    save(dst, "icn_region_choose.png")


def icon_cancel():
    dst, _, _ = blank()
    fill_circle(dst, 64, 64, 54, RED)
    draw_line(dst, 42, 42, 86, 86, WHITE, 12)
    draw_line(dst, 86, 42, 42, 86, WHITE, 12)
    save(dst, "icn_region_cancel.png")


def icon_daycycle():
    w, h = 240 * SCALE, 24 * SCALE
    dst = [[(0, 0, 0, 0) for _ in range(w)] for _ in range(h)]
    for x in range(w):
        t = x / max(w - 1, 1)
        r = int(255 * max(0, 1 - abs(t - 0.22) * 1.7))
        g = int(170 * max(0, 1 - abs(t - 0.32) * 1.5))
        b = int(70 + 150 * t)
        r, g, b = [max(0, min(255, v)) for v in (r, g, b)]
        for y in range(8, h - 8):
            dst[y][x] = (r, g, b, 255)
    fill_circle(dst, int(w * 0.18), h // 2, 28, (255, 210, 70, 255))
    fill_circle(dst, int(w * 0.82), h // 2, 24, (230, 235, 245, 255))
    fill_circle(dst, int(w * 0.82) - 8, h // 2 - 4, 16, (90, 110, 150, 255))
    save(dst, "icn_region_daycycle.png", (240, 24))


def icon_profile():
    dst, _, _ = blank()
    stroke_rect(dst, 28, 20, 100, 108, INK, 8, 12)
    fill_circle(dst, 64, 48, 14, ACCENT)
    fill_rect(dst, 46, 70, 82, 100, ACCENT, 12)
    save(dst, "icn_region_profile.png")


def main():
    icon_help()
    icon_apply()
    icon_add()
    icon_remove()
    icon_copy()
    icon_message()
    icon_kick()
    icon_telehub()
    icon_reset()
    icon_download()
    icon_upload()
    icon_bake()
    icon_return()
    icon_colliders()
    icon_scripts()
    icon_restart()
    icon_delay()
    icon_choose()
    icon_cancel()
    icon_daycycle()
    icon_profile()


if __name__ == "__main__":
    main()
