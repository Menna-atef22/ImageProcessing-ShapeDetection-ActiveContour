#pragma once
#include "../common/Image.h"

// ---------------------------------------------------------------
// Canny Edge Detector (implemented FROM SCRATCH)
// Pipeline: Gaussian blur -> Sobel gradients -> Non-Maximum Suppression
//           -> Double threshold -> Hysteresis (edge tracking)
// ---------------------------------------------------------------

struct CannyParams {
    double sigma = 1.4;      // Gaussian std-dev
    double lowRatio = 0.05;  // low  threshold = lowRatio  * max gradient magnitude
    double highRatio = 0.15; // high threshold = highRatio * max gradient magnitude
};

struct EdgeResult {
    GrayImage blurred;    // after Gaussian smoothing
    GrayImage magnitude;  // gradient magnitude
    GrayImage direction;  // gradient direction (radians)
    GrayImage suppressed; // after non-maximum suppression
    GrayImage edges;      // final binary edge map (0 or 255)
};

class EdgeDetector {
public:
    // Full pipeline
    static EdgeResult canny(const GrayImage& gray, const CannyParams& params = {});

    // Individual steps (public so they can be shown / tested separately)
    static GrayImage gaussianBlur(const GrayImage& src, double sigma);
    static void sobel(const GrayImage& src, GrayImage& magnitude, GrayImage& direction);
    static GrayImage nonMaxSuppression(const GrayImage& magnitude, const GrayImage& direction);
    static GrayImage hysteresis(const GrayImage& nms, float lowThr, float highThr);

    // Display helpers
    static RgbImage edgesToRgb(const EdgeResult& r);                       // white edges on black
    static RgbImage overlayEdges(const RgbImage& original, const GrayImage& edges);  // edges in red
};
