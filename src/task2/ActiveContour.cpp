#include "ActiveContour.h"

#include <algorithm>
#include <cmath>

#include "../common/Draw.h"
#include "../task1/EdgeDetector.h"

namespace {

// Corner relaxation: a point whose curvature is a local maximum, larger than kCornerCurvature and
// which lies on a reasonably strong edge is treated as a real corner of the object, so its
// curvature penalty (beta) is switched off for the next iteration.
constexpr double kCornerCurvature = 0.25;
constexpr float kCornerEdgeFraction = 0.10f;  // fraction of the maximum gradient magnitude

// Gradients weaker than this (on the 0..255 scale) are treated as noise when normalising, so a
// flat neighbourhood does not get its tiny differences amplified into a strong attraction.
constexpr float kMinGradientRange = 5.f;

inline double dist(const Point& a, const Point& b) {
    double dx = double(a.x) - b.x, dy = double(a.y) - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

inline int wrap(int i, int n) { return (i % n + n) % n; }

// Mean distance between consecutive contour points (closed contour).
double meanSpacing(const std::vector<Point>& c) {
    const int n = int(c.size());
    double sum = 0.0;
    for (int i = 0; i < n; ++i) sum += dist(c[i], c[(i + 1) % n]);
    return sum / n;
}

// Discrete curvature at point i: squared difference of the two unit tangent vectors that
// meet in the point. Ranges from 0 (straight) to 4 (full reversal).
double curvatureAt(const std::vector<Point>& c, int i) {
    const int n = int(c.size());
    const Point& p0 = c[wrap(i - 1, n)];
    const Point& p1 = c[i];
    const Point& p2 = c[wrap(i + 1, n)];
    double ux = double(p1.x) - p0.x, uy = double(p1.y) - p0.y;
    double vx = double(p2.x) - p1.x, vy = double(p2.y) - p1.y;
    double lu = std::sqrt(ux * ux + uy * uy), lv = std::sqrt(vx * vx + vy * vy);
    if (lu == 0.0 || lv == 0.0) return 0.0;
    ux /= lu; uy /= lu; vx /= lv; vy /= lv;
    double dx = ux - vx, dy = uy - vy;
    return dx * dx + dy * dy;
}

}  // namespace

std::vector<Point> ActiveContour::evolve(const GrayImage& gray, const std::vector<Point>& initial,
                                         const SnakeParams& params,
                                         std::vector<std::vector<Point>>* history) {
    std::vector<Point> snake = initial;
    if (history) history->clear();

    const int n = int(snake.size());
    if (n < 3 || gray.empty()) {
        if (history) history->push_back(snake);
        return snake;
    }

    // Keep every point inside the image.
    for (Point& p : snake) {
        p.x = std::min(std::max(p.x, 0), gray.width - 1);
        p.y = std::min(std::max(p.y, 0), gray.height - 1);
    }
    if (history) history->push_back(snake);

    // ---- External energy source: gradient magnitude of the smoothed image ----
    GrayImage smooth = EdgeDetector::gaussianBlur(gray, params.sigma);
    GrayImage mag, dir;
    EdgeDetector::sobel(smooth, mag, dir);
    float maxMag = 0.f;
    for (float v : mag.data) maxMag = std::max(maxMag, v);

    // Search window: odd size, at least 3x3.
    int win = std::max(3, params.window);
    if (win % 2 == 0) ++win;
    const int half = win / 2;
    const int cells = win * win;

    // Per-point curvature weight. Starts equal to beta and is set to 0 at detected corners.
    std::vector<double> beta(n, params.beta);

    // Scratch buffers for one search window.
    std::vector<double> eCont(cells), eCurv(cells), eBal(cells);
    std::vector<float> eExt(cells);
    std::vector<char> valid(cells);

    for (int it = 0; it < params.iterations; ++it) {
        const double avg = meanSpacing(snake);
        int moved = 0;

        // Centre of the contour, used by the balloon force.
        double ccx = 0.0, ccy = 0.0;
        for (const Point& p : snake) { ccx += p.x; ccy += p.y; }
        ccx /= n;
        ccy /= n;

        // Points are updated sequentially, so each one already sees the new position of its
        // predecessor (this is what makes the algorithm "greedy").
        for (int i = 0; i < n; ++i) {
            const Point prev = snake[wrap(i - 1, n)];
            const Point next = snake[wrap(i + 1, n)];
            const Point cur = snake[i];

            double maxCont = 0.0, maxCurv = 0.0;
            double minBal = 1e30, maxBal = -1e30;
            float minExt = 1e30f, maxExt = -1e30f;

            // Pass 1: raw energy terms for every candidate position in the window.
            for (int dy = -half; dy <= half; ++dy)
                for (int dx = -half; dx <= half; ++dx) {
                    const int k = (dy + half) * win + (dx + half);
                    const Point cand{cur.x + dx, cur.y + dy};
                    if (cand.x < 0 || cand.y < 0 || cand.x >= gray.width || cand.y >= gray.height) {
                        valid[k] = 0;
                        continue;
                    }
                    valid[k] = 1;
                    eCont[k] = std::fabs(avg - dist(prev, cand));
                    const double cx = double(prev.x) - 2.0 * cand.x + next.x;
                    const double cy = double(prev.y) - 2.0 * cand.y + next.y;
                    eCurv[k] = cx * cx + cy * cy;
                    eExt[k] = mag.at(cand.x, cand.y);
                    eBal[k] = std::hypot(cand.x - ccx, cand.y - ccy);  // distance to the contour centre
                    minBal = std::min(minBal, eBal[k]);
                    maxBal = std::max(maxBal, eBal[k]);
                    maxCont = std::max(maxCont, eCont[k]);
                    maxCurv = std::max(maxCurv, eCurv[k]);
                    minExt = std::min(minExt, eExt[k]);
                    maxExt = std::max(maxExt, eExt[k]);
                }

            // Normalise the external term to [0,1] inside the window; a nearly flat window
            // gets a floor on its range so that noise is not amplified.
            if (maxExt - minExt < kMinGradientRange) minExt = maxExt - kMinGradientRange;
            const double extRange = double(maxExt) - minExt;
            const double balRange = maxBal - minBal;

            // Pass 2: pick the candidate with the lowest total energy. The centre candidate is
            // evaluated first so ties keep the point where it is.
            auto energy = [&](int k) {
                const double c = maxCont > 0.0 ? eCont[k] / maxCont : 0.0;
                const double q = maxCurv > 0.0 ? eCurv[k] / maxCurv : 0.0;
                const double e = -(double(eExt[k]) - minExt) / extRange;
                // Balloon term in [0,1]: 0 for the candidate closest to the centre, 1 for the farthest.
                // A positive weight therefore pulls the point inwards, a negative one pushes it outwards.
                const double bal = balRange > 0.0 ? (eBal[k] - minBal) / balRange : 0.0;
                return params.alpha * c + beta[i] * q + params.gamma * e + params.balloon * bal;
            };
            const int centre = half * win + half;
            int best = centre;
            double bestE = energy(centre);
            for (int k = 0; k < cells; ++k) {
                if (!valid[k] || k == centre) continue;
                const double e = energy(k);
                if (e < bestE) { bestE = e; best = k; }
            }

            if (best != centre) {
                snake[i].x = cur.x + (best % win - half);
                snake[i].y = cur.y + (best / win - half);
                ++moved;
            }
        }

        // Corner relaxation for the next iteration.
        std::vector<double> curv(n);
        for (int i = 0; i < n; ++i) curv[i] = curvatureAt(snake, i);
        for (int i = 0; i < n; ++i) {
            const bool localMax = curv[i] > curv[wrap(i - 1, n)] && curv[i] > curv[wrap(i + 1, n)];
            const bool strongEdge = mag.at(snake[i].x, snake[i].y) > kCornerEdgeFraction * maxMag;
            beta[i] = (localMax && curv[i] > kCornerCurvature && strongEdge) ? 0.0 : params.beta;
        }

        if (history) history->push_back(snake);
        if (moved == 0) break;  // converged: no point wants to move any more
    }
    return snake;
}

RgbImage ActiveContour::drawContour(const RgbImage& original, const std::vector<Point>& c) {
    RgbImage out = original;
    for (size_t i = 0; i < c.size(); ++i) {
        const Point& a = c[i];
        const Point& b = c[(i + 1) % c.size()];
        Draw::line(out, a.x, a.y, b.x, b.y, {255, 255, 0}, 1);
        Draw::dot(out, a.x, a.y, 1, {255, 0, 0});
    }
    return out;
}
