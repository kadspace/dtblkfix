#!/usr/bin/env python3
"""Make DtBlkFix's alternate skins from the original images.

    make_skins.py <resources folder> <skins folder>

A skin is a folder holding the same stereo_*.png files that the plugin loads from its dtblkfix
folder. Each alternate skin here is a per-pixel recolor of the original images (same size, same
alpha), so everything lines up pixel for pixel. Only the Python standard library is used.
"""

import colorsys
import os
import struct
import sys
import zlib

IMAGES = [
    "stereo_fx_bg.png",
    "stereo_global_ctrl.png",
    "stereo_hslider_bg_coarse.png",
    "stereo_hslider_bg_fine.png",
    "stereo_hslider_knob_coarse.png",
    "stereo_hslider_knob_fine.png",
    "stereo_splash.png",
]

# the splash image is a photo behind the spectrograms: it's dimmed rather than inverted
PHOTOS = {"stereo_splash.png"}


def read_png(path):
    """Return (width, height, channels, rows) for an 8 bit RGB or RGBA, non-interlaced PNG."""
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(path + ": not a PNG")
    pos, idat, hdr = 8, b"", None
    while pos < len(data):
        n, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if kind == b"IHDR":
            hdr = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        pos += 12 + n
    width, height, depth, ctype, _, _, interlace = hdr
    if depth != 8 or ctype not in (2, 6) or interlace:
        raise ValueError(path + ": only 8 bit RGB/RGBA non-interlaced PNGs are supported")
    ch = 4 if ctype == 6 else 3
    raw = zlib.decompress(idat)
    stride = width * ch
    rows, prev, i = [], bytearray(stride), 0
    for _ in range(height):
        ftype, line = raw[i], bytearray(raw[i + 1:i + 1 + stride])
        i += 1 + stride
        for x in range(stride):
            a = line[x - ch] if x >= ch else 0
            b = prev[x]
            c = prev[x - ch] if x >= ch else 0
            if ftype == 1:
                line[x] = (line[x] + a) & 255
            elif ftype == 2:
                line[x] = (line[x] + b) & 255
            elif ftype == 3:
                line[x] = (line[x] + ((a + b) >> 1)) & 255
            elif ftype == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[x] = (line[x] + pr) & 255
        rows.append(line)
        prev = line
    return width, height, ch, rows


def write_png(path, width, height, ch, rows):
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(
            ">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    raw = b"".join(b"\x00" + bytes(r) for r in rows)
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6 if ch == 4 else 2, 0, 0, 0)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) +
                chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def dark(r, g, b, photo):
    """Dark: light, greyish colours become dark and dark ones light, keeping their hue; strong
    colours (the red amplitude curves) keep their lightness; the photo is dimmed."""
    h, l, s = colorsys.rgb_to_hls(r, g, b)
    if photo:
        return colorsys.hls_to_rgb(h, l * 0.42, s * 0.8)
    strength = min(1.0, s * (1.0 - abs(2.0 * l - 1.0)) * 2.5)  # 0 = greyish, 1 = strong colour
    l2 = 0.07 + 0.83 * (1.0 - l)
    return colorsys.hls_to_rgb(h, l2 + (l - l2) * strength, s)


def warm(r, g, b, photo):
    """Warm: the cool lavender greys turn to warm sand, other colours are kept."""
    h, l, s = colorsys.rgb_to_hls(r, g, b)
    if photo:
        return colorsys.hls_to_rgb(0.08, l, min(1.0, s * 0.5 + 0.12))
    if s < 0.5 or l > 0.75:
        h = (h + 0.45) % 1.0 if 0.5 < h < 0.85 else h
    return colorsys.hls_to_rgb(h, l, s)


SKINS = {"Dark": dark, "Warm": warm}


def recolor(rows, ch, fn, photo):
    cache = {}
    out = []
    for row in rows:
        line = bytearray(row)
        for x in range(0, len(line), ch):
            rgb = bytes(line[x:x + 3])
            if rgb not in cache:
                r, g, b = fn(rgb[0] / 255.0, rgb[1] / 255.0, rgb[2] / 255.0, photo)
                cache[rgb] = bytes(int(max(0.0, min(1.0, v)) * 255.0 + 0.5) for v in (r, g, b))
            line[x:x + 3] = cache[rgb]
        out.append(line)
    return out


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    src, dst = sys.argv[1], sys.argv[2]
    for name, fn in sorted(SKINS.items()):
        folder = os.path.join(dst, name)
        os.makedirs(folder, exist_ok=True)
        for image in IMAGES:
            w, h, ch, rows = read_png(os.path.join(src, image))
            write_png(os.path.join(folder, image), w, h, ch, recolor(rows, ch, fn, image in PHOTOS))
        print("made skin", folder)


if __name__ == "__main__":
    main()
