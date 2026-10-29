#include "EllipseDetector.h"

#include "../common/Draw.h"

// TODO: implement ellipse detection from scratch.
std::vector<Ellipse> EllipseDetector::detect(const GrayImage&, const EllipseParams&) { return {}; }

RgbImage EllipseDetector::drawEllipses(const RgbImage& original, const std::vector<Ellipse>& ellipses) {
    RgbImage out = original;
    for (const Ellipse& e : ellipses) {
        Draw::ellipse(out, e.cx, e.cy, e.a, e.b, e.angle, {0, 128, 255}, 2);
        Draw::dot(out, int(e.cx), int(e.cy), 2, {255, 0, 0});
    }
    return out;
}
