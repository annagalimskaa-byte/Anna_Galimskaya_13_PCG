#include "clipping.h"

#include <algorithm>
#include <cmath>

namespace clip {

using geom::Point;
using geom::Polygon;
using geom::Rect;
using geom::Segment;

unsigned computeOutCode(Point p, const Rect& r) {
    unsigned code = kInside;
    if (p.x < r.xmin) code |= kLeft;
    else if (p.x > r.xmax) code |= kRight;
    if (p.y < r.ymin) code |= kBottom;
    else if (p.y > r.ymax) code |= kTop;
    return code;
}

namespace {

struct MidpointState {
    const Rect& window;
    double eps;
    int steps = 0;
    bool found = false;
    Point first{};
    Point last{};

    void addVisible(Point a, Point b) {
        if (!found) {
            first = a;
            found = true;
        }
        last = b;
    }
};

void midpointRecursive(Point a, Point b, unsigned codeA, unsigned codeB,
                       int depth, MidpointState& st) {
    ++st.steps;

    if ((codeA | codeB) == 0) {
        st.addVisible(a, b);
        return;
    }
    if ((codeA & codeB) != 0) {
        return;
    }
    const Point m = geom::midpoint(a, b);
    if (geom::length(b - a) < st.eps || depth > 64) {
        if (codeA == 0) st.addVisible(a, a);
        else if (codeB == 0) st.addVisible(b, b);
        else if (st.window.contains(m, st.eps)) st.addVisible(m, m);
        return;
    }
    const unsigned codeM = computeOutCode(m, st.window);
    midpointRecursive(a, m, codeA, codeM, depth + 1, st);
    midpointRecursive(m, b, codeM, codeB, depth + 1, st);
}

}

ClipResult midpointClip(const Segment& s, const Rect& window, double eps) {
    if (eps <= 0.0) {
        const double size = std::max({window.xmax - window.xmin,
                                      window.ymax - window.ymin, 1.0});
        eps = size * 1e-7;
    }
    MidpointState st{window, eps};
    midpointRecursive(s.a, s.b, computeOutCode(s.a, window),
                      computeOutCode(s.b, window), 0, st);

    ClipResult res;
    res.steps = st.steps;
    if (st.found) res.visible = Segment{st.first, st.last};
    return res;
}

ClipResult cyrusBeckClip(const Segment& s, const Polygon& window) {
    ClipResult res;
    const std::size_t n = window.size();
    if (n < 3) return res;

    const double orient = geom::signedArea(window) > 0 ? 1.0 : -1.0;

    const Point d = s.b - s.a;
    double tLower = 0.0;
    double tUpper = 1.0;

    for (std::size_t i = 0; i < n; ++i) {
        ++res.steps;
        const Point vi = window[i];
        const Point edge = window[(i + 1) % n] - vi;
        const Point normal{-edge.y * orient, edge.x * orient};

        const Point w = s.a - vi;
        const double num = geom::dot(normal, w);
        const double den = geom::dot(normal, d);

        if (std::fabs(den) < geom::kEps) {
            if (num < -geom::kEps) return res;
            continue;
        }

        const double t = -num / den;
        if (den > 0) {
            tLower = std::max(tLower, t);
        } else {
            tUpper = std::min(tUpper, t);
        }
        if (tLower > tUpper + geom::kEps) return res;
    }

    res.visible = Segment{s.a + d * tLower, s.a + d * tUpper};
    return res;
}

}
