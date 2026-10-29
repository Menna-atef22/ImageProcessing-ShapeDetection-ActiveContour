#pragma once
// Shared types between all team members. Agree on these BEFORE coding.

struct Point {
    int x = 0;
    int y = 0;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};

// Line in Hough (polar) form:  x*cos(theta) + y*sin(theta) = rho
struct Line {
    double rho = 0.0;    // distance from origin (pixels)
    double theta = 0.0;  // angle in radians [0, PI)
    int votes = 0;       // accumulator votes
};

struct Circle {
    int cx = 0, cy = 0;  // center
    int r = 0;           // radius
    int votes = 0;
};

struct Ellipse {
    double cx = 0.0, cy = 0.0;  // center
    double a = 0.0, b = 0.0;    // semi-major / semi-minor axes
    double angle = 0.0;         // orientation in radians
    int votes = 0;
};
