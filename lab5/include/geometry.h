#pragma once

#include <cmath>
#include <vector>

namespace geom {

constexpr double kEps = 1e-9;
constexpr double kPi = 3.14159265358979323846;

struct Point {
    double x = 0.0;
    double y = 0.0;
};

inline Point operator+(Point a, Point b) { return {a.x + b.x, a.y + b.y}; }
inline Point operator-(Point a, Point b) { return {a.x - b.x, a.y - b.y}; }
inline Point operator*(Point a, double k) { return {a.x * k, a.y * k}; }

inline double dot(Point a, Point b) { return a.x * b.x + a.y * b.y; }
inline double cross(Point a, Point b) { return a.x * b.y - a.y * b.x; }
inline double length(Point a) { return std::hypot(a.x, a.y); }
inline Point midpoint(Point a, Point b) { return {(a.x + b.x) / 2.0, (a.y + b.y) / 2.0}; }

struct Segment {
    Point a;
    Point b;
};

struct Rect {
    double xmin = 0.0;
    double ymin = 0.0;
    double xmax = 0.0;
    double ymax = 0.0;

    bool contains(Point p, double eps = kEps) const {
        return p.x >= xmin - eps && p.x <= xmax + eps &&
               p.y >= ymin - eps && p.y <= ymax + eps;
    }
};

using Polygon = std::vector<Point>;

double signedArea(const Polygon& poly);

bool isConvex(const Polygon& poly);

Polygon rectToPolygon(const Rect& r);

}
