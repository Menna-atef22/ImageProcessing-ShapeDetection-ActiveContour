#pragma once
#include <vector>

#include "../common/Image.h"
#include "../common/Types.h"

// ---------------------------------------------------------------
// Ellipse detection (Xie & Ji 1D Hough method + outline-support check)
// ---------------------------------------------------------------
struct EllipseParams {
    int minAxis = 10;
    int maxAxis = 200;
    int minVotes = 30;
    int maxEllipses = 10;
};

class EllipseDetector {
public:
    static std::vector<Ellipse> detect(const GrayImage& edges, const EllipseParams& params = {});
    static RgbImage drawEllipses(const RgbImage& original, const std::vector<Ellipse>& ellipses);
};
