#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "clipping.h"
#include "geometry.h"

namespace io {

class ParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

enum class WindowKind { Rectangle, ConvexPolygon };

struct InputData {
    std::vector<geom::Segment> segments;
    WindowKind kind = WindowKind::Rectangle;
    geom::Rect rect;
    geom::Polygon polygon;
};

InputData parseInput(const std::string& text);

InputData loadInputFile(const std::string& path);

std::vector<clip::ClipResult> clipAll(const InputData& data);

const char* algorithmName(WindowKind kind);

}
