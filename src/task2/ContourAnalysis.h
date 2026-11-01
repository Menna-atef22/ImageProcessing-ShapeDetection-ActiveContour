#pragma once
#include <string>
#include <vector>

#include "../common/Types.h"

// ---------------------------------------------------------------
// Chain code, perimeter, area
//
// The snake contour (a closed polygon of points) is converted to an 8-direction Freeman chain code
// by tracing a Bresenham path between consecutive points. Perimeter and area are reported twice:
//   * from the polygon     : Euclidean edge lengths / shoelace formula
//   * from the chain code  : even moves = 1 px, odd (diagonal) moves = sqrt(2) px, and the area
//                            enclosed by the pixel path (Green's theorem over all moves)
// ---------------------------------------------------------------
struct ContourMeasurements {
    std::vector<int> chainCode;  // 8-direction Freeman code (0..7)
    Point start;                 // pixel where the chain code starts (decodes back to the contour)
    double perimeter = 0.0;      // polygon perimeter (px)
    double area = 0.0;           // polygon area (px^2), shoelace formula
    double chainPerimeter = 0.0; // perimeter measured on the chain code (px)
    double chainArea = 0.0;      // area enclosed by the chain-code path (px^2)
    int numPoints = 0;
};

class ContourAnalysis {
public:
    static ContourMeasurements analyze(const std::vector<Point>& closedContour);
    static std::vector<int> chainCode(const std::vector<Point>& contour);
    static double perimeter(const std::vector<Point>& contour);
    static double area(const std::vector<Point>& contour);  // Shoelace formula

    // Measurements computed from the chain code only (start point is not needed for these).
    static double chainPerimeter(const std::vector<int>& code);  // even = 1, odd = sqrt(2)
    static double chainArea(const std::vector<int>& code);       // area enclosed by the pixel path
    static std::string toString(const std::vector<int>& code);   // "0012776..."
    static std::vector<Point> decode(const Point& start, const std::vector<int>& code);  // pixel path
};
