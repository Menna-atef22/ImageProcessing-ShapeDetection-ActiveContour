#include "ContourAnalysis.h"

#include <cmath>
#include <cstdlib>
#include <string>

namespace {

// Map an 8-connected single-pixel step (dx, dy) in {-1, 0, 1}^2 \ {(0, 0)}
// to its standard 8-direction Freeman chain code (0..7):
//   0: E (+1,  0)   1: NE (+1, -1)   2: N ( 0, -1)   3: NW (-1, -1)
//   4: W (-1,  0)   5: SW (-1, +1)   6: S ( 0, +1)   7: SE (+1, +1)
int freemanDirection(int dx, int dy) {
    if (dx ==  1 && dy ==  0) return 0;
    if (dx ==  1 && dy == -1) return 1;
    if (dx ==  0 && dy == -1) return 2;
    if (dx == -1 && dy == -1) return 3;
    if (dx == -1 && dy ==  0) return 4;
    if (dx == -1 && dy ==  1) return 5;
    if (dx ==  0 && dy ==  1) return 6;
    if (dx ==  1 && dy ==  1) return 7;
    return -1;
}

// Trace an 8-connected Bresenham path from p0 to p1 and append each pixel move's Freeman code.
void appendSegmentChainCode(const Point& p0, const Point& p1, std::vector<int>& code) {
    int x0 = p0.x, y0 = p0.y;
    const int x1 = p1.x, y1 = p1.y;
    const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (x0 != x1 || y0 != y1) {
        int e2 = 2 * err;
        int stepX = 0, stepY = 0;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
            stepX = sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
            stepY = sy;
        }
        int dir = freemanDirection(stepX, stepY);
        if (dir >= 0) code.push_back(dir);
    }
}

}  // namespace

ContourMeasurements ContourAnalysis::analyze(const std::vector<Point>& c) {
    ContourMeasurements m;
    m.numPoints = int(c.size());
    if (!c.empty()) m.start = c[0];
    m.chainCode = chainCode(c);
    m.perimeter = perimeter(c);
    m.area = area(c);
    m.chainPerimeter = chainPerimeter(m.chainCode);
    m.chainArea = chainArea(m.chainCode);
    return m;
}

namespace {
// (dx, dy) of each Freeman direction; inverse of freemanDirection().
constexpr int kDx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
constexpr int kDy[8] = {0, -1, -1, -1, 0, 1, 1, 1};
}  // namespace

double ContourAnalysis::chainPerimeter(const std::vector<int>& code) {
    double sum = 0.0;
    for (int c : code) sum += (c % 2 == 0) ? 1.0 : std::sqrt(2.0);
    return sum;
}

double ContourAnalysis::chainArea(const std::vector<int>& code) {
    // Walk the closed pixel path from (0, 0); each move adds the trapezoid (x + (x + dx)) / 2 * dy
    // (Green's theorem), i.e. the exact area of the polygon through the visited pixel centres.
    long long x = 0, twiceSum = 0;
    for (int c : code) {
        if (c < 0 || c > 7) continue;
        twiceSum += (2 * x + kDx[c]) * kDy[c];
        x += kDx[c];
    }
    return std::fabs(double(twiceSum)) / 2.0;
}

std::string ContourAnalysis::toString(const std::vector<int>& code) {
    std::string s;
    s.reserve(code.size());
    for (int c : code) s.push_back(char('0' + c));
    return s;
}

std::vector<Point> ContourAnalysis::decode(const Point& start, const std::vector<int>& code) {
    std::vector<Point> path{start};
    Point p = start;
    for (int c : code) {
        if (c < 0 || c > 7) continue;
        p.x += kDx[c];
        p.y += kDy[c];
        path.push_back(p);
    }
    return path;
}

std::vector<int> ContourAnalysis::chainCode(const std::vector<Point>& contour) {
    std::vector<int> code;
    const size_t n = contour.size();
    if (n < 2) return code;

    for (size_t i = 0; i < n; ++i) {
        const Point& p0 = contour[i];
        const Point& p1 = contour[(i + 1) % n];
        appendSegmentChainCode(p0, p1, code);
    }
    return code;
}

double ContourAnalysis::perimeter(const std::vector<Point>& contour) {
    const size_t n = contour.size();
    if (n < 2) return 0.0;

    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const Point& p0 = contour[i];
        const Point& p1 = contour[(i + 1) % n];
        sum += std::hypot(double(p1.x - p0.x), double(p1.y - p0.y));
    }
    return sum;
}

double ContourAnalysis::area(const std::vector<Point>& contour) {
    const size_t n = contour.size();
    if (n < 3) return 0.0;

    // Shoelace (Gauss's area) formula: A = 0.5 * |sum(x_i * y_{i+1} - x_{i+1} * y_i)|
    double crossSum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const Point& p0 = contour[i];
        const Point& p1 = contour[(i + 1) % n];
        crossSum += double(p0.x) * double(p1.y) - double(p1.x) * double(p0.y);
    }
    return 0.5 * std::fabs(crossSum);
}
