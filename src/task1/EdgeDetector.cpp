#include "EdgeDetector.h"

#include <algorithm>
#include <cmath>
#include <vector>

static const double PI = 3.14159265358979323846;

// ---------- 1) Gaussian blur (separable, border replicate) ----------
GrayImage EdgeDetector::gaussianBlur(const GrayImage& src, double sigma) {
    if (sigma <= 0.0 || src.empty()) return src;

    int radius = std::max(1, int(std::ceil(3.0 * sigma)));
    std::vector<float> k(2 * radius + 1);
    float sum = 0.f;
    for (int i = -radius; i <= radius; ++i) {
        k[i + radius] = float(std::exp(-(i * i) / (2.0 * sigma * sigma)));
        sum += k[i + radius];
    }
    for (float& v : k) v /= sum;

    GrayImage tmp(src.width, src.height), dst(src.width, src.height);
    // horizontal pass
    for (int y = 0; y < src.height; ++y)
        for (int x = 0; x < src.width; ++x) {
            float acc = 0.f;
            for (int i = -radius; i <= radius; ++i) acc += k[i + radius] * src.atClamped(x + i, y);
            tmp.at(x, y) = acc;
        }
    // vertical pass
    for (int y = 0; y < src.height; ++y)
        for (int x = 0; x < src.width; ++x) {
            float acc = 0.f;
            for (int i = -radius; i <= radius; ++i) acc += k[i + radius] * tmp.atClamped(x, y + i);
            dst.at(x, y) = acc;
        }
    return dst;
}

// ---------- 2) Sobel gradients ----------
void EdgeDetector::sobel(const GrayImage& src, GrayImage& magnitude, GrayImage& direction) {
    magnitude = GrayImage(src.width, src.height);
    direction = GrayImage(src.width, src.height);
    for (int y = 0; y < src.height; ++y)
        for (int x = 0; x < src.width; ++x) {
            float p00 = src.atClamped(x - 1, y - 1), p01 = src.atClamped(x, y - 1), p02 = src.atClamped(x + 1, y - 1);
            float p10 = src.atClamped(x - 1, y),                                    p12 = src.atClamped(x + 1, y);
            float p20 = src.atClamped(x - 1, y + 1), p21 = src.atClamped(x, y + 1), p22 = src.atClamped(x + 1, y + 1);
            float gx = (p02 + 2 * p12 + p22) - (p00 + 2 * p10 + p20);
            float gy = (p20 + 2 * p21 + p22) - (p00 + 2 * p01 + p02);
            magnitude.at(x, y) = std::sqrt(gx * gx + gy * gy);
            direction.at(x, y) = std::atan2(gy, gx);
        }
}

// ---------- 3) Non-maximum suppression ----------
GrayImage EdgeDetector::nonMaxSuppression(const GrayImage& mag, const GrayImage& dir) {
    GrayImage out(mag.width, mag.height, 0.f);
    for (int y = 1; y < mag.height - 1; ++y)
        for (int x = 1; x < mag.width - 1; ++x) {
            double deg = dir.at(x, y) * 180.0 / PI;
            if (deg < 0) deg += 180.0;  // direction of the line, [0,180)

            float a, b;  // the two neighbours along the gradient direction
            if (deg < 22.5 || deg >= 157.5) {  // horizontal gradient
                a = mag.at(x - 1, y);     b = mag.at(x + 1, y);
            } else if (deg < 67.5) {           // 45 deg
                a = mag.at(x - 1, y - 1); b = mag.at(x + 1, y + 1);
            } else if (deg < 112.5) {          // vertical gradient
                a = mag.at(x, y - 1);     b = mag.at(x, y + 1);
            } else {                           // 135 deg
                a = mag.at(x + 1, y - 1); b = mag.at(x - 1, y + 1);
            }
            float m = mag.at(x, y);
            if (m >= a && m >= b) out.at(x, y) = m;
        }
    return out;
}

// ---------- 4+5) Double threshold + hysteresis ----------
GrayImage EdgeDetector::hysteresis(const GrayImage& nms, float lowThr, float highThr) {
    const int W = nms.width, H = nms.height;
    GrayImage out(W, H, 0.f);
    std::vector<int> stack;

    // strong edges are the seeds
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            if (nms.at(x, y) > 0.f && nms.at(x, y) >= highThr) {
                out.at(x, y) = 255.f;
                stack.push_back(y * W + x);
            }

    // grow through weak pixels connected (8-neighbourhood) to strong ones
    while (!stack.empty()) {
        int idx = stack.back();
        stack.pop_back();
        int cx = idx % W, cy = idx / W;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = cx + dx, ny = cy + dy;
                if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
                if (out.at(nx, ny) == 0.f && nms.at(nx, ny) > 0.f && nms.at(nx, ny) >= lowThr) {
                    out.at(nx, ny) = 255.f;
                    stack.push_back(ny * W + nx);
                }
            }
    }
    return out;
}

// ---------- Full pipeline ----------
EdgeResult EdgeDetector::canny(const GrayImage& gray, const CannyParams& p) {
    EdgeResult r;
    r.blurred = gaussianBlur(gray, p.sigma);
    sobel(r.blurred, r.magnitude, r.direction);
    r.suppressed = nonMaxSuppression(r.magnitude, r.direction);

    float maxMag = 0.f;
    for (float v : r.magnitude.data) maxMag = std::max(maxMag, v);
    float high = float(p.highRatio) * maxMag;
    float low = float(p.lowRatio) * maxMag;
    if (low > high) std::swap(low, high);

    r.edges = hysteresis(r.suppressed, low, high);
    return r;
}

// ---------- Display helpers ----------
RgbImage EdgeDetector::edgesToRgb(const EdgeResult& r) { return grayToRgb(r.edges); }

RgbImage EdgeDetector::overlayEdges(const RgbImage& original, const GrayImage& edges) {
    RgbImage out = original;
    for (int y = 0; y < out.height; ++y)
        for (int x = 0; x < out.width; ++x)
            if (edges.at(x, y) > 0.f) {
                uint8_t* p = out.px(x, y);
                p[0] = 255; p[1] = 0; p[2] = 0;
            }
    return out;
}
