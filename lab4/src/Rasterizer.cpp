#include "Rasterizer.h"
#include <cmath>
#include <algorithm>

std::vector<Point> Rasterizer::stepAlgorithm(int x1, int y1, int x2, int y2) {
    std::vector<Point> points;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = std::max(std::abs(dx), std::abs(dy));

    if (steps == 0) {
        points.push_back({x1, y1});
        return points;
    }

    float xInc = static_cast<float>(dx) / steps;
    float yInc = static_cast<float>(dy) / steps;
    float x = static_cast<float>(x1);
    float y = static_cast<float>(y1);

    for (int i = 0; i <= steps; i++) {
        points.push_back({static_cast<int>(std::round(x)),
                          static_cast<int>(std::round(y))});
        x += xInc;
        y += yInc;
    }
    return points;
}

std::vector<Point> Rasterizer::ddaAlgorithm(int x1, int y1, int x2, int y2) {
    std::vector<Point> points;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int length = std::max(std::abs(dx), std::abs(dy));

    if (length == 0) {
        points.push_back({x1, y1});
        return points;
    }

    float dxInc = static_cast<float>(dx) / length;
    float dyInc = static_cast<float>(dy) / length;
    float x = static_cast<float>(x1);
    float y = static_cast<float>(y1);

    for (int i = 0; i <= length; i++) {
        points.push_back({static_cast<int>(std::round(x)),
                          static_cast<int>(std::round(y))});
        x += dxInc;
        y += dyInc;
    }
    return points;
}

std::vector<Point> Rasterizer::bresenhamLine(int x1, int y1, int x2, int y2) {
    std::vector<Point> points;
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int x = x1;
    int y = y1;

    while (true) {
        points.push_back({x, y});
        if (x == x2 && y == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
    return points;
}

std::vector<Point> Rasterizer::bresenhamCircle(int cx, int cy, int radius) {
    std::vector<Point> points;
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    auto addCirclePoints = [&](int px, int py) {
        points.push_back({cx + px, cy + py});
        points.push_back({cx + py, cy + px});
        points.push_back({cx - px, cy + py});
        points.push_back({cx - py, cy + px});
        points.push_back({cx + px, cy - py});
        points.push_back({cx + py, cy - px});
        points.push_back({cx - px, cy - py});
        points.push_back({cx - py, cy - px});
    };

    while (x <= y) {
        addCirclePoints(x, y);
        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y -= 1;
        }
        x += 1;
    }
    return points;
}