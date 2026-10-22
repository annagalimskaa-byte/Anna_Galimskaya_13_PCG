#pragma once
#include <vector>
#include <cstdint>

struct Point {
    int x;
    int y;
};

class Rasterizer {
public:
    static std::vector<Point> stepAlgorithm(int x1, int y1, int x2, int y2);
    static std::vector<Point> ddaAlgorithm(int x1, int y1, int x2, int y2);
    static std::vector<Point> bresenhamLine(int x1, int y1, int x2, int y2);
    static std::vector<Point> bresenhamCircle(int cx, int cy, int radius);
};