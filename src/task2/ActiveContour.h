#pragma once
#include <vector>

#include "../common/Image.h"
#include "../common/Types.h"

// ---------------------------------------------------------------
// Active Contour Model (Snake) with Greedy algorithm  [TODO]
// ---------------------------------------------------------------
struct SnakeParams {
    double alpha = 1.0;   // continuity weight
    double beta = 1.0;    // curvature weight
    double gamma = 1.2;   // external (image) energy weight
    int iterations = 200;
    int window = 3;       // neighbourhood search size (3 => 3x3)
};

class ActiveContour {
public:
    // initial: user-selected points around the object.
    // Returns the final contour; 'history' (optional) gets one snapshot per iteration.
    static std::vector<Point> evolve(const GrayImage& gray, const std::vector<Point>& initial,
                                     const SnakeParams& params = {},
                                     std::vector<std::vector<Point>>* history = nullptr);
    static RgbImage drawContour(const RgbImage& original, const std::vector<Point>& contour);
};
