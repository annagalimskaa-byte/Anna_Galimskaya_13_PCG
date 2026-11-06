#pragma once
#include <vector>
#include <optional>

struct Point { double x, y; };
struct Segment { Point p1, p2; };
struct Rect { double xmin, ymin, xmax, ymax; };

class Clipper {
public:
    // Часть 1 (вариант 6): алгоритм средней точки — отсечение отрезка прямоугольным окном.
    // eps — точность деления отрезка пополам (в мировых единицах).
    static std::optional<Segment> midpoint(const Segment& seg, const Rect& rect, double eps = 1e-4);

    // Часть 2 (чётный вариант): алгоритм Кируса-Бека — отсечение отрезка выпуклым многоугольником.
    // Работает при любом порядке обхода вершин (по и против часовой стрелки).
    static std::optional<Segment> cyrusBeck(const Segment& seg, const std::vector<Point>& poly);

    static bool isConvex(const std::vector<Point>& poly);
    static double signedArea(const std::vector<Point>& poly);

private:
    static int computeCode(const Point& p, const Rect& rect);
    static Point lerp(const Segment& s, double t);
    static void midpointRec(const Segment& s, double t0, double t1, const Rect& rect, double eps,
                            double& tMin, double& tMax, bool& found);
};
