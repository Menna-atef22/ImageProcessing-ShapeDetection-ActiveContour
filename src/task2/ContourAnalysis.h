#pragma once
#include <string>
#include <vector>

#include "../common/Types.h"

// ---------------------------------------------------------------
// Chain code, perimeter, area  [TODO]
// ---------------------------------------------------------------
struct ContourMeasurements {
    std::vector<int> chainCode;  // 8-direction Freeman code (0..7)
    double perimeter = 0.0;
    double area = 0.0;
    int numPoints = 0;
};

class ContourAnalysis {
public:
    static ContourMeasurements analyze(const std::vector<Point>& closedContour);
    static std::vector<int> chainCode(const std::vector<Point>& contour);
    static double perimeter(const std::vector<Point>& contour);
    static double area(const std::vector<Point>& contour);  // Shoelace formula
};
