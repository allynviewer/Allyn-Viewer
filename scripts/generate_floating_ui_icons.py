#!/usr/bin/env python3
# @file generate_floating_ui_icons.py
#
# $LicenseInfo:firstyear=2026&license=viewerlgpl$
# Copyright (C) 2026 - Allyn Viewer
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation;
# version 2.1 of the License only.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
# $/LicenseInfo$
from __future__ import annotations

import argparse
import math
import struct
from pathlib import Path

from PIL import Image, ImageDraw

GRID = 24
SS = 8
STROKE = 1.8
ACCENT = (139, 92, 246)

ROOT = Path(__file__).resolve().parents[1]
TEX = ROOT / "indra" / "newview" / "skins" / "default" / "textures"
CHROME_DIR = "floating_ui"


class Glyph:
    def __init__(self, grid=GRID):
        self.grid = grid
        self.im = Image.new("L", (grid * SS, grid * SS), 0)
        self.d = ImageDraw.Draw(self.im)

    def line(self, *pts, w=STROKE, close=False, ink=255):
        p = [(x * SS, y * SS) for x, y in pts]
        if close:
            p.append(p[0])
        width = max(1, round(w * SS))
        self.d.line(p, fill=ink, width=width, joint="curve")
        r = width / 2.0
        for x, y in p:
            self.d.ellipse((x - r, y - r, x + r, y + r), fill=ink)
        return self

    def arc(self, cx, cy, r, a0, a1, w=STROKE, ink=255):
        steps = max(8, int(abs(a1 - a0) / 4))
        pts = []
        for i in range(steps + 1):
            a = math.radians(a0 + (a1 - a0) * i / steps)
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
        return self.line(*pts, w=w, ink=ink)

    def circle(self, cx, cy, r, w=STROKE, ink=255):
        return self.arc(cx, cy, r, 0, 360, w=w, ink=ink)

    def ellipse(self, cx, cy, rx, ry, w=STROKE, ink=255):
        pts = []
        for i in range(73):
            a = math.radians(i * 5)
            pts.append((cx + rx * math.cos(a), cy + ry * math.sin(a)))
        return self.line(*pts, w=w, ink=ink)

    def disc(self, cx, cy, r, ink=255):
        self.d.ellipse(((cx - r) * SS, (cy - r) * SS, (cx + r) * SS, (cy + r) * SS), fill=ink)
        return self

    def rrect(self, x0, y0, x1, y1, rad=2.5, w=STROKE, ink=255):
        pts = []
        corners = (
            (x1 - rad, y0 + rad, -90),
            (x1 - rad, y1 - rad, 0),
            (x0 + rad, y1 - rad, 90),
            (x0 + rad, y0 + rad, 180),
        )
        for cx, cy, start in corners:
            for i in range(7):
                a = math.radians(start + 15 * i)
                pts.append((cx + rad * math.cos(a), cy + rad * math.sin(a)))
        return self.line(*pts, w=w, close=True, ink=ink)

    def fill(self, *pts, ink=255):
        self.d.polygon([(x * SS, y * SS) for x, y in pts], fill=ink)
        return self

    def box(self, x0, y0, x1, y1, ink=255):
        self.d.rectangle((x0 * SS, y0 * SS, x1 * SS, y1 * SS), fill=ink)
        return self

    def pie(self, cx, cy, r, a0, a1, ink=255):
        self.d.pieslice(((cx - r) * SS, (cy - r) * SS, (cx + r) * SS, (cy + r) * SS), a0, a1, fill=ink)
        return self

    def mask(self):
        return self.im.resize((self.grid, self.grid), Image.LANCZOS)

    def rgba(self, color=(255, 255, 255)):
        out = Image.new("RGBA", (self.grid, self.grid), color + (0,))
        out.putalpha(self.mask())
        return out


def person(g, cx, head_y=8.0, head_r=3.2, body_r=6.8, base=20.5):
    g.circle(cx, head_y, head_r)
    g.arc(cx, base, body_r, 188, 352)
    return g


def window(g):
    g.rrect(3, 4.5, 21, 19.5, 2.5)
    g.line((3.4, 9), (20.6, 9))
    return g


def magnifier(g, cx=10.5, cy=10.5, r=6.3):
    g.circle(cx, cy, r)
    k = r * 0.7071
    g.line((cx + k + 0.4, cy + k + 0.4), (20.5, 20.5), w=STROKE + 0.3)
    return g


def bubble(g):
    g.rrect(3, 4, 21, 16.5, 4)
    g.line((7.5, 16.5), (7.5, 20.5), (12, 16.5))
    return g


def flag(g):
    g.line((7, 20.5), (7, 4))
    g.line((7, 4.5), (18.5, 7.5), (7, 11.5))
    return g


def lock(g):
    g.rrect(5.5, 10.5, 18.5, 20.5, 2.5)
    g.line((8.2, 10.5), (8.2, 8))
    g.line((15.8, 10.5), (15.8, 8))
    g.arc(12, 8, 3.8, 180, 360)
    return g


def hanger(g):
    g.line((12, 9), (3.5, 16.5), (20.5, 16.5), close=True)
    g.arc(12, 6.4, 2.3, 150, 450)
    return g


def brackets(g):
    g.line((8, 6.5), (3.5, 12), (8, 17.5))
    g.line((16, 6.5), (20.5, 12), (16, 17.5))
    return g


def tray(g):
    g.line((4, 14.5), (4, 19.5), (20, 19.5), (20, 14.5))
    return g


def cloud(g, dx=0.0, dy=0.0):
    g.arc(8 + dx, 15 + dy, 3.6, 90, 270)
    g.arc(11.3 + dx, 11.4 + dy, 4.2, 195, 345)
    g.arc(16.2 + dx, 14.8 + dy, 3.8, 255, 450)
    g.line((8 + dx, 18.6 + dy), (16.2 + dx, 18.6 + dy))
    return g


def sun(g, cx=12.0, cy=12.0, r=3.8, inner=6.4, outer=8.8):
    g.circle(cx, cy, r)
    for i in range(8):
        a = math.radians(i * 45)
        g.line((cx + inner * math.cos(a), cy + inner * math.sin(a)),
               (cx + outer * math.cos(a), cy + outer * math.sin(a)))
    return g


def speaker(g):
    g.line((3.5, 9.5), (7.5, 9.5), (12, 5.5), (12, 18.5), (7.5, 14.5), (3.5, 14.5), close=True)
    return g


def globe(g):
    g.circle(12, 12, 8.5)
    g.ellipse(12, 12, 3.6, 8.5)
    g.line((3.6, 12), (20.4, 12))
    return g


def gear(g, cx=12.0, cy=12.0):
    for i in range(8):
        a = math.radians(i * 45)
        g.line((cx + 6.0 * math.cos(a), cy + 6.0 * math.sin(a)),
               (cx + 8.3 * math.cos(a), cy + 8.3 * math.sin(a)), w=3.0)
    g.disc(cx, cy, 6.9)
    g.disc(cx, cy, 5.0, ink=0)
    g.circle(cx, cy, 2.0, w=1.5)
    return g


def star(g, cx=12.0, cy=12.5, outer=9.0, inner=4.0, points=5, rot=-90.0):
    pts = []
    for i in range(points * 2):
        r = outer if i % 2 == 0 else inner
        a = math.radians(rot + i * 180.0 / points)
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    g.line(*pts, close=True)
    return g


def info_mark(g, cx=12.0, top=7.6, bottom=16.8):
    g.disc(cx, top, 1.25)
    g.line((cx, top + 3.3), (cx, bottom))
    return g


def alert_mark(g, cx=12.0, top=7.2, bottom=16.8):
    g.line((cx, top), (cx, bottom - 3.6))
    g.disc(cx, bottom, 1.25)
    return g


def plus(g, cx, cy, r, w=STROKE):
    g.line((cx - r, cy), (cx + r, cy), w=w)
    g.line((cx, cy - r), (cx, cy + r), w=w)
    return g


def stick_figure(g, cx=12.0):
    g.disc(cx, 5, 2.0)
    g.line((cx, 8.4), (cx, 14.5))
    g.line((cx, 14.5), (cx - 3.5, 20.5))
    g.line((cx, 14.5), (cx + 3.5, 20.5))
    g.line((cx - 5, 10.5), (cx, 9.4), (cx + 5, 10.5))
    return g


def icon_about_land(g):
    g.rrect(3.5, 3.5, 20.5, 20.5, 3)
    info_mark(g)


def icon_about_region(g):
    g.line((12, 3.5), (21, 9), (12, 14.5), (3, 9), close=True)
    g.line((3, 14.5), (12, 20), (21, 14.5))


def icon_about_viewer(g):
    g.circle(12, 12, 8.5)
    info_mark(g)


def icon_anims_explorer(g):
    g.rrect(3, 5, 21, 19, 2.5)
    g.line((10, 9), (15.5, 12), (10, 15), close=True)
    g.fill((10, 9), (15.5, 12), (10, 15))


def icon_ao(g):
    stick_figure(g)
    g.arc(12, 12, 10, -22, 22)
    g.arc(12, 12, 10, 158, 202)


def icon_appearance(g):
    g.line((8.6, 4), (4, 7), (6, 11), (8.2, 9.8), (8.2, 20), (15.8, 20), (15.8, 9.8), (18, 11), (20, 7), (15.4, 4))
    g.arc(12, 4, 3.4, 0, 180)


def icon_areasearch(g):
    g.line((3.5, 8), (3.5, 3.5), (8, 3.5))
    g.line((16, 3.5), (20.5, 3.5), (20.5, 8))
    g.line((20.5, 16), (20.5, 20.5), (16, 20.5))
    g.line((8, 20.5), (3.5, 20.5), (3.5, 16))
    g.circle(11.2, 11.2, 3.6)
    g.line((14, 14), (16.8, 16.8))


def icon_asset_blacklist(g):
    g.rrect(5, 3.5, 19, 20.5, 2.5)
    g.line((9, 9), (15, 15))
    g.line((15, 9), (9, 15))


def icon_auto_replace(g):
    g.line((4, 8.5), (19.5, 8.5))
    g.line((16, 5), (19.5, 8.5), (16, 12))
    g.line((20, 15.5), (4.5, 15.5))
    g.line((8, 12), (4.5, 15.5), (8, 19))


def icon_avatar(g):
    person(g, 12, 8, 3.6, 7.6, 21.5)


def icon_beacons(g):
    g.disc(12, 9.5, 1.7)
    g.arc(12, 9.5, 4.6, -48, 48)
    g.arc(12, 9.5, 4.6, 132, 228)
    g.arc(12, 9.5, 8.2, -42, 42)
    g.arc(12, 9.5, 8.2, 138, 222)
    g.line((12, 12.5), (8.5, 20.5))
    g.line((12, 12.5), (15.5, 20.5))


def icon_build(g):
    g.line((12, 3), (20, 7.5), (20, 16.5), (12, 21), (4, 16.5), (4, 7.5), close=True)
    g.line((4, 7.5), (12, 12), (20, 7.5))
    g.line((12, 12), (12, 21))


def icon_bumps(g):
    star(g, 12, 12, 9.2, 5.2, 8, -90)


def dollar(g, cx=12.0, cy=12.0, s=1.0):
    pts = [(2.7, -3.4), (1.3, -4.4), (-1.3, -4.4), (-2.7, -3.2), (-2.7, -1.6), (-1.3, -0.4),
           (1.3, 0.4), (2.7, 1.6), (2.7, 3.2), (1.3, 4.4), (-1.3, 4.4), (-2.7, 3.4)]
    g.line(*[(cx + x * s, cy + y * s) for x, y in pts], w=STROKE - 0.2)
    g.line((cx, cy - 6.2 * s), (cx, cy + 6.2 * s), w=STROKE - 0.3)
    return g


def icon_buy_currency(g):
    g.circle(12, 12, 8.8)
    dollar(g, 12, 12, 0.9)


def icon_buy_land(g):
    g.line((6, 20.5), (6, 4))
    g.line((6, 4.5), (16, 7.3), (6, 10.6))
    plus(g, 16.5, 16.5, 3.5)


def icon_camera_controls(g):
    g.rrect(2.8, 7, 15, 17, 2.5)
    g.line((15, 10.6), (21, 7.6), (21, 16.4), (15, 13.4))


def icon_change_buttons(g):
    g.rrect(4, 4, 10.5, 10.5, 1.6)
    g.rrect(13.5, 4, 20, 10.5, 1.6)
    g.rrect(4, 13.5, 10.5, 20, 1.6)
    plus(g, 16.75, 16.75, 3.3)


def icon_day_cycle_editor(g):
    g.circle(12, 12, 8.5)
    g.pie(12, 12, 8.5, -90, 90)


def icon_debug_avatar(g):
    g.line((6.5, 3.8), (3.5, 3.8), (3.5, 20.2), (6.5, 20.2))
    g.line((17.5, 3.8), (20.5, 3.8), (20.5, 20.2), (17.5, 20.2))
    g.circle(12, 9, 2.7)
    g.arc(12, 19.5, 5.2, 195, 345)


def icon_debug_console(g):
    g.rrect(3, 4.5, 21, 19.5, 2.5)
    g.line((7, 9.5), (10.5, 12), (7, 14.5))
    g.line((12.5, 15), (17, 15))


def icon_debug_settings(g):
    for y, kx in ((6.5, 9), (12, 15), (17.5, 8)):
        g.line((4, y), (20, y))
        g.disc(kx, y, 2.9, ink=0)
        g.circle(kx, y, 2.1)


def icon_destinations(g):
    g.circle(12, 12, 8.6)
    g.line((16, 8), (13.6, 13.6), (8, 16), (10.4, 10.4), close=True)


def icon_display_name(g):
    g.rrect(2.8, 5.5, 21.2, 18.5, 2.5)
    g.disc(8.2, 10.4, 1.9)
    g.arc(8.2, 16.4, 3.4, 200, 340)
    g.line((13.8, 10), (18, 10))
    g.line((13.8, 14), (18, 14))


def icon_edit_ui(g):
    g.line((4, 20), (5, 15.5), (15.5, 5), (19, 8.5), (8.5, 19), close=True)
    g.line((13.2, 7.3), (16.7, 10.8))


def icon_env_editor(g):
    cloud(g, -1.0, 1.2)
    g.arc(17, 7.5, 3.0, 160, 420)
    g.line((17, 2.4), (17, 2.9))
    g.line((21.6, 7.5), (21.2, 7.5))
    g.line((20.4, 4.1), (20.1, 4.4))
    g.line((13.6, 4.1), (13.9, 4.4))


def icon_experiences(g):
    g.line((12, 3), (14.2, 9.8), (21, 12), (14.2, 14.2), (12, 21), (9.8, 14.2), (3, 12), (9.8, 9.8), close=True)


def icon_fast_timers(g):
    g.circle(12, 13.5, 7.4)
    g.line((12, 13.5), (12, 9.4))
    g.line((9.8, 3.2), (14.2, 3.2))
    g.line((12, 3.2), (12, 6))
    g.line((18.4, 7.2), (19.6, 6))


def icon_favorites(g):
    star(g, 12, 12.6, 9.4, 4.3, 5, -90)


def icon_fly(g):
    g.line((21, 3), (3, 10.5), (10, 13.6), (13, 21), close=True)
    g.line((10, 13.6), (21, 3))


def icon_font_test(g):
    g.line((5.5, 19.5), (12, 4.5), (18.5, 19.5))
    g.line((8, 14.2), (16, 14.2))


def icon_frame_console(g):
    window(g)
    g.line((7.5, 16.5), (7.5, 14), w=1.6)
    g.line((10.5, 16.5), (10.5, 12), w=1.6)
    g.line((13.5, 16.5), (13.5, 13.4), w=1.6)
    g.line((16.5, 16.5), (16.5, 11.8), w=1.6)


def icon_friends(g):
    person(g, 9, 8.3, 3.0, 6.4, 20.5)
    g.arc(16.2, 8.6, 2.6, -110, 110)
    g.arc(16.6, 20.5, 5.2, 275, 352)


def icon_gestures(g):
    g.line((8.2, 12), (8.2, 6.2))
    g.line((11, 11.5), (11, 4.2))
    g.line((13.8, 11.5), (13.8, 4.8))
    g.line((16.6, 12.5), (16.6, 7))
    g.line((8.2, 14.5), (5.2, 11.8))
    g.arc(12.4, 15.2, 4.9, 20, 175)
    g.line((16.6, 12.5), (17.2, 16))
    g.line((8.2, 12), (7.6, 15.6))


def icon_god_tools(g):
    g.line((3.5, 8), (7.5, 13), (12, 5.5), (16.5, 13), (20.5, 8), (18.6, 18.5), (5.4, 18.5), close=True)


def icon_grid_options(g):
    g.line((9, 4), (9, 20))
    g.line((15, 4), (15, 20))
    g.line((4, 9), (20, 9))
    g.line((4, 15), (20, 15))


def icon_group_titles(g):
    g.line((3.6, 12.6), (11.4, 4.6), (19.4, 4.6), (19.4, 12.6), (11.4, 20.4), close=True)
    g.circle(15.3, 8.7, 1.3, w=1.4)


def icon_groups(g):
    person(g, 12, 7.6, 2.9, 6.0, 20.5)
    g.arc(5.6, 9.4, 2.3, 70, 290)
    g.arc(5.2, 20.5, 4.4, 188, 262)
    g.arc(18.4, 9.4, 2.3, -110, 110)
    g.arc(18.8, 20.5, 4.4, 278, 352)


def icon_http_console(g):
    window(g)
    g.line((9, 17), (9, 11.5))
    g.line((7, 13.4), (9, 11.4), (11, 13.4))
    g.line((15, 11.5), (15, 17))
    g.line((13, 15.1), (15, 17.1), (17, 15.1))


def icon_im(g):
    bubble(g)
    g.disc(8, 10.2, 1.15)
    g.disc(12, 10.2, 1.15)
    g.disc(16, 10.2, 1.15)


def icon_inspect(g):
    magnifier(g)
    g.disc(10.5, 10.5, 1.5)


def icon_inventory(g):
    g.rrect(3, 8, 21, 20, 2.5)
    g.line((8.5, 8), (8.5, 5.6), (9.6, 4.5), (14.4, 4.5), (15.5, 5.6), (15.5, 8))
    g.line((3.4, 13.4), (20.6, 13.4))


def icon_joystick(g):
    g.rrect(2.5, 7, 21.5, 17.5, 5)
    plus(g, 8.2, 12.25, 2.2, w=1.6)
    g.disc(15, 13.6, 1.2)
    g.disc(17.6, 10.9, 1.2)


def icon_local_assets(g):
    g.rrect(3, 4.5, 21, 19.5, 2.5)
    g.line((3.6, 17), (9, 11.2), (13, 15.4), (15.4, 12.9), (20.4, 17.6))
    g.disc(15.6, 8.8, 1.5)


def icon_local_chat(g):
    bubble(g)
    g.line((7.5, 8.4), (16.5, 8.4))
    g.line((7.5, 12), (13.5, 12))


def icon_make_outfit(g):
    g.line((10, 8.6), (3, 14.8), (14.5, 14.8))
    g.line((10, 8.6), (13.6, 11.8))
    g.arc(10, 6.2, 2.1, 150, 450)
    plus(g, 17.5, 17.5, 3.2)


def icon_map(g):
    g.line((3, 6), (9, 4), (15, 6), (21, 4), (21, 18), (15, 20), (9, 18), (3, 20), close=True)
    g.line((9, 4), (9, 18))
    g.line((15, 6), (15, 20))


def icon_marketplace(g):
    g.line((2.8, 4.5), (5.8, 4.5), (8.4, 15.5), (18.6, 15.5), (20.6, 7.6), (6.6, 7.6))
    g.disc(9.6, 19.2, 1.6)
    g.disc(17.4, 19.2, 1.6)


def icon_marketplace_listings(g):
    g.line((4, 9.6), (5.6, 4.5), (18.4, 4.5), (20, 9.6), close=True)
    g.line((5.6, 9.6), (5.6, 20), (18.4, 20), (18.4, 9.6))
    g.line((10, 20), (10, 14.4), (14, 14.4), (14, 20))


def icon_media_filter(g):
    g.line((3.5, 5), (20.5, 5), (14, 12.6), (14, 19), (10, 21), (10, 12.6), close=True)


def icon_media_ticker(g):
    g.line((9, 17.2), (9, 6.2), (19, 4.2), (19, 15.2))
    g.disc(6.7, 17.4, 2.6)
    g.disc(16.7, 15.4, 2.6)


def icon_memleak(g):
    g.rrect(6.5, 6.5, 17.5, 17.5, 2)
    for p in (9.6, 14.4):
        g.line((p, 6.5), (p, 3.4))
        g.line((p, 17.5), (p, 20.6))
        g.line((6.5, p), (3.4, p))
        g.line((17.5, p), (20.6, p))
    g.disc(12, 12, 1.5)


def icon_message_log(g):
    for y in (6.5, 12, 17.5):
        g.disc(4.8, y, 1.3)
        g.line((9, y), (20, y))


def icon_minimap(g):
    g.circle(12, 12, 7.2)
    g.disc(12, 12, 1.7)
    g.line((12, 2.6), (12, 5.6))
    g.line((12, 18.4), (12, 21.4))
    g.line((2.6, 12), (5.6, 12))
    g.line((18.4, 12), (21.4, 12))


def icon_mouselook(g):
    g.arc(12, 18.2, 11, 214, 326)
    g.arc(12, 5.8, 11, 34, 146)
    g.circle(12, 12, 2.9)


def icon_movement_controls(g):
    g.line((12, 3.2), (12, 20.8))
    g.line((3.2, 12), (20.8, 12))
    g.line((9.6, 5.6), (12, 3.2), (14.4, 5.6))
    g.line((9.6, 18.4), (12, 20.8), (14.4, 18.4))
    g.line((5.6, 9.6), (3.2, 12), (5.6, 14.4))
    g.line((18.4, 9.6), (20.8, 12), (18.4, 14.4))


def icon_mute_list(g):
    g.circle(12, 12, 8.6)
    g.line((6, 6), (18, 18))


def icon_my_land(g):
    flag(g)
    g.line((3.5, 20.5), (20.5, 20.5))


def icon_notifications_console(g):
    window(g)
    g.line((12, 11.6), (12, 14.6))
    g.disc(12, 17, 1.0)


def icon_outfits(g):
    hanger(g)


def icon_pathfinding_characters(g):
    g.rrect(5.5, 8, 18.5, 18.5, 2.8)
    g.disc(9.5, 12.8, 1.3)
    g.disc(14.5, 12.8, 1.3)
    g.line((12, 8), (12, 5))
    g.disc(12, 4, 1.4)
    g.line((3, 11.6), (3, 15))
    g.line((21, 11.6), (21, 15))


def icon_pathing_linksets(g):
    g.rrect(2.8, 8.4, 13.2, 15.6, 3.6)
    g.rrect(10.8, 8.4, 21.2, 15.6, 3.6)


def icon_post_process(g):
    g.line((4, 20), (14.6, 9.4), w=2.2)
    plus(g, 18, 5.5, 2.2, w=1.5)
    plus(g, 19.5, 13.5, 1.6, w=1.4)
    plus(g, 10, 5, 1.6, w=1.4)


def icon_preferences(g):
    gear(g)


def icon_quit(g):
    g.arc(12, 13, 7.6, -58, 238)
    g.line((12, 3.4), (12, 11.6))


def icon_radar(g):
    g.circle(12, 12, 8.6)
    g.circle(12, 12, 4.4, w=1.4)
    g.line((12, 12), (18, 6))
    g.disc(12, 12, 1.4)


def icon_received_items(g):
    tray(g)
    g.line((12, 3.5), (12, 13.2))
    g.line((7.6, 9), (12, 13.4), (16.4, 9))


def icon_region_console(g):
    window(g)
    g.circle(12, 14.3, 2.9, w=1.4)
    g.line((8.2, 14.3), (15.8, 14.3), w=1.2)


def icon_region_tracker(g):
    g.arc(12, 9.8, 6.2, 150, 390)
    g.line((6.63, 12.9), (12, 21), (17.37, 12.9))
    g.circle(12, 9.8, 2.2)


def icon_report_abuse(g):
    g.line((12, 3.6), (21.4, 19.8), (2.6, 19.8), close=True)
    g.line((12, 9.6), (12, 14.2))
    g.disc(12, 16.9, 1.15)


def icon_rlv_locks(g):
    g.line((12, 3), (19.5, 5.8), (19.5, 12), (17.6, 16.4), (12, 21), (6.4, 16.4), (4.5, 12), (4.5, 5.8), close=True)
    g.disc(12, 10.4, 1.7)
    g.line((12, 11), (12, 15))


def icon_rlv_restrictions(g):
    lock(g)


def icon_rlv_strings(g):
    g.line((5, 6), (19, 6))
    g.line((12, 6), (12, 19))
    g.line((9.4, 19), (14.6, 19))
    g.line((5, 6), (5, 8.2))
    g.line((19, 6), (19, 8.2))


def icon_run(g):
    g.disc(15.2, 4.8, 2.0)
    g.line((14, 8.2), (11, 13.6))
    g.line((11, 13.6), (14.6, 16), (13.6, 20.6))
    g.line((11, 13.6), (8, 17.2), (4.4, 17.2))
    g.line((14, 8.6), (10, 9.4), (8, 12.4))
    g.line((14, 8.6), (17.4, 11.2), (20, 10.2))


def icon_script_errors(g):
    brackets(g)
    alert_mark(g, 12, 7.2, 16.8)


def icon_script_info(g):
    brackets(g)
    info_mark(g, 12, 7.4, 16.8)


def icon_search(g):
    magnifier(g)


def icon_sit(g):
    g.line((7, 3.8), (7, 20.4))
    g.line((7, 13), (17, 13))
    g.line((17, 13), (17, 20.4))
    g.line((7, 8.4), (10.5, 8.4))


def icon_snapshot(g):
    g.rrect(3, 7, 21, 19.5, 2.5)
    g.circle(12, 13.2, 3.5)
    g.line((8.4, 7), (10, 4.5), (14, 4.5), (15.6, 7))


def icon_sound_explorer(g):
    speaker(g)
    g.arc(13.6, 12, 3.4, -50, 50)
    g.arc(13.6, 12, 7.0, -48, 48)


def icon_stats(g):
    g.line((5, 20), (5, 12.5), w=2.4)
    g.line((9.7, 20), (9.7, 5), w=2.4)
    g.line((14.3, 20), (14.3, 9.5), w=2.4)
    g.line((19, 20), (19, 14.5), w=2.4)


def icon_teleport_history(g):
    g.arc(12.6, 12, 8.2, -140, 150)
    g.line((3.2, 5.4), (6.2, 6.9), (4.9, 10.1))
    g.line((12.6, 7.6), (12.6, 12), (15.8, 13.9))


def icon_test(g):
    g.line((9.5, 3.5), (9.5, 9.5), (4.6, 18.8), (5.6, 20.5), (18.4, 20.5), (19.4, 18.8), (14.5, 9.5), (14.5, 3.5))
    g.line((7.8, 3.5), (16.2, 3.5))
    g.line((7.2, 15), (16.8, 15))


def icon_texture_category_console(g):
    window(g)
    g.box(6.6, 11.6, 9.4, 13.4)
    g.line((12, 12.5), (17.2, 12.5), w=1.5)
    g.box(6.6, 15.4, 9.4, 17.2)
    g.line((12, 16.3), (17.2, 16.3), w=1.5)


def icon_texture_console(g):
    window(g)
    g.box(8.6, 11.4, 12, 14.2)
    g.box(12, 14.2, 15.4, 17)
    g.rrect(8.6, 11.4, 15.4, 17, 0.4, w=1.2)


def icon_texture_size_console(g):
    window(g)
    g.line((8, 16.6), (16, 11.4))
    g.line((12.6, 11.4), (16, 11.4), (16, 14.4))
    g.line((8, 13.6), (8, 16.6), (11.4, 16.6))


def icon_tutorial(g):
    g.circle(12, 12, 8.6)
    g.arc(12, 10, 2.8, 185, 400)
    g.line((14.1, 11.8), (12, 13.6), (12, 14.4))
    g.disc(12, 17.2, 1.15)


def icon_upload_perms(g):
    tray(g)
    g.line((12, 15.2), (12, 4))
    g.line((7.6, 8.4), (12, 4), (16.4, 8.4))


def icon_voice_effects(g):
    g.rrect(8, 3.5, 14, 13.5, 3)
    g.arc(11, 11.2, 6, 15, 165)
    g.line((11, 17.2), (11, 20.5))
    g.line((7.6, 20.5), (14.4, 20.5))
    plus(g, 19, 5.6, 2.2, w=1.5)


def icon_water_editor(g):
    g.arc(12, 14.4, 6.1, -12, 192)
    g.line((6.03, 13.1), (12, 3.4), (17.97, 13.1))


def icon_web(g):
    globe(g)


def icon_windlight(g):
    sun(g)


def icon_chat(g):
    g.rrect(2.5, 6, 21.5, 18, 2.6)
    for x in (6.4, 10.1, 13.9, 17.6):
        g.disc(x, 10.2, 1.05)
    g.line((7.5, 14.2), (16.5, 14.2))


def icon_active_speakers(g):
    person(g, 9, 8.4, 3.0, 6.4, 20.5)
    g.arc(13.5, 9.5, 4.2, -42, 42)
    g.arc(13.5, 9.5, 7.6, -40, 40)


def icon_nav_back(g):
    g.line((19.5, 12), (5, 12))
    g.line((11, 5.8), (4.8, 12), (11, 18.2))


def icon_nav_forward(g):
    g.line((4.5, 12), (19, 12))
    g.line((13, 5.8), (19.2, 12), (13, 18.2))


def icon_nav_home(g):
    g.line((3.5, 11.5), (12, 4), (20.5, 11.5))
    g.line((5.8, 9.8), (5.8, 20), (18.2, 20), (18.2, 9.8))
    g.line((10, 20), (10, 14.6), (14, 14.6), (14, 20))


def icon_nav_landmark(g):
    g.arc(10.5, 9.8, 5.8, 150, 390)
    g.line((5.48, 12.7), (10.5, 20.4), (15.52, 12.7))
    plus(g, 18.6, 17.4, 2.8, w=1.6)


def icon_plus(g):
    plus(g, 12, 12, 7.2, w=2.2)


def icon_bell(g):
    g.arc(12, 10.4, 5.6, 180, 360)
    g.line((6.4, 10.4), (6.4, 14.6), (4.4, 17.6), (19.6, 17.6), (17.6, 14.6), (17.6, 10.4))
    g.arc(12, 18.6, 2.2, 20, 160)
    g.line((12, 3.2), (12, 4.6))


def icon_volume(g):
    speaker(g)
    g.arc(13.6, 12, 3.4, -50, 50)
    g.arc(13.6, 12, 7.0, -48, 48)


def icon_volume_muted(g):
    speaker(g)
    g.line((15.4, 9.2), (20.6, 14.8))
    g.line((20.6, 9.2), (15.4, 14.8))


def icon_environment(g):
    icon_env_editor(g)


def icon_more(g):
    g.disc(5.5, 12, 1.9)
    g.disc(12, 12, 1.9)
    g.disc(18.5, 12, 1.9)


def icon_chevron_up(g):
    g.line((6.5, 14.6), (12, 9), (17.5, 14.6), w=2.0)


def icon_chevron_down(g):
    g.line((6.5, 9.4), (12, 15), (17.5, 9.4), w=2.0)


def icon_history(g):
    g.circle(12, 12, 8.6)
    g.line((12, 7.2), (12, 12), (15.4, 14))


TOOLBAR_TGA = [
    "about_land", "about_region", "about_viewer", "anims_explorer", "ao", "appearance", "areasearch",
    "asset_blacklist", "auto_replace", "avatar", "beacons", "build", "bumps", "buy_currency", "buy_land",
    "camera_controls", "change_buttons", "day_cycle_editor", "debug_avatar", "debug_console",
    "debug_settings", "destinations", "display_name", "edit_ui", "env_editor", "experiences",
    "fast_timers", "favorites", "fly", "font_test", "frame_console", "friends", "gestures", "god_tools",
    "grid_options", "group_titles", "groups", "http_console", "im", "inspect", "inventory", "joystick",
    "local_assets", "local_chat", "make_outfit", "map", "marketplace", "marketplace_listings",
    "media_filter", "media_ticker", "memleak", "message_log", "minimap", "mouselook",
    "movement_controls", "mute_list", "my_land", "notifications_console", "outfits",
    "pathfinding_characters", "pathing_linksets", "post_process", "preferences", "quit", "radar",
    "received_items", "region_console", "report_abuse", "rlv_locks", "rlv_restrictions", "rlv_strings",
    "run", "script_errors", "script_info", "search", "sit", "snapshot", "sound_explorer", "stats",
    "teleport_history", "test", "texture_category_console", "texture_console", "texture_size_console",
    "tutorial", "upload_perms", "voice_effects", "water_editor", "web", "windlight",
]

TOOLBAR_PNG = ["region_tracker"]

CHROME = [
    "chat", "active_speakers", "nav_back", "nav_forward", "nav_home", "nav_landmark", "plus", "bell",
    "volume", "volume_muted", "environment", "more", "chevron_up", "chevron_down", "history",
]

CHROME_SHARED = {
    "nav_land": "about_land",
    "nav_lighting": "windlight",
    "nav_search": "search",
    "ao": "ao",
    "mouselook": "mouselook",
}


def render(name, grid=GRID):
    g = Glyph(grid)
    globals()["icon_" + name](g)
    return g.rgba()


def write_tga(path, image):
    image = image.convert("RGBA")
    w, h = image.size
    px = image.load()
    data = bytearray(struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, w, h, 32, 8))
    for y in range(h - 1, -1, -1):
        for x in range(w):
            r, g, b, a = px[x, y]
            data += bytes((b, g, r, a))
    path.write_bytes(bytes(data))


def circle_mask(size, radius, stroke=None):
    big = Image.new("L", (size * SS, size * SS), 0)
    d = ImageDraw.Draw(big)
    c = size * SS / 2.0
    r = radius * SS
    d.ellipse((c - r, c - r, c + r, c + r), fill=255)
    if stroke is not None:
        r2 = (radius - stroke) * SS
        d.ellipse((c - r2, c - r2, c + r2, c + r2), fill=0)
    return big.resize((size, size), Image.LANCZOS)


def rounded_mask(size, radius, inset=0.0):
    big = Image.new("L", (size * SS, size * SS), 0)
    d = ImageDraw.Draw(big)
    d.rounded_rectangle((inset * SS, inset * SS, (size - inset) * SS - 1, (size - inset) * SS - 1),
                        radius=radius * SS, fill=255)
    return big.resize((size, size), Image.LANCZOS)


def tinted(mask, color, alpha=255):
    out = Image.new("RGBA", mask.size, color + (0,))
    out.putalpha(mask.point(lambda v: v * alpha // 255))
    return out


def dot_layer(size, color):
    big = Image.new("L", (size * SS, size * SS), 0)
    d = ImageDraw.Draw(big)
    cx = size * SS / 2.0
    cy = (size - 3.0) * SS
    r = 1.6 * SS
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=255)
    return tinted(big.resize((size, size), Image.LANCZOS), color)


def chrome_textures():
    white = (255, 255, 255)
    out = {}
    out["pill"] = tinted(circle_mask(64, 32.0), white)
    out["pill_border"] = tinted(circle_mask(64, 32.0, 2.4), white)
    chip = rounded_mask(32, 10.0, 1.0)
    idle = Image.new("RGBA", (32, 32), white + (0,))
    hover = tinted(chip, white, 30)
    pressed = tinted(chip, white, 54)
    dot = dot_layer(32, ACCENT)
    active = idle.copy()
    active.alpha_composite(dot)
    active_hover = hover.copy()
    active_hover.alpha_composite(dot)
    out["btn_idle"] = idle
    out["btn_hover"] = hover
    out["btn_pressed"] = pressed
    out["btn_active"] = active
    out["btn_active_hover"] = active_hover
    return out


def contact_sheet(items, path, cell=44, cols=16, scale=1):
    rows = (len(items) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * cell, rows * cell), (24, 24, 28, 255))
    for i, (name, image) in enumerate(items):
        im = image
        if scale != 1:
            im = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
        x = (i % cols) * cell + (cell - im.width) // 2
        y = (i // cols) * cell + (cell - im.height) // 2
        sheet.alpha_composite(im, (x, y))
    sheet.save(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preview", type=Path, default=None)
    args = parser.parse_args()

    chrome_dir = TEX / CHROME_DIR
    chrome_dir.mkdir(parents=True, exist_ok=True)

    rendered = []
    for name in TOOLBAR_TGA:
        image = render(name)
        write_tga(TEX / ("icn_toolbar_%s.tga" % name), image)
        rendered.append((name, image))
    for name in TOOLBAR_PNG:
        image = render(name)
        image.save(TEX / ("icn_toolbar_%s.png" % name))
        rendered.append((name, image))
    for name in CHROME:
        image = render(name)
        image.save(chrome_dir / ("%s.png" % name))
        rendered.append((name, image))
    for name, source in CHROME_SHARED.items():
        image = render(source)
        image.save(chrome_dir / ("%s.png" % name))
    for name, image in chrome_textures().items():
        image.save(chrome_dir / ("%s.png" % name))

    if args.preview:
        args.preview.mkdir(parents=True, exist_ok=True)
        contact_sheet(rendered, args.preview / "icons_1x.png")
        contact_sheet(rendered, args.preview / "icons_3x.png", cell=88, cols=12, scale=3)
    print("%d icons written to %s" % (len(rendered) + len(CHROME_SHARED), TEX))


if __name__ == "__main__":
    main()
