#include "ContourAnalysis.h"

// TODO: implement from scratch.
ContourMeasurements ContourAnalysis::analyze(const std::vector<Point>& c) {
    ContourMeasurements m;
    m.numPoints = int(c.size());
    m.chainCode = chainCode(c);
    m.perimeter = perimeter(c);
    m.area = area(c);
    return m;
}
std::vector<int> ContourAnalysis::chainCode(const std::vector<Point>&) { return {}; }
double ContourAnalysis::perimeter(const std::vector<Point>&) { return 0.0; }
double ContourAnalysis::area(const std::vector<Point>&) { return 0.0; }
