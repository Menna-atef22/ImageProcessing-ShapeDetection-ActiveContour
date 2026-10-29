#pragma once
#include <vector>

#include "../common/Draw.h"
#include "../common/Image.h"
#include "../common/Types.h"

// ---------------------------------------------------------------
// Hough Line Transform (implemented FROM SCRATCH)
// Input: binary edge map (output of EdgeDetector::canny -> edges)
// ---------------------------------------------------------------

struct HoughLineParams {
    double rhoStep = 1.0;       // accumulator resolution for rho (pixels)
    double thetaStepDeg = 1.0;  // accumulator resolution for theta (degrees)
    double voteRatio = 0.5;     // a peak needs >= voteRatio * (max votes)
    int minVotes = 30;          // ... and at least this many votes
    int nmsRho = 8;             // peak suppression window (accumulator cells)
    int nmsTheta = 8;
    int maxLines = 20;          // keep only the strongest N lines
};

struct HoughLineResult {
    std::vector<Line> lines;  // sorted by votes (strongest first)
    GrayImage accumulator;    // Hough space, normalised 0..255 (width = theta, height = rho)
};

class LineDetector {
public:
    static HoughLineResult detect(const GrayImage& edges, const HoughLineParams& params = {});

    // Draws the (full, image-clipped) lines on a COPY of the original image.
    static RgbImage drawLines(const RgbImage& original, const std::vector<Line>& lines,
                              Color color = {0, 255, 0}, int thickness = 2);

    // Clips an infinite polar line to the image rectangle. Returns false if it misses the image.
    static bool clipLine(const Line& l, int width, int height, Point& p0, Point& p1);
};
