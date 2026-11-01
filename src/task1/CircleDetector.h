#pragma once
#include <vector>

#include "../common/Image.h"
#include "../common/Types.h"

// ---------------------------------------------------------------
// Circle detection (Hough Circle Transform)
// Input: binary edge map from EdgeDetector::canny(...).edges
// ---------------------------------------------------------------
struct HoughCircleParams {
    int minRadius = 10;
    int maxRadius = 100;
    int minVotes = 50;
    int maxCircles = 10;
};

class CircleDetector {
public:
    static std::vector<Circle> detect(const GrayImage& edges, const HoughCircleParams& params = {});
    static RgbImage drawCircles(const RgbImage& original, const std::vector<Circle>& circles);
};
