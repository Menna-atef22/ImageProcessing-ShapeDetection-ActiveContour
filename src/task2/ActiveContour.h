#pragma once
#include <vector>

#include "../common/Image.h"
#include "../common/Types.h"

// ---------------------------------------------------------------
// Active Contour Model (Snake) - greedy algorithm
//
// Every contour point is moved, one at a time, to the position inside a
// small search window that minimises
//
//     E = alpha * E_continuity + beta * E_curvature + gamma * E_external + balloon * E_balloon
//
//   E_continuity : |meanSpacing - distance(prev, candidate)|  (keeps points evenly spaced)
//   E_curvature  : |prev - 2*candidate + next|^2               (penalises sharp bends)
//   E_external   : -normalised gradient magnitude              (attracts points to edges)
//   E_balloon    : distance to the contour centre              (constant inward / outward pressure;
//                  lets the snake travel through flat regions where there is no edge to follow)
//
// The three terms are normalised inside each search window so that the
// weights alpha / beta / gamma are directly comparable.
// ---------------------------------------------------------------
struct SnakeParams {
    double alpha = 1.0;   // continuity weight
    double beta = 1.0;    // curvature weight
    double gamma = 1.2;   // external (image) energy weight
    int iterations = 200;
    int window = 3;       // neighbourhood search size (3 => 3x3, 5 => 5x5, ...)
    double balloon = 0.0; // balloon force: > 0 pulls every point towards the contour centre (shrinks),
                          // < 0 pushes outwards (inflates), 0 = disabled
    double sigma = 2.0;   // Gaussian smoothing applied before computing the edge map
                          // (larger sigma = wider capture range, less precise edges)
};

class ActiveContour {
public:
    // initial: ordered points of a closed contour placed around the object.
    // Returns the final contour; 'history' (optional) receives the initial contour followed by one
    // snapshot per iteration. The loop stops early once no point moves any more.
    static std::vector<Point> evolve(const GrayImage& gray, const std::vector<Point>& initial,
                                     const SnakeParams& params = {},
                                     std::vector<std::vector<Point>>* history = nullptr);
    static RgbImage drawContour(const RgbImage& original, const std::vector<Point>& contour);
};
