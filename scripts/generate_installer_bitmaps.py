#!/usr/bin/env python3
"""Generate NSIS MUI header bitmaps (Windows 3.x 24-bit BMP, 150x57).

NSIS LoadImage only accepts classic BITMAPINFOHEADER (40-byte) BMPs.
Recommended MUI header size is 150x57. Palette matches skins/cyber/colors.xml.
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INSTALLER_DIR = ROOT / "indra" / "newview" / "installers" / "windows"

# Cyber tokens (colors.xml)
BG_DEEP = (8, 9, 22)
BG_PANEL = (18, 20, 43)
PURPLE = (139, 92, 246)
BLUE = (99, 102, 241)
LAVENDER = (167, 139, 250)

HEADER_W = 150
HEADER_H = 57
ICON_SIZE = 44


def clamp(v, lo=0, hi=255):
    return max(lo, min(hi, int(v + 0.5)))


def lerp(a, b, t):
    return a + (b - a) * t


def lerp_color(c1, c2, t):
    return tuple(clamp(lerp(c1[i], c2[i], t)) for i in range(3))


def load_bmp(path: Path):
    data = path.read_bytes()
    if data[:2] != b"BM":
        raise ValueError("%s is not a BMP" % path)
    offset = struct.unpack_from("<I", data, 10)[0]
    header_size = struct.unpack_from("<I", data, 14)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    _planes, bpp, compression = struct.unpack_from("<HHI", data, 26)
    bottom_up = height > 0
    height = abs(height)
    pixels = [(0, 0, 0, 0)] * (width * height)

    if bpp == 24 and compression == 0:
        row_size = ((width * 3 + 3) // 4) * 4
        for i in range(height):
            src_row = i if not bottom_up else (height - 1 - i)
            start = offset + src_row * row_size
            for x in range(width):
                b, g, r = data[start + x * 3 : start + x * 3 + 3]
                pixels[i * width + x] = (r, g, b, 255)
        return width, height, pixels

    if bpp == 32:
        row_size = width * 4
        for i in range(height):
            src_row = i if not bottom_up else (height - 1 - i)
            start = offset + src_row * row_size
            for x in range(width):
                b, g, r, a = data[start + x * 4 : start + x * 4 + 4]
                pixels[i * width + x] = (r, g, b, a)
        return width, height, pixels

    raise ValueError("%s: unsupported BMP bpp=%s compression=%s" % (path, bpp, compression))


def sample(pixels, w, h, x, y):
    x = min(max(x, 0.0), w - 1.0)
    y = min(max(y, 0.0), h - 1.0)
    x0, y0 = int(x), int(y)
    x1 = min(x0 + 1, w - 1)
    y1 = min(y0 + 1, h - 1)
    fx, fy = x - x0, y - y0

    def pix(xx, yy):
        return pixels[yy * w + xx]

    c00, c10, c01, c11 = pix(x0, y0), pix(x1, y0), pix(x0, y1), pix(x1, y1)
    out = []
    for k in range(4):
        v0 = c00[k] * (1 - fx) + c10[k] * fx
        v1 = c01[k] * (1 - fx) + c11[k] * fx
        out.append(v0 * (1 - fy) + v1 * fy)
    return out


def resize_rgba(pixels, w, h, nw, nh):
    out = []
    for y in range(nh):
        sy = (y + 0.5) * h / nh - 0.5
        for x in range(nw):
            sx = (x + 0.5) * w / nw - 0.5
            r, g, b, a = sample(pixels, w, h, sx, sy)
            out.append((clamp(r), clamp(g), clamp(b), clamp(a)))
    return out


def write_bmp24_win3(path: Path, w: int, h: int, pixels):
    """Classic Windows 3.x 24-bit BMP (40-byte BITMAPINFOHEADER, BI_RGB)."""
    row_size = ((w * 3 + 3) // 4) * 4
    pixel_bytes = row_size * h
    offset = 14 + 40
    header = bytearray(offset)
    header[0:2] = b"BM"
    struct.pack_into("<I", header, 2, offset + pixel_bytes)
    struct.pack_into("<I", header, 10, offset)
    struct.pack_into("<I", header, 14, 40)
    struct.pack_into("<i", header, 18, w)
    struct.pack_into("<i", header, 22, h)
    struct.pack_into("<H", header, 26, 1)
    struct.pack_into("<H", header, 28, 24)
    struct.pack_into("<I", header, 34, pixel_bytes)
    body = bytearray()
    pad = b"\x00" * (row_size - w * 3)
    for y in range(h - 1, -1, -1):
        for x in range(w):
            r, g, b = pixels[y * w + x][:3]
            body.extend((b, g, r))
        body.extend(pad)
    path.write_bytes(header + body)
    print("wrote %s (%dx%d, %d bytes)" % (path, w, h, offset + pixel_bytes))


def blend(dst, src):
    sr, sg, sb, sa = src
    t = sa / 255.0
    return (
        clamp(dst[0] * (1 - t) + sr * t),
        clamp(dst[1] * (1 - t) + sg * t),
        clamp(dst[2] * (1 - t) + sb * t),
    )


def make_header(icon_path: Path | None, accent):
    pixels = []
    for y in range(HEADER_H):
        for x in range(HEADER_W):
            t = x / max(HEADER_W - 1, 1)
            c = lerp_color(BG_DEEP, BG_PANEL, t * 0.85)
            # Soft corner glow
            gx = (x - HEADER_W * 0.72) / 40.0
            gy = (y - HEADER_H * 0.45) / 28.0
            glow = max(0.0, 1.0 - (gx * gx + gy * gy))
            c = lerp_color(c, accent, glow * 0.18)
            pixels.append(c)

    # Bottom accent bar (purple / blue)
    for y in range(HEADER_H - 3, HEADER_H):
        u = (y - (HEADER_H - 3)) / 2.0
        bar = lerp_color(accent, BLUE, u * 0.35)
        for x in range(HEADER_W):
            pixels[y * HEADER_W + x] = bar

    if icon_path and icon_path.is_file():
        iw, ih, ipix = load_bmp(icon_path)
        scaled = resize_rgba(ipix, iw, ih, ICON_SIZE, ICON_SIZE)
        ox = (HEADER_W - ICON_SIZE) // 2
        oy = (HEADER_H - 3 - ICON_SIZE) // 2
        for y in range(ICON_SIZE):
            for x in range(ICON_SIZE):
                src = scaled[y * ICON_SIZE + x]
                if src[3] < 8:
                    continue
                dx, dy = ox + x, oy + y
                if 0 <= dx < HEADER_W and 0 <= dy < HEADER_H - 3:
                    idx = dy * HEADER_W + dx
                    pixels[idx] = blend(pixels[idx], src)

    return pixels


def write_license_rtf():
    """GPL as RTF with Cyber text/background so NSIS RichEdit is not black-on-white."""
    src = ROOT / "doc" / "GPL-license.txt"
    dst = INSTALLER_DIR / "license.rtf"
    text = src.read_text(encoding="utf-8", errors="replace")

    def esc(line):
        return (
            line.replace("\\", "\\\\")
            .replace("{", "\\{")
            .replace("}", "\\}")
        )

    parts = [
        r"{\rtf1\ansi\ansicpg1252\deff0",
        r"{\fonttbl{\f0\fnil\fcharset0 Tahoma;}}",
        r"{\colortbl;\red236\green238\blue248;\red10\green11\blue30;}",
        r"\viewkind4\uc1\pard\cf1\highlight2\f0\fs16",
    ]
    for line in text.splitlines():
        parts.append(esc(line) + r"\par")
    parts.append("}")
    dst.write_bytes("\n".join(parts).encode("ascii", "replace"))
    print("wrote %s" % dst)


def main():
    install_icon = INSTALLER_DIR / "install_icon.BMP"
    if not install_icon.is_file():
        install_icon = INSTALLER_DIR / "install_icon.bmp"
    uninstall_icon = INSTALLER_DIR / "uninstall_icon.bmp"
    if not uninstall_icon.is_file():
        uninstall_icon = INSTALLER_DIR / "uninstall_icon.BMP"

    write_bmp24_win3(
        INSTALLER_DIR / "install_header.bmp",
        HEADER_W,
        HEADER_H,
        make_header(install_icon, PURPLE),
    )
    write_bmp24_win3(
        INSTALLER_DIR / "uninstall_header.bmp",
        HEADER_W,
        HEADER_H,
        make_header(uninstall_icon, LAVENDER),
    )
    write_license_rtf()
    return 0


if __name__ == "__main__":
    sys.exit(main())
