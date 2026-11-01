#include "EllipseDetector.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include "../common/Draw.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

// Wrap angle difference in [0, PI)
double angleDiffPi(double a1, double a2) {
    double d = std::fmod(std::fabs(a1 - a2), kPi);
    return std::min(d, kPi - d);
}

// A candidate is only accepted if enough of its outline is really supported by edge pixels. This
// removes the false ellipses that the voting step produces from a few partial arcs / unrelated edges.
constexpr double kMinOutlineSupport = 0.85;  // fraction of outline samples that must lie near an edge pixel
constexpr int kSupportTolerance = 3;         // px; also covers the two sides of a thick outline

double outlineSupport(const GrayImage& edges, const Ellipse& e) {
    const double perim = kPi * (1.5 * (e.a + e.b) - std::sqrt(e.a * e.b));
    const int n = std::max(24, int(perim));
    const double ca = std::cos(e.angle), sa = std::sin(e.angle);
    int hits = 0;
    for (int i = 0; i < n; ++i) {
        const double t = 2.0 * kPi * i / n;
        const double ex = e.a * std::cos(t), ey = e.b * std::sin(t);
        const int px = int(std::lround(e.cx + ex * ca - ey * sa));
        const int py = int(std::lround(e.cy + ex * sa + ey * ca));
        bool near = false;
        for (int dy = -kSupportTolerance; dy <= kSupportTolerance && !near; ++dy)
            for (int dx = -kSupportTolerance; dx <= kSupportTolerance; ++dx) {
                const int x = px + dx, y = py + dy;
                if (x >= 0 && y >= 0 && x < edges.width && y < edges.height && edges.at(x, y) > 0.f) { near = true; break; }
            }
        if (near) ++hits;
    }
    return double(hits) / n;
}

}  // namespace

// -----------------------------------------------------------------------------
// Efficient 1D Hough Transform for Ellipse Detection
// Based on: Yonghong Xie & Qiang Ji (2002), "A New Efficient Ellipse Detection Method"
//
// For a pair of edge points (x1, y1) and (x2, y2) assumed to be the two vertices
// of the major axis:
//   x0 = (x1 + x2)/2,  y0 = (y1 + y2)/2          (Eq. 1-2: centre)
//   a  = sqrt((x2 - x1)^2 + (y2 - y1)^2) / 2     (Eq. 3: semi-major axis)
//   alpha = atan((y2 - y1) / (x2 - x1))          (Eq. 4: orientation)
// For every third edge point (x, y) at distance d from (x0, y0) with minAxis <= d <= a:
//   cos(tau) = (a^2 + d^2 - f^2) / (2 * a * d)   (Eq. 6)
//   b = sqrt( (a^2 * d^2 * sin^2(tau)) / (a^2 - d^2 * cos^2(tau)) )  (Eq. 5)
// Votes are accumulated in a 1D array over b.
// -----------------------------------------------------------------------------
std::vector<Ellipse> EllipseDetector::detect(const GrayImage& edges, const EllipseParams& params) {
    if (edges.empty()) return {};

    const int W = edges.width;
    const int H = edges.height;
    const int minA = std::max(5, params.minAxis);
    const int maxA = std::min(params.maxAxis, int(std::hypot(W, H) / 2));
    if (minA > maxA) return {};

    std::vector<Point> pts;
    pts.reserve(size_t(W * H / 8));
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            if (edges.at(x, y) > 0.f) pts.push_back({x, y});

    const int N = int(pts.size());
    if (N < params.minVotes) return {};

    // To keep pair search interactive on dense edge maps, subsample the outer pair indices
    // while keeping a dense third-point voting set.
    const int maxPairPts = 320;
    const int pairStep = std::max(1, N / maxPairPts);
    const int maxThirdPts = 1200;
    const int thirdStep = std::max(1, N / maxThirdPts);

    std::vector<Ellipse> candidates;
    std::vector<int> acc(size_t(maxA + 2), 0);

    const double minASq = double(minA) * minA;
    const double maxASq = double(maxA) * maxA;

    for (int i = 0; i < N; i += pairStep) {
        const double x1 = pts[i].x;
        const double y1 = pts[i].y;

        for (int j = i + pairStep; j < N; j += pairStep) {
            const double x2 = pts[j].x;
            const double y2 = pts[j].y;

            const double dx12 = x2 - x1;
            const double dy12 = y2 - y1;
            const double distSq = dx12 * dx12 + dy12 * dy12;
            const double aSq = 0.25 * distSq;
            if (aSq < minASq || aSq > maxASq) continue;

            const double a = std::sqrt(aSq);
            const double x0 = 0.5 * (x1 + x2);
            const double y0 = 0.5 * (y1 + y2);

            // Centre must lie inside image bounds
            if (x0 < 0 || x0 >= W || y0 < 0 || y0 >= H) continue;

            const int maxB = int(std::floor(a));
            if (maxB < minA) continue;
            std::fill(acc.begin(), acc.begin() + maxB + 2, 0);

            for (int k = 0; k < N; k += thirdStep) {
                if (k == i || k == j) continue;
                const double x = pts[k].x;
                const double y = pts[k].y;

                const double dx0 = x - x0;
                const double dy0 = y - y0;
                const double dSq = dx0 * dx0 + dy0 * dy0;
                if (dSq < minASq || dSq > aSq) continue;

                const double d = std::sqrt(dSq);
                if (d < 1e-6) continue;

                // f^2: squared distance from third point (x, y) to one major-axis endpoint
                // Choose the closer endpoint as recommended in Xie & Ji (2002)
                const double f1Sq = (x - x1) * (x - x1) + (y - y1) * (y - y1);
                const double f2Sq = (x - x2) * (x - x2) + (y - y2) * (y - y2);
                const double fSq = std::min(f1Sq, f2Sq);

                const double cosTau = (aSq + dSq - fSq) / (2.0 * a * d);
                const double cosTauSq = std::min(1.0, cosTau * cosTau);
                const double sinTauSq = std::max(0.0, 1.0 - cosTauSq);

                const double denom = aSq - dSq * cosTauSq;
                if (denom <= 1e-6) continue;

                const double bSq = (aSq * dSq * sinTauSq) / denom;
                if (bSq <= 0.0) continue;

                const int b = int(std::lround(std::sqrt(bSq)));
                if (b >= minA && b <= maxB) {
                    ++acc[b];
                }
            }

            // Smooth 1D accumulator across 3 adjacent bins (b-1, b, b+1) to handle pixel quantization
            int bestB = -1;
            int bestVotes = 0;
            for (int b = minA; b <= maxB; ++b) {
                int v = acc[b] + (b > minA ? acc[b - 1] / 2 : 0) + (b < maxB ? acc[b + 1] / 2 : 0);
                if (v > bestVotes) {
                    bestVotes = v;
                    bestB = b;
                }
            }

            // Scale votes back by thirdStep and check threshold & geometric coverage
            const int scaledVotes = bestVotes * thirdStep;
            // Approximate perimeter (Ramanujan I): pi * (3(a+b) - sqrt((3a+b)(a+3b)))
            const double approxPerim = kPi * (1.5 * (a + bestB) - std::sqrt(a * bestB));
            const int minReqVotes = std::max(params.minVotes, int(std::ceil(0.32 * approxPerim)));

            if (bestB >= minA && scaledVotes >= minReqVotes) {
                double alpha = std::atan2(dy12, dx12);
                if (alpha < 0.0) alpha += kPi;
                candidates.push_back({x0, y0, a, double(bestB), alpha, scaledVotes});
            }
        }
    }

    // Sort candidates by normalized vote density (votes / perimeter)
    std::sort(candidates.begin(), candidates.end(), [](const Ellipse& e1, const Ellipse& e2) {
        double p1 = std::max(1.0, e1.a + e1.b);
        double p2 = std::max(1.0, e2.a + e2.b);
        double s1 = e1.votes / p1;
        double s2 = e2.votes / p2;
        if (std::fabs(s1 - s2) > 1e-6) return s1 > s2;
        return e1.votes > e2.votes;
    });

    // Non-Maximum Suppression in 5D ellipse parameter space
    std::vector<Ellipse> kept;
    for (const Ellipse& c : candidates) {
        bool duplicate = false;
        for (const Ellipse& k : kept) {
            double dc = std::hypot(c.cx - k.cx, c.cy - k.cy);
            double da = std::fabs(c.a - k.a);
            double db = std::fabs(c.b - k.b);
            double dang = angleDiffPi(c.angle, k.angle);

            double minSep = std::max(12.0, 0.3 * std::min(c.b, k.b));
            bool sameCenterAndAxes = (dc < minSep && da < 0.25 * k.a + 8.0 && db < 0.25 * k.b + 8.0);
            // For nearly circular ellipses (a ~ b), orientation angle is arbitrary
            bool almostCircle = (c.a - c.b < 0.15 * c.a) && (k.a - k.b < 0.15 * k.a);
            if (sameCenterAndAxes && (almostCircle || dang < 0.35)) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate && outlineSupport(edges, c) >= kMinOutlineSupport) {
            kept.push_back(c);
            if (int(kept.size()) >= params.maxEllipses) break;
        }
    }

    return kept;
}

RgbImage EllipseDetector::drawEllipses(const RgbImage& original, const std::vector<Ellipse>& ellipses) {
    RgbImage out = original;
    for (const Ellipse& e : ellipses) {
        Draw::ellipse(out, e.cx, e.cy, e.a, e.b, e.angle, {0, 128, 255}, 2);
        Draw::dot(out, int(std::lround(e.cx)), int(std::lround(e.cy)), 2, {255, 0, 0});
    }
    return out;
}
