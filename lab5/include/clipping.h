#pragma once

#include <optional>

#include "geometry.h"

namespace clip {

enum OutCode : unsigned {
    kInside = 0,
    kLeft = 1u << 0,
    kRight = 1u << 1,
    kBottom = 1u << 2,
    kTop = 1u << 3,
};

unsigned computeOutCode(geom::Point p, const geom::Rect& r);

struct ClipResult {
    std::optional<geom::Segment> visible;
    int steps = 0;
};

ClipResult midpointClip(const geom::Segment& s, const geom::Rect& window, double eps = 0.0);

ClipResult cyrusBeckClip(const geom::Segment& s, const geom::Polygon& window);

}
