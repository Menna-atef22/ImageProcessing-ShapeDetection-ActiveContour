#include "CircleDetector.h"

#include "../common/Draw.h"

// TODO: implement Hough Circle Transform from scratch.
std::vector<Circle> CircleDetector::detect(const GrayImage&, const HoughCircleParams&) { return {}; }

RgbImage CircleDetector::drawCircles(const RgbImage& original, const std::vector<Circle>& circles) {
    RgbImage out = original;
    for (const Circle& c : circles) {
        Draw::circle(out, c.cx, c.cy, c.r, {0, 255, 0}, 2);
        Draw::dot(out, c.cx, c.cy, 2, {255, 0, 0});  // center
    }
    return out;
}
