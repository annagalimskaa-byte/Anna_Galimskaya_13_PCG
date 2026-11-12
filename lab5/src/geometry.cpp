#include "geometry.h"

namespace geom {

double signedArea(const Polygon& poly) {
    double s = 0.0;
    const std::size_t n = poly.size();
    for (std::size_t i = 0; i < n; ++i) {
        s += cross(poly[i], poly[(i + 1) % n]);
    }
    return s / 2.0;
}

bool isConvex(const Polygon& poly) {
    const std::size_t n = poly.size();
    if (n < 3) return false;

    int sign = 0;
    double angleSum = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const Point a = poly[i];
        const Point b = poly[(i + 1) % n];
        const Point c = poly[(i + 2) % n];
        const Point e1 = b - a;
        const Point e2 = c - b;
        if (length(e1) < kEps) return false;
        const double cr = cross(e1, e2);
        if (std::fabs(cr) > kEps) {
            const int s = cr > 0 ? 1 : -1;
            if (sign == 0) sign = s;
            else if (s != sign) return false;
        }
        angleSum += std::atan2(cr, dot(e1, e2));
    }
    if (sign == 0) return false;
    return std::fabs(std::fabs(angleSum) - 2.0 * kPi) < 1e-6;
}

Polygon rectToPolygon(const Rect& r) {
    return {{r.xmin, r.ymin}, {r.xmax, r.ymin}, {r.xmax, r.ymax}, {r.xmin, r.ymax}};
}

}
