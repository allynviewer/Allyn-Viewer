#!/usr/bin/env python3
"""Generate Allyn Cyber UI chrome textures (navy / purple / blue, no neon)."""

import math
import os
import struct
import sys

# Design tokens (match skins/cyber/colors.xml)
BG_DEEP = (10, 11, 30)
BG_PANEL = (18, 20, 43)
BG_SURFACE = (24, 26, 52)
BG_FIELD = (16, 18, 40)
PURPLE = (139, 92, 246)
BLUE = (99, 102, 241)
LAVENDER = (167, 139, 250)
WHITE = (255, 255, 255)
TEXT = (236, 238, 248)
GRAY = (110, 116, 150)
BORDER = (55, 60, 100)
DISABLED = (50, 54, 78)


def clamp(v, lo=0, hi=255):
    return max(lo, min(hi, int(v)))


def lerp(a, b, t):
    return a + (b - a) * t


def lerp_color(c1, c2, t):
    return tuple(clamp(lerp(c1[i], c2[i], t)) for i in range(3))


def save_tga(path, width, height, pixels):
    header = struct.pack(
        "<BBBHHBHHHHBB",
        0, 0, 2, 0, 0, 0, 0, 0, width, height, 32, 0x28,
    )
    data = bytearray()
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[y * width + x]
            data.extend((b, g, r, a))
    with open(path, "wb") as f:
        f.write(header)
        f.write(data)


def blank(w, h, color=(0, 0, 0, 0)):
    r, g, b = color[:3]
    a = color[3] if len(color) > 3 else 255
    return [(r, g, b, a)] * (w * h)


def set_px(pixels, w, x, y, color):
    if 0 <= x < w and 0 <= y < len(pixels) // w:
        pixels[y * w + x] = color if len(color) == 4 else (*color, 255)


def blend_px(pixels, w, x, y, color):
    if not (0 <= x < w and 0 <= y < len(pixels) // w):
        return
    src = color if len(color) == 4 else (*color, 255)
    dr, dg, db, da = pixels[y * w + x]
    sr, sg, sb, sa = src
    t = sa / 255.0
    pixels[y * w + x] = (
        clamp(dr * (1 - t) + sr * t),
        clamp(dg * (1 - t) + sg * t),
        clamp(db * (1 - t) + sb * t),
        clamp(max(da, sa)),
    )


def draw_hline(pixels, w, y, x0, x1, color, thickness=1):
    for yy in range(y, y + thickness):
        for x in range(x0, x1 + 1):
            set_px(pixels, w, x, yy, color)


def draw_vline(pixels, w, h, x, y0, y1, color, thickness=1):
    for xx in range(x, x + thickness):
        for y in range(y0, y1 + 1):
            set_px(pixels, w, xx, y, color)


def fill_rect(pixels, w, x0, y0, x1, y1, color):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            set_px(pixels, w, x, y, color)


def rounded_cover(px, py, x0, y0, x1, y1, radius):
    """0 if outside rounded rect, 1 if inside, 0-1 on the rim."""
    cx = min(max(px, x0 + radius), x1 - radius)
    cy = min(max(py, y0 + radius), y1 - radius)
    dx = px - cx
    dy = py - cy
    dist = math.sqrt(dx * dx + dy * dy)
    if dist <= radius - 0.5:
        return 1.0
    if dist >= radius + 0.5:
        return 0.0
    return 1.0 - (dist - (radius - 0.5))


def fill_rounded(pixels, w, h, x0, y0, x1, y1, color, radius=4):
    r, g, b = color[:3]
    a = color[3] if len(color) > 3 else 255
    for y in range(max(0, y0), min(h, y1 + 1)):
        for x in range(max(0, x0), min(w, x1 + 1)):
            cov = rounded_cover(x + 0.5, y + 0.5, x0, y0, x1, y1, radius)
            if cov > 0:
                set_px(pixels, w, x, y, (r, g, b, clamp(a * cov)))


def stroke_rounded(pixels, w, h, x0, y0, x1, y1, color, radius=4, bw=1):
    r, g, b = color[:3]
    a = color[3] if len(color) > 3 else 255
    for y in range(max(0, y0 - 1), min(h, y1 + 2)):
        for x in range(max(0, x0 - 1), min(w, x1 + 2)):
            outer = rounded_cover(x + 0.5, y + 0.5, x0, y0, x1, y1, radius)
            inner = rounded_cover(
                x + 0.5, y + 0.5, x0 + bw, y0 + bw, x1 - bw, y1 - bw, max(0, radius - bw)
            )
            cov = max(0.0, outer - inner)
            if cov > 0.05:
                blend_px(pixels, w, x, y, (r, g, b, clamp(a * cov)))


def fill_gradient_v(pixels, w, h, top_color, bottom_color, x0=0, y0=0, x1=None, y1=None):
    if x1 is None:
        x1 = w - 1
    if y1 is None:
        y1 = h - 1
    span = max(y1 - y0, 1)
    for y in range(y0, y1 + 1):
        t = (y - y0) / span
        c = lerp_color(top_color, bottom_color, t)
        for x in range(x0, x1 + 1):
            set_px(pixels, w, x, y, (*c, 255))


def fill_gradient_h(pixels, w, h, left_color, right_color, x0=0, y0=0, x1=None, y1=None):
    if x1 is None:
        x1 = w - 1
    if y1 is None:
        y1 = h - 1
    span = max(x1 - x0, 1)
    for x in range(x0, x1 + 1):
        t = (x - x0) / span
        c = lerp_color(left_color, right_color, t)
        for y in range(y0, y1 + 1):
            set_px(pixels, w, x, y, (*c, 255))


def fill_rounded_gradient_h(pixels, w, h, x0, y0, x1, y1, c_left, c_right, radius=5):
    span = max(x1 - x0, 1)
    for y in range(max(0, y0), min(h, y1 + 1)):
        for x in range(max(0, x0), min(w, x1 + 1)):
            cov = rounded_cover(x + 0.5, y + 0.5, x0, y0, x1, y1, radius)
            if cov > 0:
                t = (x - x0) / span
                c = lerp_color(c_left, c_right, t)
                set_px(pixels, w, x, y, (*c, clamp(255 * cov)))


def make_button(w=128, h=32, selected=False, disabled=False):
    pixels = blank(w, h)
    radius = 5
    if disabled:
        fill_rounded(pixels, w, h, 1, 1, w - 2, h - 2, (*DISABLED, 255), radius)
        stroke_rounded(pixels, w, h, 1, 1, w - 2, h - 2, (*GRAY, 140), radius)
    elif selected:
        fill_rounded_gradient_h(pixels, w, h, 1, 1, w - 2, h - 2, PURPLE, BLUE, radius)
        stroke_rounded(pixels, w, h, 1, 1, w - 2, h - 2, (*LAVENDER, 80), radius)
    else:
        fill_rounded(pixels, w, h, 1, 1, w - 2, h - 2, (*BG_SURFACE, 255), radius)
        stroke_rounded(pixels, w, h, 1, 1, w - 2, h - 2, (*BORDER, 220), radius)
    return pixels


def make_square_button(w=128, h=32, selected=False):
    return make_button(w, h, selected=selected, disabled=False)


def make_tab(w=64, h=16, selected=False, horizontal=True):
    pixels = blank(w, h)
    if selected:
        fill_rounded_gradient_h(pixels, w, h, 0, 0, w - 1, h - 1, PURPLE, BLUE, 3)
        stroke_rounded(pixels, w, h, 0, 0, w - 1, h - 1, (*LAVENDER, 60), 3)
    else:
        fill_rounded(pixels, w, h, 0, 0, w - 1, h - 1, (*BG_PANEL, 255), 3)
        stroke_rounded(pixels, w, h, 0, 0, w - 1, h - 1, (*BORDER, 140), 3)
    return pixels


def _draw_chevron(pixels, w, h, x, color):
    cy = h // 2
    pts = [
        (0, 0), (1, -1), (1, 1), (2, -2), (2, 2), (3, -3), (3, 3),
        (1, 0), (2, -1), (2, 1),
    ]
    for dx, dy in pts:
        set_px(pixels, w, x + dx, cy + dy, color)


def make_sidebar_tab(w=172, h=28, selected=False):
    """Vertical prefs nav: idle is transparent; selected is a gradient pill."""
    pixels = blank(w, h)
    if not selected:
        return pixels
    x0, y0, x1, y1 = 1, 1, w - 2, h - 2
    # soft outer glow
    for y in range(h):
        for x in range(w):
            cov = rounded_cover(x + 0.5, y + 0.5, x0 - 1, y0 - 1, x1 + 1, y1 + 1, 12)
            if 0 < cov < 1:
                blend_px(pixels, w, x, y, (*PURPLE, clamp(70 * cov)))
    fill_rounded_gradient_h(pixels, w, h, x0, y0, x1, y1, PURPLE, BLUE, 11)
    stroke_rounded(pixels, w, h, x0, y0, x1, y1, (*LAVENDER, 70), 11)
    _draw_chevron(pixels, w, h, w - 14, (*WHITE, 230))
    return pixels


def make_section_rule(w=256, h=2):
    pixels = blank(w, h)
    for x in range(w):
        t = x / max(w - 1, 1)
        c = lerp_color(PURPLE, (55, 60, 100), min(1.0, t * 1.15))
        a = 200 if x < w * 0.75 else clamp(200 * (1.0 - (x / w - 0.75) / 0.25))
        set_px(pixels, w, x, 0, (*c, a))
        set_px(pixels, w, x, 1, (*c, clamp(a * 0.55)))
    return pixels


def make_nav_icon(kind):
    """16x16 line-art icons for the preferences sidebar."""
    w = h = 16
    pixels = blank(w, h)
    c = (*WHITE, 235)
    d = (*WHITE, 160)

    def px(x, y, col=c):
        set_px(pixels, w, x, y, col)

    def hline(y, x0, x1, col=c):
        for x in range(x0, x1 + 1):
            px(x, y, col)

    def vline(x, y0, y1, col=c):
        for y in range(y0, y1 + 1):
            px(x, y, col)

    def rect(x0, y0, x1, y1, col=c):
        hline(y0, x0, x1, col)
        hline(y1, x0, x1, col)
        vline(x0, y0, y1, col)
        vline(x1, y0, y1, col)

    if kind == "general":
        for i, width in enumerate((10, 7, 11)):
            y = 4 + i * 3
            hline(y, 3, 3 + width)
            px(3, y - 1, d)
    elif kind == "input":
        rect(2, 5, 13, 11)
        hline(8, 4, 6)
        vline(5, 7, 9)
        px(10, 7)
        px(11, 8)
        px(10, 9)
        px(9, 8)
    elif kind == "network":
        rect(3, 3, 12, 12)
        hline(7, 3, 12)
        vline(7, 3, 12)
        px(5, 5)
        px(10, 5)
        px(5, 10)
        px(10, 10)
    elif kind == "web":
        for y in range(3, 13):
            for x in range(3, 13):
                dx, dy = x - 7.5, y - 7.5
                dist = math.sqrt(dx * dx + dy * dy)
                if 4.4 <= dist <= 5.2:
                    px(x, y)
        hline(7, 3, 12)
        vline(7, 3, 12)
    elif kind == "graphics":
        rect(2, 4, 13, 11)
        hline(13, 6, 9)
        hline(12, 5, 10, d)
    elif kind == "audio":
        px(3, 7)
        px(3, 8)
        hline(6, 4, 6)
        hline(9, 4, 6)
        vline(6, 5, 10)
        px(8, 6, d)
        px(9, 5, d)
        px(10, 7, d)
        px(10, 8, d)
        px(9, 10, d)
        px(8, 9, d)
    elif kind == "chat":
        rect(2, 4, 13, 10)
        px(5, 11)
        px(6, 12)
        px(4, 10)
        hline(6, 5, 10, d)
        hline(8, 5, 8, d)
    elif kind == "voice":
        vline(7, 3, 8)
        vline(8, 3, 8)
        hline(3, 6, 9)
        hline(9, 5, 10)
        hline(11, 7, 8)
        hline(12, 6, 9)
    elif kind == "im":
        rect(2, 4, 13, 11)
        px(3, 5)
        px(4, 6)
        px(5, 7)
        px(6, 8)
        px(7, 7)
        px(8, 6)
        px(9, 5)
        px(10, 6)
        px(11, 7)
        px(12, 8)
    elif kind == "notify":
        hline(4, 6, 9)
        vline(5, 5, 9)
        vline(10, 5, 9)
        hline(10, 4, 11)
        hline(12, 6, 9)
        px(7, 3)
        px(8, 3)
    elif kind == "skins":
        rect(3, 4, 8, 9)
        rect(7, 7, 12, 12)
        px(5, 6)
        px(10, 9)
    elif kind == "grids":
        rect(3, 3, 7, 7)
        rect(9, 3, 13, 7)
        rect(3, 9, 7, 13)
        rect(9, 9, 13, 13)
    elif kind == "advchat":
        rect(2, 3, 10, 8)
        rect(5, 8, 13, 13)
    elif kind == "system":
        rect(5, 5, 10, 10)
        px(7, 3)
        px(8, 3)
        px(7, 12)
        px(8, 12)
        px(3, 7)
        px(3, 8)
        px(12, 7)
        px(12, 8)
        px(4, 4)
        px(11, 4)
        px(4, 11)
        px(11, 11)
    elif kind == "vanity":
        # head
        for y in range(3, 7):
            for x in range(6, 10):
                if abs(x - 7.5) + abs(y - 4.5) < 2.6:
                    px(x, y)
        # shoulders
        hline(8, 4, 11)
        vline(4, 8, 12)
        vline(11, 8, 12)
        hline(12, 4, 11)
    elif kind == "translate":
        hline(4, 4, 11)
        vline(7, 4, 12)
        px(5, 6)
        px(6, 8)
        px(8, 8)
        px(9, 10)
        hline(12, 5, 10)
    elif kind == "about":
        # i
        hline(3, 7, 8)
        vline(7, 6, 12)
        vline(8, 6, 12)
        hline(6, 6, 9)
    elif kind == "help":
        hline(4, 5, 10)
        vline(10, 4, 7)
        hline(7, 7, 10)
        vline(7, 7, 9)
        px(7, 11)
        px(8, 11)
    else:
        rect(4, 4, 11, 11)
    return pixels


def make_checkbox(checked=False, disabled=False):
    w = h = 16
    pixels = blank(w, h)
    if disabled:
        fill_rounded(pixels, w, h, 1, 1, 14, 14, (*DISABLED, 255), 3)
        stroke_rounded(pixels, w, h, 1, 1, 14, 14, (*GRAY, 160), 3)
        if checked:
            _draw_check(pixels, w, GRAY)
        return pixels
    if checked:
        fill_rounded(pixels, w, h, 1, 1, 14, 14, (*PURPLE, 255), 3)
        stroke_rounded(pixels, w, h, 1, 1, 14, 14, (*LAVENDER, 80), 3)
        _draw_check(pixels, w, WHITE)
    else:
        fill_rounded(pixels, w, h, 1, 1, 14, 14, (*BG_FIELD, 255), 3)
        stroke_rounded(pixels, w, h, 1, 1, 14, 14, (*BORDER, 230), 3)
    return pixels


def _draw_check(pixels, w, color):
    # two-stroke check
    pts = [(4, 8), (5, 9), (6, 10), (7, 11), (8, 10), (9, 8), (10, 6), (11, 4)]
    for x, y in pts:
        set_px(pixels, w, x, y, (*color, 255))
        set_px(pixels, w, x, y - 1, (*color, 200))


def make_radio(selected=False, disabled=False):
    w = h = 16
    pixels = blank(w, h)
    cx, cy, r = 7.5, 7.5, 6.4
    ring = GRAY if disabled else BORDER
    fill = DISABLED if disabled else BG_FIELD
    for y in range(h):
        for x in range(w):
            d = math.sqrt((x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2)
            if d <= r - 1.2:
                set_px(pixels, w, x, y, (*fill, 255))
            elif d <= r:
                t = 1.0 - abs(d - (r - 0.5))
                set_px(pixels, w, x, y, (*ring, clamp(255 * t)))
    if selected and not disabled:
        for y in range(h):
            for x in range(w):
                d = math.sqrt((x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2)
                if d <= 3.2:
                    set_px(pixels, w, x, y, (*PURPLE, 255))
    return pixels


def make_slider_thumb(w=32, h=16):
    pixels = blank(w, h)
    cx, cy, r = w / 2.0 - 0.5, h / 2.0 - 0.5, 6.2
    for y in range(h):
        for x in range(w):
            d = math.sqrt((x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2)
            if d <= r:
                t = d / r
                c = lerp_color(WHITE, LAVENDER, t * 0.45)
                set_px(pixels, w, x, y, (*c, 255))
            elif d <= r + 1.8:
                glow = 1.0 - (d - r) / 1.8
                blend_px(pixels, w, x, y, (*PURPLE, clamp(90 * glow)))
    return pixels


def make_slider_groove(w=32, h=8):
    pixels = blank(w, h)
    fill_rounded(pixels, w, h, 0, 2, w - 1, h - 3, (*BG_DEEP, 255), 3)
    stroke_rounded(pixels, w, h, 0, 2, w - 1, h - 3, (*BORDER, 160), 3)
    return pixels


def make_slider_highlight(w=32, h=4):
    pixels = blank(w, h)
    fill_rounded_gradient_h(pixels, w, h, 0, 0, w - 1, h - 1, PURPLE, BLUE, 2)
    return pixels


def make_progress(w=128, h=8, fill=False):
    pixels = blank(w, h)
    if fill:
        fill_rounded_gradient_h(pixels, w, h, 0, 0, w - 1, h - 1, PURPLE, BLUE, 3)
    else:
        fill_rounded(pixels, w, h, 0, 0, w - 1, h - 1, (*BG_DEEP, 255), 3)
        stroke_rounded(pixels, w, h, 0, 0, w - 1, h - 1, (*BORDER, 160), 3)
    return pixels


def make_toolbar_btn(w=128, h=32, selected=False, disabled=False):
    """Same chrome as UI buttons so the bottom toolbar matches the rest of Cyber."""
    return make_button(w, h, selected=selected, disabled=disabled)


def make_flyout_btn(w=128, h=32, selected=False, disabled=False, left=True):
    """Rounded flyout halves matching make_button (left stretches; right is arrow cap)."""
    if left:
        return make_button(w, h, selected=selected, disabled=disabled)
    return make_button(32, 32, selected=selected, disabled=disabled)


def make_chatbar_btn(w=128, h=32, selected=False):
    return make_button(w, h, selected=selected)


def make_rounded_template(w=32, h=32, radius=6):
    """White 9-slice rounded rect, tinted at draw time."""
    pixels = blank(w, h)
    fill_rounded(pixels, w, h, 0, 0, w - 1, h - 1, (*WHITE, 255), radius)
    return pixels


def make_textfield(w=32, h=24):
    pixels = blank(w, h)
    fill_rounded(pixels, w, h, 0, 0, w - 1, h - 1, (*BG_FIELD, 255), 4)
    stroke_rounded(pixels, w, h, 0, 0, w - 1, h - 1, (*BORDER, 220), 4)
    return pixels


def make_scrollbar_bg(w=16, h=64):
    pixels = blank(w, h)
    fill_rect(pixels, w, 0, 0, w - 1, h - 1, (*BG_DEEP, 255))
    return pixels


def make_scrollbar_thumb(w=16, h=64):
    pixels = blank(w, h)
    fill_rounded(pixels, w, h, 2, 2, w - 3, h - 3, (90, 96, 150, 255), 4)
    return pixels


def make_toolbar_bg(w=128, h=32):
    pixels = blank(w, h)
    fill_gradient_v(pixels, w, h, BG_PANEL, BG_DEEP)
    draw_hline(pixels, w, 0, 0, w - 1, (*BORDER, 160), 1)
    return pixels


def make_icon_button(w=16, h=16, kind="close", active=True, pressed=False):
    pixels = blank(w, h)
    col = TEXT if active else GRAY
    if pressed:
        fill_rounded(pixels, w, h, 0, 0, w - 1, h - 1, (*PURPLE, 80), 3)
    cx, cy = w // 2, h // 2
    if kind == "close":
        for i in range(-4, 5):
            set_px(pixels, w, cx + i, cy + i, (*col, 255))
            set_px(pixels, w, cx + i, cy - i, (*col, 255))
            set_px(pixels, w, cx + i + 1, cy + i, (*col, 180))
            set_px(pixels, w, cx + i + 1, cy - i, (*col, 180))
    elif kind == "minimize":
        for x in range(cx - 4, cx + 5):
            set_px(pixels, w, x, cy + 2, (*col, 255))
            set_px(pixels, w, x, cy + 3, (*col, 200))
    elif kind == "restore":
        for x in range(cx - 3, cx + 4):
            set_px(pixels, w, x, cy - 3, (*col, 255))
            set_px(pixels, w, x, cy + 3, (*col, 255))
        for y in range(cy - 3, cy + 4):
            set_px(pixels, w, cx - 3, y, (*col, 255))
            set_px(pixels, w, cx + 3, y, (*col, 255))
    elif kind == "tearoff":
        for x in range(3, 13):
            set_px(pixels, w, x, 6, (*col, 220))
            set_px(pixels, w, x, 9, (*col, 220))
    return pixels


def make_arrow(w=16, h=16, direction="up", active=True):
    pixels = blank(w, h)
    col = TEXT if active else GRAY
    cx, cy = w // 2, h // 2
    if direction == "up":
        pts = [(0, 3), (-1, 2), (1, 2), (-2, 1), (2, 1), (-3, 0), (3, 0)]
        for dx, dy in pts:
            set_px(pixels, w, cx + dx, cy - dy, (*col, 255))
    elif direction == "down":
        pts = [(0, 3), (-1, 2), (1, 2), (-2, 1), (2, 1), (-3, 0), (3, 0)]
        for dx, dy in pts:
            set_px(pixels, w, cx + dx, cy + dy, (*col, 255))
    elif direction == "left":
        pts = [(-3, 0), (-2, -1), (-2, 1), (-1, -2), (-1, 2), (0, -3), (0, 3)]
        for dx, dy in pts:
            set_px(pixels, w, cx + dx, cy + dy, (*col, 255))
    else:
        pts = [(3, 0), (2, -1), (2, 1), (1, -2), (1, 2), (0, -3), (0, 3)]
        for dx, dy in pts:
            set_px(pixels, w, cx + dx, cy + dy, (*col, 255))
    return pixels


def make_combobox_arrow(w=16, h=16):
    return make_arrow(w, h, "down", True)


def write_named(out_dir, name, pixels, ww, hh):
    path = os.path.join(out_dir, name)
    save_tga(path, ww, hh, pixels)
    print(f"  wrote {name}")


def write_all(out_dir):
    os.makedirs(out_dir, exist_ok=True)

    specs = [
        ("button_enabled_32x128.tga", lambda: make_button(selected=False), 128, 32),
        ("button_enabled_selected_32x128.tga", lambda: make_button(selected=True), 128, 32),
        ("button_disabled_32x128.tga", lambda: make_button(disabled=True), 128, 32),
        ("square_btn_32x128.tga", lambda: make_square_button(selected=False), 128, 32),
        ("square_btn_selected_32x128.tga", lambda: make_square_button(selected=True), 128, 32),
        ("tab_top_blue.tga", lambda: make_tab(64, 16, False), 64, 16),
        ("tab_top_selected_blue.tga", lambda: make_tab(64, 16, True), 64, 16),
        ("tab_bottom_blue.tga", lambda: make_tab(64, 16, False), 64, 16),
        ("tab_bottom_selected_blue.tga", lambda: make_tab(64, 16, True), 64, 16),
        ("tab_left.tga", lambda: make_sidebar_tab(172, 28, False), 172, 28),
        ("tab_left_selected.tga", lambda: make_sidebar_tab(172, 28, True), 172, 28),
        ("pref_section_rule.tga", lambda: make_section_rule(), 256, 2),
        ("pref_nav_general.tga", lambda: make_nav_icon("general"), 16, 16),
        ("pref_nav_input.tga", lambda: make_nav_icon("input"), 16, 16),
        ("pref_nav_network.tga", lambda: make_nav_icon("network"), 16, 16),
        ("pref_nav_web.tga", lambda: make_nav_icon("web"), 16, 16),
        ("pref_nav_graphics.tga", lambda: make_nav_icon("graphics"), 16, 16),
        ("pref_nav_audio.tga", lambda: make_nav_icon("audio"), 16, 16),
        ("pref_nav_chat.tga", lambda: make_nav_icon("chat"), 16, 16),
        ("pref_nav_voice.tga", lambda: make_nav_icon("voice"), 16, 16),
        ("pref_nav_im.tga", lambda: make_nav_icon("im"), 16, 16),
        ("pref_nav_notify.tga", lambda: make_nav_icon("notify"), 16, 16),
        ("pref_nav_skins.tga", lambda: make_nav_icon("skins"), 16, 16),
        ("pref_nav_grids.tga", lambda: make_nav_icon("grids"), 16, 16),
        ("pref_nav_advchat.tga", lambda: make_nav_icon("advchat"), 16, 16),
        ("pref_nav_system.tga", lambda: make_nav_icon("system"), 16, 16),
        ("pref_nav_vanity.tga", lambda: make_nav_icon("vanity"), 16, 16),
        ("pref_nav_translate.tga", lambda: make_nav_icon("translate"), 16, 16),
        ("pref_nav_about.tga", lambda: make_nav_icon("about"), 16, 16),
        ("pref_nav_help.tga", lambda: make_nav_icon("help"), 16, 16),
        ("checkbox_enabled_false.tga", lambda: make_checkbox(False, False), 16, 16),
        ("checkbox_enabled_true.tga", lambda: make_checkbox(True, False), 16, 16),
        ("checkbox_disabled_false.tga", lambda: make_checkbox(False, True), 16, 16),
        ("checkbox_disabled_true.tga", lambda: make_checkbox(True, True), 16, 16),
        ("radio_active_false.tga", lambda: make_radio(False, False), 16, 16),
        ("radio_active_true.tga", lambda: make_radio(True, False), 16, 16),
        ("radio_inactive_false.tga", lambda: make_radio(False, True), 16, 16),
        ("radio_inactive_true.tga", lambda: make_radio(True, True), 16, 16),
        ("icn_slide-thumb_dark.tga", lambda: make_slider_thumb(), 32, 16),
        ("icn_slide-groove_dark.tga", lambda: make_slider_groove(), 32, 8),
        ("icn_slide-highlight.tga", lambda: make_slider_highlight(), 32, 4),
        ("progressbar_fill.tga", lambda: make_progress(fill=True), 128, 8),
        ("progressbar_track.tga", lambda: make_progress(fill=False), 128, 8),
        ("progress_fill.tga", lambda: make_progress(64, 8, fill=True), 64, 8),
        ("toolbar_btn_enabled.tga", lambda: make_toolbar_btn(selected=False), 128, 32),
        ("toolbar_btn_selected.tga", lambda: make_toolbar_btn(selected=True), 128, 32),
        ("toolbar_btn_disabled.tga", lambda: make_toolbar_btn(disabled=True), 128, 32),
        ("flyout_btn_left.tga", lambda: make_flyout_btn(left=True), 128, 32),
        ("flyout_btn_left_selected.tga", lambda: make_flyout_btn(selected=True, left=True), 128, 32),
        ("flyout_btn_left_disabled.tga", lambda: make_flyout_btn(disabled=True, left=True), 128, 32),
        ("flyout_btn_right.tga", lambda: make_flyout_btn(left=False), 32, 32),
        ("flyout_btn_right_selected.tga", lambda: make_flyout_btn(selected=True, left=False), 32, 32),
        ("flyout_btn_right_disabled.tga", lambda: make_flyout_btn(disabled=True, left=False), 32, 32),
        ("btn_chatbar.tga", lambda: make_chatbar_btn(selected=False), 128, 32),
        ("btn_chatbar_selected.tga", lambda: make_chatbar_btn(selected=True), 128, 32),
        ("sm_rounded_corners_simple.tga", lambda: make_rounded_template(32, 32, 6), 32, 32),
        ("rounded_square.tga", lambda: make_rounded_template(128, 128, 12), 128, 128),
        ("icn_textfield_enabled.tga", lambda: make_textfield(), 32, 24),
        ("icn_scrollbar_bg.tga", lambda: make_scrollbar_bg(), 16, 64),
        ("icn_scrollbar_thumb.tga", lambda: make_scrollbar_thumb(), 16, 64),
        ("toolbar_bg.tga", lambda: make_toolbar_bg(), 128, 32),
        ("closebox.tga", lambda: make_icon_button(kind="close", active=True), 16, 16),
        ("close_in_blue.tga", lambda: make_icon_button(kind="close", active=True, pressed=True), 16, 16),
        ("close_inactive_blue.tga", lambda: make_icon_button(kind="close", active=False), 16, 16),
        ("minimize.tga", lambda: make_icon_button(kind="minimize", active=True), 16, 16),
        ("minimize_pressed.tga", lambda: make_icon_button(kind="minimize", active=True, pressed=True), 16, 16),
        ("minimize_inactive.tga", lambda: make_icon_button(kind="minimize", active=False), 16, 16),
        ("restore.tga", lambda: make_icon_button(kind="restore", active=True), 16, 16),
        ("restore_pressed.tga", lambda: make_icon_button(kind="restore", active=True, pressed=True), 16, 16),
        ("restore_inactive.tga", lambda: make_icon_button(kind="restore", active=False), 16, 16),
        ("tearoffbox.tga", lambda: make_icon_button(kind="tearoff", active=True), 16, 16),
        ("tearoff_pressed.tga", lambda: make_icon_button(kind="tearoff", active=True, pressed=True), 16, 16),
        ("scrollbutton_up_out_blue.tga", lambda: make_arrow(direction="up", active=True), 16, 16),
        ("scrollbutton_up_in_blue.tga", lambda: make_arrow(direction="up", active=True), 16, 16),
        ("scrollbutton_down_out_blue.tga", lambda: make_arrow(direction="down", active=True), 16, 16),
        ("scrollbutton_down_in_blue.tga", lambda: make_arrow(direction="down", active=True), 16, 16),
        ("scrollbutton_left_out_blue.tga", lambda: make_arrow(direction="left", active=True), 16, 16),
        ("scrollbutton_left_in_blue.tga", lambda: make_arrow(direction="left", active=True), 16, 16),
        ("scrollbutton_right_out_blue.tga", lambda: make_arrow(direction="right", active=True), 16, 16),
        ("scrollbutton_right_in_blue.tga", lambda: make_arrow(direction="right", active=True), 16, 16),
        ("spin_up_out_blue.tga", lambda: make_arrow(direction="up", active=True), 16, 16),
        ("spin_up_in_blue.tga", lambda: make_arrow(direction="up", active=True), 16, 16),
        ("spin_down_out_blue.tga", lambda: make_arrow(direction="down", active=True), 16, 16),
        ("spin_down_in_blue.tga", lambda: make_arrow(direction="down", active=True), 16, 16),
        ("combobox_arrow.tga", lambda: make_combobox_arrow(), 16, 16),
    ]

    for name, factory, ww, hh in specs:
        pixels = factory()
        write_named(out_dir, name, pixels, ww, hh)

    print(f"Done: {len(specs)} textures -> {out_dir}")


if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(__file__), "..", "indra", "newview", "skins", "cyber", "textures"
    )
    write_all(os.path.normpath(target))
