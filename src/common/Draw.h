#pragma once
#include "Image.h"
#include "Types.h"

// Simple drawing helpers (written from scratch) used by all detectors to
// superimpose results on an image.
struct Color {
    uint8_t r = 255, g = 0, b = 0;
};

namespace Draw {
void pixel(RgbImage& img, int x, int y, Color c);
void dot(RgbImage& img, int x, int y, int radius, Color c);  // filled square dot
void line(RgbImage& img, int x0, int y0, int x1, int y1, Color c, int thickness = 1);  // Bresenham
void circle(RgbImage& img, int cx, int cy, int r, Color c, int thickness = 1);          // midpoint
void ellipse(RgbImage& img, double cx, double cy, double a, double b, double angle, Color c,
             int thickness = 1);
}  // namespace Draw
