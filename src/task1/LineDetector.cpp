#include "LineDetector.h"

#include <algorithm>
#include <cmath>

static const double PI = 3.14159265358979323846;

HoughLineResult LineDetector::detect(const GrayImage& edges, const HoughLineParams& p) {
    HoughLineResult res;
    if (edges.empty()) return res;

    const int W = edges.width, H = edges.height;
    const double diag = std::sqrt(double(W) * W + double(H) * H);
    const int maxRho = int(std::ceil(diag / p.rhoStep));
    const int nRho = 2 * maxRho + 1;
    const int nTheta = std::max(1, int(std::lround(180.0 / p.thetaStepDeg)));

    // precompute trig tables
    std::vector<double> cosT(nTheta), sinT(nTheta);
    for (int t = 0; t < nTheta; ++t) {
        double th = t * PI / nTheta;
        cosT[t] = std::cos(th);
        sinT[t] = std::sin(th);
    }

    // ---- voting ----
    std::vector<int> acc(size_t(nTheta) * nRho, 0);  // acc[t * nRho + r]
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            if (edges.at(x, y) <= 0.f) continue;
            for (int t = 0; t < nTheta; ++t) {
                double rho = x * cosT[t] + y * sinT[t];
                int r = int(std::lround(rho / p.rhoStep)) + maxRho;
                ++acc[size_t(t) * nRho + r];
            }
        }

    int maxVotes = *std::max_element(acc.begin(), acc.end());

    // ---- accumulator image for display ----
    res.accumulator = GrayImage(nTheta, nRho);
    for (int t = 0; t < nTheta; ++t)
        for (int r = 0; r < nRho; ++r)
            res.accumulator.at(t, r) = maxVotes > 0 ? 255.f * acc[size_t(t) * nRho + r] / maxVotes : 0.f;

    if (maxVotes == 0) return res;

    // ---- peak detection with non-maximum suppression ----
    const int threshold = std::max(p.minVotes, int(std::ceil(p.voteRatio * maxVotes)));

    // Neighbour lookup that wraps theta (theta=PI is theta=0 with rho -> -rho)
    auto votesAt = [&](int t, int r) -> int {
        if (r < 0 || r >= nRho) return 0;
        if (t < 0) { t += nTheta; r = nRho - 1 - r; }
        else if (t >= nTheta) { t -= nTheta; r = nRho - 1 - r; }
        if (r < 0 || r >= nRho) return 0;
        return acc[size_t(t) * nRho + r];
    };

    std::vector<Line> found;
    for (int t = 0; t < nTheta; ++t)
        for (int r = 0; r < nRho; ++r) {
            int v = acc[size_t(t) * nRho + r];
            if (v < threshold) continue;

            bool isPeak = true;
            for (int dt = -p.nmsTheta; dt <= p.nmsTheta && isPeak; ++dt)
                for (int dr = -p.nmsRho; dr <= p.nmsRho; ++dr) {
                    if (dt == 0 && dr == 0) continue;
                    int nv = votesAt(t + dt, r + dr);
                    // strict > wins; ties are broken by scan order so plateaus give ONE peak
                    if (nv > v || (nv == v && (dt < 0 || (dt == 0 && dr < 0)))) {
                        isPeak = false;
                        break;
                    }
                }
            if (!isPeak) continue;

            Line l;
            l.theta = t * PI / nTheta;
            l.rho = (r - maxRho) * p.rhoStep;
            l.votes = v;
            found.push_back(l);
        }

    std::sort(found.begin(), found.end(), [](const Line& a, const Line& b) { return a.votes > b.votes; });
    if (int(found.size()) > p.maxLines) found.resize(p.maxLines);
    res.lines = std::move(found);
    return res;
}

// Liang-Barsky style clipping of  x*cos + y*sin = rho  against [0,W-1]x[0,H-1]
bool LineDetector::clipLine(const Line& l, int width, int height, Point& p0, Point& p1) {
    double c = std::cos(l.theta), s = std::sin(l.theta);
    double ox = l.rho * c, oy = l.rho * s;  // closest point to origin
    double dx = -s, dy = c;                 // line direction

    double tMin = -1e9, tMax = 1e9;
    auto clip = [&](double p, double q) {  // p*t <= q
        if (std::fabs(p) < 1e-12) return q >= 0;
        double t = q / p;
        if (p < 0) tMin = std::max(tMin, t);
        else tMax = std::min(tMax, t);
        return true;
    };
    double xmax = width - 1, ymax = height - 1;
    if (!clip(-dx, ox - 0.0) || !clip(dx, xmax - ox) || !clip(-dy, oy - 0.0) || !clip(dy, ymax - oy)) return false;
    if (tMin > tMax) return false;

    p0 = {int(std::lround(ox + tMin * dx)), int(std::lround(oy + tMin * dy))};
    p1 = {int(std::lround(ox + tMax * dx)), int(std::lround(oy + tMax * dy))};
    return true;
}

RgbImage LineDetector::drawLines(const RgbImage& original, const std::vector<Line>& lines, Color color,
                                 int thickness) {
    RgbImage out = original;  // never modify the original
    for (const Line& l : lines) {
        Point a, b;
        if (clipLine(l, out.width, out.height, a, b)) Draw::line(out, a.x, a.y, b.x, b.y, color, thickness);
    }
    return out;
}
