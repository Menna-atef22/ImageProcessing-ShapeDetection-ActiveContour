#include "ActiveContour.h"

#include "../common/Draw.h"

// TODO: implement greedy snake from scratch.
std::vector<Point> ActiveContour::evolve(const GrayImage&, const std::vector<Point>& initial,
                                         const SnakeParams&, std::vector<std::vector<Point>>*) {
    return initial;
}

RgbImage ActiveContour::drawContour(const RgbImage& original, const std::vector<Point>& c) {
    RgbImage out = original;
    for (size_t i = 0; i < c.size(); ++i) {
        const Point& a = c[i];
        const Point& b = c[(i + 1) % c.size()];
        Draw::line(out, a.x, a.y, b.x, b.y, {255, 255, 0}, 1);
        Draw::dot(out, a.x, a.y, 1, {255, 0, 0});
    }
    return out;
}
