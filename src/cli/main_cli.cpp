// Command-line runner for Assignment 2 (no GUI needed). Runs every task on one image and writes the
// superimposed results + a text report, e.g. to produce the figures for the report in batch.
//
//   ipcli <image> <outDir> [options]
//     --canny  sigma low high              (default 1.4 0.05 0.15)
//     --lines                              Hough lines
//     --lineparams ratio minVotes maxLines (default 0.5 30 20; lower ratio finds weaker lines)
//     --circles minR maxR minVotes         Hough circles
//     --ellipses minA maxA minVotes        ellipses (Xie & Ji)
//     --snake cx cy r [options]            greedy active contour starting from a circle
//         --points n --alpha a --beta b --gamma g --balloon k --iter n --window w --sigma s
//   With no task flag, only Canny is run.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "../common/Draw.h"
#include "../common/Image.h"
#include "../task1/CircleDetector.h"
#include "../task1/EdgeDetector.h"
#include "../task1/EllipseDetector.h"
#include "../task1/LineDetector.h"
#include "../task2/ActiveContour.h"
#include "../task2/ContourAnalysis.h"

namespace {
constexpr double kPi = 3.14159265358979323846;

std::string baseName(const std::string& path) {
    size_t s = path.find_last_of("/\\");
    std::string f = s == std::string::npos ? path : path.substr(s + 1);
    size_t d = f.find_last_of('.');
    return d == std::string::npos ? f : f.substr(0, d);
}

void drawPoly(RgbImage& img, const std::vector<Point>& pts, Color c) {
    for (size_t i = 0; i < pts.size(); ++i) {
        const Point& a = pts[i];
        const Point& b = pts[(i + 1) % pts.size()];
        Draw::line(img, a.x, a.y, b.x, b.y, c, 2);
        Draw::dot(img, a.x, a.y, 2, c);
    }
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s <image> <outDir> [--canny s lo hi] [--lines | --lineparams ratio minVotes maxLines] [--circles minR maxR votes]\n"
                             "          [--ellipses minA maxA votes] [--snake cx cy r [--points n --alpha a --beta b --gamma g\n"
                             "          --balloon k --iter n --window w --sigma s]]\n", argv[0]);
        return 1;
    }
    const std::string in = argv[1], outDir = argv[2];
    RgbImage rgb;
    if (!ImageIO::loadRgb(in, rgb)) { std::fprintf(stderr, "cannot read %s\n", in.c_str()); return 1; }
    const GrayImage gray = toGray(rgb);
    const std::string base = outDir + "/" + baseName(in);

    CannyParams cp;
    bool doLines = false, doCircles = false, doEllipses = false, doSnake = false;
    HoughLineParams hl;
    HoughCircleParams hc;
    EllipseParams he;
    SnakeParams sp;
    int snakeCx = 0, snakeCy = 0, snakeR = 0, snakePoints = 60;

    auto num = [&](int& i) { return std::atof(argv[++i]); };
    for (int i = 3; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--canny" && i + 3 < argc) { cp.sigma = num(i); cp.lowRatio = num(i); cp.highRatio = num(i); }
        else if (a == "--lines") doLines = true;
        else if (a == "--lineparams" && i + 3 < argc) { doLines = true; hl.voteRatio = num(i); hl.minVotes = int(num(i)); hl.maxLines = int(num(i)); }
        else if (a == "--circles" && i + 3 < argc) { doCircles = true; hc.minRadius = int(num(i)); hc.maxRadius = int(num(i)); hc.minVotes = int(num(i)); }
        else if (a == "--ellipses" && i + 3 < argc) { doEllipses = true; he.minAxis = int(num(i)); he.maxAxis = int(num(i)); he.minVotes = int(num(i)); }
        else if (a == "--snake" && i + 3 < argc) { doSnake = true; snakeCx = int(num(i)); snakeCy = int(num(i)); snakeR = int(num(i)); }
        else if (a == "--points" && i + 1 < argc) snakePoints = int(num(i));
        else if (a == "--alpha" && i + 1 < argc) sp.alpha = num(i);
        else if (a == "--beta" && i + 1 < argc) sp.beta = num(i);
        else if (a == "--gamma" && i + 1 < argc) sp.gamma = num(i);
        else if (a == "--balloon" && i + 1 < argc) sp.balloon = num(i);
        else if (a == "--iter" && i + 1 < argc) sp.iterations = int(num(i));
        else if (a == "--window" && i + 1 < argc) sp.window = int(num(i));
        else if (a == "--sigma" && i + 1 < argc) sp.sigma = num(i);
        else { std::fprintf(stderr, "unknown or incomplete option: %s\n", a.c_str()); return 1; }
    }

    std::ostringstream rep;
    rep << "Image: " << in << " (" << rgb.width << "x" << rgb.height << ")\n";

    // ---- Task 1: Canny ----
    EdgeResult edges = EdgeDetector::canny(gray, cp);
    long long edgePx = std::count_if(edges.edges.data.begin(), edges.edges.data.end(), [](float v) { return v > 0.f; });
    ImageIO::saveRgb(base + "_canny_edges.png", EdgeDetector::edgesToRgb(edges));
    ImageIO::saveRgb(base + "_canny_overlay.png", EdgeDetector::overlayEdges(rgb, edges.edges));
    rep << "\n[Canny] sigma=" << cp.sigma << " low=" << cp.lowRatio << " high=" << cp.highRatio << " -> " << edgePx << " edge pixels\n";

    if (doLines) {
        HoughLineResult r = LineDetector::detect(edges.edges, hl);
        ImageIO::saveRgb(base + "_lines.png", LineDetector::drawLines(rgb, r.lines));
        rep << "\n[Hough lines] " << r.lines.size() << " lines\n";
        int k = 1;
        for (const Line& l : r.lines)
            rep << "  #" << k++ << " rho=" << std::fixed << std::setprecision(1) << l.rho << " theta=" << l.theta * 180.0 / kPi
                << " deg votes=" << l.votes << "\n";
    }
    if (doCircles) {
        std::vector<Circle> c = CircleDetector::detect(edges.edges, hc);
        ImageIO::saveRgb(base + "_circles.png", CircleDetector::drawCircles(rgb, c));
        rep << "\n[Hough circles] " << c.size() << " circles (r " << hc.minRadius << ".." << hc.maxRadius << ")\n";
        int k = 1;
        for (const Circle& x : c) rep << "  #" << k++ << " centre=(" << x.cx << "," << x.cy << ") r=" << x.r << " votes=" << x.votes << "\n";
    }
    if (doEllipses) {
        std::vector<Ellipse> e = EllipseDetector::detect(edges.edges, he);
        ImageIO::saveRgb(base + "_ellipses.png", EllipseDetector::drawEllipses(rgb, e));
        rep << "\n[Ellipses] " << e.size() << " ellipses (axis " << he.minAxis << ".." << he.maxAxis << ")\n";
        int k = 1;
        for (const Ellipse& x : e)
            rep << "  #" << k++ << " centre=(" << std::fixed << std::setprecision(1) << x.cx << "," << x.cy << ") a=" << x.a << " b=" << x.b
                << " angle=" << x.angle * 180.0 / kPi << " deg votes=" << x.votes << "\n";
    }

    // ---- Task 2: active contour + chain code ----
    if (doSnake) {
        std::vector<Point> init;
        for (int i = 0; i < snakePoints; ++i) {
            double a = 2.0 * kPi * i / snakePoints;
            init.push_back({int(std::lround(snakeCx + snakeR * std::cos(a))), int(std::lround(snakeCy + snakeR * std::sin(a)))});
        }
        std::vector<std::vector<Point>> history;
        std::vector<Point> fin = ActiveContour::evolve(gray, init, sp, &history);
        RgbImage out = rgb;
        drawPoly(out, init, {0, 220, 255});
        out = ActiveContour::drawContour(out, fin);
        ImageIO::saveRgb(base + "_snake.png", out);

        ContourMeasurements m = ContourAnalysis::analyze(fin);
        const std::string code = ContourAnalysis::toString(m.chainCode);
        rep << "\n[Active contour] start circle centre=(" << snakeCx << "," << snakeCy << ") r=" << snakeR << " points=" << snakePoints
            << "\n  alpha=" << sp.alpha << " beta=" << sp.beta << " gamma=" << sp.gamma << " balloon=" << sp.balloon << " window=" << sp.window
            << " sigma=" << sp.sigma << " iterations used=" << (history.empty() ? 0 : history.size() - 1) << " (max " << sp.iterations << ")\n"
            << "  contour points         : " << m.numPoints << "\n"
            << "  chain code start pixel : (" << m.start.x << "," << m.start.y << ")\n"
            << "  chain code length      : " << m.chainCode.size() << " moves\n"
            << std::fixed << std::setprecision(2)
            << "  perimeter (polygon)    : " << m.perimeter << " px\n"
            << "  perimeter (chain code) : " << m.chainPerimeter << " px\n"
            << "  area (polygon, shoelace): " << m.area << " px^2\n"
            << "  area (chain code)      : " << m.chainArea << " px^2\n";

        std::ofstream cf(base + "_chaincode.txt");
        cf << "Freeman 8-direction chain code (0=E 1=NE 2=N 3=NW 4=W 5=SW 6=S 7=SE; image y axis points down)\n"
           << "start pixel (x,y): " << m.start.x << " " << m.start.y << "\n"
           << "moves: " << m.chainCode.size() << "\n"
           << "perimeter_polygon: " << m.perimeter << "\nperimeter_chain: " << m.chainPerimeter << "\n"
           << "area_polygon: " << m.area << "\narea_chain: " << m.chainArea << "\n"
           << "chain code:\n" << code << "\n";
    }

    std::ofstream rf(base + "_report.txt");
    rf << rep.str();
    std::fputs(rep.str().c_str(), stdout);
    return 0;
}
