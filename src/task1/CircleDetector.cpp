#include "CircleDetector.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "../common/Draw.h"

namespace {

// Generate deduplicated 8-symmetric integer perimeter offsets for radius r
// using the midpoint circle algorithm so each perimeter pixel votes at most once.
std::vector<Point> circleOffsets(int r) {
    std::vector<Point> pts;
    pts.reserve(size_t(8 * r));
    int x = r, y = 0, err = 1 - r;
    auto addUnique = [&](int dx, int dy) {
        for (const Point& p : pts)
            if (p.x == dx && p.y == dy) return;
        pts.push_back({dx, dy});
    };
    while (x >= y) {
        addUnique( x,  y); addUnique(-x,  y);
        addUnique( x, -y); addUnique(-x, -y);
        addUnique( y,  x); addUnique(-y,  x);
        addUnique( y, -x); addUnique(-y, -x);
        ++y;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            --x;
            err += 2 * (y - x) + 1;
        }
    }
    return pts;
}

// A circle is only accepted if most of its outline is supported by edge pixels. This rejects the
// false circles that fit a short arc (e.g. the curved tip of an ellipse) with enough votes.
constexpr double kMinOutlineSupport = 0.80;  // fraction of outline samples that must lie near an edge pixel
constexpr int kSupportTolerance = 3;         // px; also covers both sides of a thick outline

double outlineSupport(const GrayImage& edges, const Circle& c) {
    const int n = std::max(24, int(2.0 * 3.14159265358979323846 * c.r));
    int hits = 0;
    for (int i = 0; i < n; ++i) {
        const double t = 2.0 * 3.14159265358979323846 * i / n;
        const int px = int(std::lround(c.cx + c.r * std::cos(t)));
        const int py = int(std::lround(c.cy + c.r * std::sin(t)));
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

std::vector<Circle> CircleDetector::detect(const GrayImage& edges, const HoughCircleParams& params) {
    if (edges.empty()) return {};

    const int W = edges.width;
    const int H = edges.height;
    const int rMin = std::max(3, params.minRadius);
    const int rMax = std::min(params.maxRadius, std::max(W, H));
    if (rMin > rMax) return {};

    // Collect all foreground edge pixels
    std::vector<Point> edgePts;
    edgePts.reserve(size_t(W * H / 8));
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            if (edges.at(x, y) > 0.f) edgePts.push_back({x, y});

    if (edgePts.empty()) return {};

    std::vector<Circle> candidates;
    std::vector<int> acc(size_t(W) * H, 0);

    // Vote in 2D (cx, cy) slice per radius r to keep memory small and cache-friendly
    for (int r = rMin; r <= rMax; ++r) {
        std::fill(acc.begin(), acc.end(), 0);
        const std::vector<Point> offsets = circleOffsets(r);
        const int perimeterSamples = int(offsets.size());
        if (perimeterSamples == 0) continue;

        for (const Point& ep : edgePts) {
            for (const Point& d : offsets) {
                int cx = ep.x - d.x;
                int cy = ep.y - d.y;
                if (cx >= 0 && cx < W && cy >= 0 && cy < H) {
                    ++acc[size_t(cy) * W + cx];
                }
            }
        }

        // Minimum vote requirement: both user threshold and a fraction of perimeter
        // so long straight edges cannot masquerade as huge-radius circles.
        const int minCoverageVotes = std::max(params.minVotes, int(std::ceil(0.35 * perimeterSamples)));

        // 2D local peak extraction (5x5 window)
        const int nmsXY = 4;
        for (int cy = 0; cy < H; ++cy) {
            for (int cx = 0; cx < W; ++cx) {
                int v = acc[size_t(cy) * W + cx];
                if (v < minCoverageVotes) continue;

                bool isPeak = true;
                for (int dy = -nmsXY; dy <= nmsXY && isPeak; ++dy) {
                    int ny = cy + dy;
                    if (ny < 0 || ny >= H) continue;
                    for (int dx = -nmsXY; dx <= nmsXY; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = cx + dx;
                        if (nx < 0 || nx >= W) continue;
                        int nv = acc[size_t(ny) * W + nx];
                        if (nv > v || (nv == v && (dy < 0 || (dy == 0 && dx < 0)))) {
                            isPeak = false;
                            break;
                        }
                    }
                }
                if (isPeak) {
                    candidates.push_back({cx, cy, r, v});
                }
            }
        }
    }

    // Sort by coverage ratio (votes / r) with raw votes tie-break so circles of all sizes compete fairly
    std::sort(candidates.begin(), candidates.end(), [](const Circle& a, const Circle& b) {
        double scoreA = double(a.votes) / std::max(1, a.r);
        double scoreB = double(b.votes) / std::max(1, b.r);
        if (std::fabs(scoreA - scoreB) > 1e-6) return scoreA > scoreB;
        return a.votes > b.votes;
    });

    // 3D Non-Maximum Suppression in (cx, cy, r) space
    std::vector<Circle> kept;
    for (const Circle& c : candidates) {
        bool duplicate = false;
        for (const Circle& k : kept) {
            double distCenters = std::hypot(double(c.cx - k.cx), double(c.cy - k.cy));
            int dr = std::abs(c.r - k.r);
            double minCenterSep = std::max(10.0, 0.35 * std::min(c.r, k.r));
            if (distCenters < minCenterSep && dr < std::max(8, int(0.25 * std::min(c.r, k.r)))) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate && outlineSupport(edges, c) >= kMinOutlineSupport) {
            kept.push_back(c);
            if (int(kept.size()) >= params.maxCircles) break;
        }
    }

    return kept;
}

RgbImage CircleDetector::drawCircles(const RgbImage& original, const std::vector<Circle>& circles) {
    RgbImage out = original;
    for (const Circle& c : circles) {
        Draw::circle(out, c.cx, c.cy, c.r, {0, 255, 0}, 2);
        Draw::dot(out, c.cx, c.cy, 2, {255, 0, 0});  // center
    }
    return out;
}
