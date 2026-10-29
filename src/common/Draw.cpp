#include "Draw.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Draw {

void pixel(RgbImage& img, int x, int y, Color c) {
    if (x < 0 || y < 0 || x >= img.width || y >= img.height) return;
    uint8_t* p = img.px(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
}

void dot(RgbImage& img, int x, int y, int radius, Color c) {
    for (int dy = -radius; dy <= radius; ++dy)
        for (int dx = -radius; dx <= radius; ++dx) pixel(img, x + dx, y + dy, c);
}

void line(RgbImage& img, int x0, int y0, int x1, int y1, Color c, int thickness) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int h = thickness / 2;
    while (true) {
        if (thickness <= 1) pixel(img, x0, y0, c);
        else dot(img, x0, y0, h, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void circle(RgbImage& img, int cx, int cy, int r, Color c, int thickness) {
    int h = thickness / 2;
    auto put = [&](int px, int py) {
        if (thickness <= 1) pixel(img, px, py, c);
        else dot(img, px, py, h, c);
    };
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        put(cx + x, cy + y); put(cx - x, cy + y); put(cx + x, cy - y); put(cx - x, cy - y);
        put(cx + y, cy + x); put(cx - y, cy + x); put(cx + y, cy - x); put(cx - y, cy - x);
        ++y;
        if (err < 0) err += 2 * y + 1;
        else { --x; err += 2 * (y - x) + 1; }
    }
}

void ellipse(RgbImage& img, double cx, double cy, double a, double b, double angle, Color c,
             int thickness) {
    const double PI = 3.14159265358979323846;
    int steps = int(2 * PI * std::max(a, b)) + 16;
    double ca = std::cos(angle), sa = std::sin(angle);
    int px = 0, py = 0;
    for (int i = 0; i <= steps; ++i) {
        double t = 2 * PI * i / steps;
        double ex = a * std::cos(t), ey = b * std::sin(t);
        int x = int(std::lround(cx + ex * ca - ey * sa));
        int y = int(std::lround(cy + ex * sa + ey * ca));
        if (i > 0) line(img, px, py, x, y, c, thickness);
        px = x;
        py = y;
    }
}

}  // namespace Draw
