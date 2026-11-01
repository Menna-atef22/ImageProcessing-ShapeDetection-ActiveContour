#!/usr/bin/env python3
"""Generates the extra test images (circles, ellipses, snake objects) used by Assignment 2.

    python tools/make_test_images.py

Writes PNGs to images/input/Shapes & Snake_Test_Images/ . Needs Pillow only.
Shapes are drawn 4x oversized and down-sampled so that the edges are anti-aliased.
"""
import math
import os
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.join(os.path.dirname(__file__), "..", "images", "input", "Shapes & Snake_Test_Images")
S = 4  # super-sampling factor


def canvas(w, h, bg):
    img = Image.new("RGB", (w * S, h * S), bg)
    return img, ImageDraw.Draw(img)


def finish(img, w, h, name, gray=False, blur=0.6):
    img = img.resize((w, h), Image.LANCZOS)
    if blur:
        img = img.filter(ImageFilter.GaussianBlur(blur))
    if gray:
        img = img.convert("L").convert("RGB")
    os.makedirs(OUT, exist_ok=True)
    img.save(os.path.join(OUT, name))
    print("wrote", name)


def circle(d, cx, cy, r, outline=None, fill=None, width=3):
    d.ellipse([(cx - r) * S, (cy - r) * S, (cx + r) * S, (cy + r) * S], outline=outline, fill=fill, width=width * S)


def rotated_ellipse(d, cx, cy, a, b, ang_deg, outline=None, fill=None, width=3, steps=720):
    ang = math.radians(ang_deg)
    pts = []
    for i in range(steps):
        t = 2 * math.pi * i / steps
        x, y = a * math.cos(t), b * math.sin(t)
        pts.append(((cx + x * math.cos(ang) - y * math.sin(ang)) * S, (cy + x * math.sin(ang) + y * math.cos(ang)) * S))
    if fill:
        d.polygon(pts, fill=fill)
    if outline:
        d.line(pts + [pts[0]], fill=outline, width=width * S, joint="curve")


# 06 - circles (colour): three outlined circles of different radii + one filled disc
w, h = 400, 300
img, d = canvas(w, h, (245, 245, 240))
circle(d, 100, 100, 60, outline=(200, 30, 30), width=4)
circle(d, 270, 90, 40, outline=(30, 60, 200), width=4)
circle(d, 230, 215, 55, outline=(20, 140, 60), width=4)
circle(d, 80, 235, 28, fill=(230, 160, 20), width=1)
finish(img, w, h, "06_circles_color_test.png")

# 07 - ellipses (grayscale): two tilted ellipses + one axis-aligned
w, h = 420, 320
img, d = canvas(w, h, (235, 235, 235))
rotated_ellipse(d, 120, 110, 85, 40, 30, outline=(30, 30, 30), width=4)
rotated_ellipse(d, 300, 100, 70, 35, -45, outline=(60, 60, 60), width=4)
rotated_ellipse(d, 210, 240, 100, 45, 0, outline=(20, 20, 20), width=4)
finish(img, w, h, "07_ellipses_grayscale_test.png", gray=True)

# 08 - mixed: lines + circle + ellipse (colour)
w, h = 440, 330
img, d = canvas(w, h, (250, 250, 250))
d.line([(20 * S, 40 * S), (420 * S, 70 * S)], fill=(150, 30, 30), width=4 * S)
d.line([(40 * S, 300 * S), (400 * S, 160 * S)], fill=(30, 30, 150), width=4 * S)
circle(d, 110, 160, 55, outline=(20, 120, 40), width=4)
rotated_ellipse(d, 300, 220, 80, 38, 20, outline=(130, 60, 160), width=4)
finish(img, w, h, "08_mixed_shapes_color_test.png")

# 09 - snake object (grayscale): a kidney-shaped bright blob (has a concavity) on a dark noisy-free background
w, h = 320, 320
img, d = canvas(w, h, (40, 40, 40))
pts = []
for i in range(360):
    t = math.radians(i)
    r = 85 + 18 * math.cos(2 * t) + 10 * math.sin(3 * t) - 25 * math.exp(-((i - 90) / 25.0) ** 2)
    pts.append(((160 + r * math.cos(t)) * S, (160 + r * math.sin(t)) * S))
d.polygon(pts, fill=(215, 215, 215))
finish(img, w, h, "09_snake_blob_grayscale_test.png", gray=True, blur=1.2)

# 10 - snake object (colour): orange disc-like object with a bump on a blue background
w, h = 320, 320
img, d = canvas(w, h, (60, 90, 150))
pts = []
for i in range(360):
    t = math.radians(i)
    r = 80 + 12 * math.sin(4 * t)
    pts.append(((170 + r * math.cos(t)) * S, (150 + r * math.sin(t)) * S))
d.polygon(pts, fill=(235, 150, 40))
finish(img, w, h, "10_snake_flower_color_test.png", blur=1.2)

# 11 - snake object (grayscale): a plain square-ish object (shows corner handling)
w, h = 320, 320
img, d = canvas(w, h, (225, 225, 225))
d.rectangle([90 * S, 80 * S, 230 * S, 240 * S], fill=(50, 50, 50))
finish(img, w, h, "11_snake_rectangle_grayscale_test.png", gray=True, blur=1.2)
