#include "Clipper.h"
#include <algorithm>
#include <cmath>

namespace {
const int INSIDE = 0;
const int LEFT   = 1;
const int RIGHT  = 2;
const int BOTTOM = 4;
const int TOP    = 8;

double dot(const Point& a, const Point& b) { return a.x * b.x + a.y * b.y; }
double cross(const Point& a, const Point& b) { return a.x * b.y - a.y * b.x; }
Point sub(const Point& a, const Point& b) { return {a.x - b.x, a.y - b.y}; }
double length(const Point& a, const Point& b) { return std::hypot(b.x - a.x, b.y - a.y); }
const double EPS = 1e-9;
}

int Clipper::computeCode(const Point& p, const Rect& rect) {
    int code = INSIDE;
    if (p.x < rect.xmin) code |= LEFT;
    else if (p.x > rect.xmax) code |= RIGHT;
    if (p.y < rect.ymin) code |= BOTTOM;
    else if (p.y > rect.ymax) code |= TOP;
    return code;
}

Point Clipper::lerp(const Segment& s, double t) {
    return {s.p1.x + (s.p2.x - s.p1.x) * t, s.p1.y + (s.p2.y - s.p1.y) * t};
}

// ---------------------------------------------------------------------------
// Алгоритм средней точки.
// Каждый кусок отрезка [t0, t1] классифицируется по кодам концов (как в Коэне-Сазерленде):
//   * оба кода 0          -> кусок полностью видим, запоминаем его;
//   * code0 & code1 != 0  -> кусок полностью невидим, отбрасываем;
//   * иначе               -> делим кусок пополам в средней точке и проверяем обе половины.
// Деление прекращается, когда длина куска меньше eps. Так как окно выпуклое,
// видимая часть — один непрерывный отрезок [tMin, tMax].
// ---------------------------------------------------------------------------
void Clipper::midpointRec(const Segment& s, double t0, double t1, const Rect& rect, double eps,
                          double& tMin, double& tMax, bool& found) {
    Point a = lerp(s, t0), b = lerp(s, t1);
    int ca = computeCode(a, rect), cb = computeCode(b, rect);

    if (ca == INSIDE && cb == INSIDE) {               // тривиально видим
        tMin = std::min(tMin, t0); tMax = std::max(tMax, t1); found = true;
        return;
    }
    if (ca & cb) return;                              // тривиально невидим

    if (length(a, b) < eps) {                         // достигли нужной точности
        if (ca == INSIDE) { tMin = std::min(tMin, t0); tMax = std::max(tMax, t0); found = true; }
        if (cb == INSIDE) { tMin = std::min(tMin, t1); tMax = std::max(tMax, t1); found = true; }
        return;
    }

    double tm = (t0 + t1) / 2.0;                      // средняя точка
    midpointRec(s, t0, tm, rect, eps, tMin, tMax, found);
    midpointRec(s, tm, t1, rect, eps, tMin, tMax, found);
}

std::optional<Segment> Clipper::midpoint(const Segment& seg, const Rect& rect, double eps) {
    double tMin = 1.0, tMax = 0.0;
    bool found = false;
    midpointRec(seg, 0.0, 1.0, rect, eps, tMin, tMax, found);
    if (!found) return std::nullopt;
    return Segment{lerp(seg, tMin), lerp(seg, tMax)};
}

// ---------------------------------------------------------------------------
// Вспомогательное для многоугольника
// ---------------------------------------------------------------------------
double Clipper::signedArea(const std::vector<Point>& poly) {
    double s = 0;
    for (size_t i = 0; i < poly.size(); ++i) {
        const Point& a = poly[i];
        const Point& b = poly[(i + 1) % poly.size()];
        s += a.x * b.y - b.x * a.y;
    }
    return s / 2.0;
}

bool Clipper::isConvex(const std::vector<Point>& poly) {
    const size_t n = poly.size();
    if (n < 3) return false;
    int sign = 0;
    for (size_t i = 0; i < n; ++i) {
        Point e1 = sub(poly[(i + 1) % n], poly[i]);
        Point e2 = sub(poly[(i + 2) % n], poly[(i + 1) % n]);
        double c = cross(e1, e2);
        if (std::abs(c) < EPS) continue;              // три точки на одной прямой — допустимо
        int s = c > 0 ? 1 : -1;
        if (sign == 0) sign = s;
        else if (s != sign) return false;
    }
    return sign != 0;
}

// ---------------------------------------------------------------------------
// Алгоритм Кируса-Бека.
// P(t) = P1 + (P2 - P1) t, t ∈ [0, 1].  Для каждого ребра Vi -> Vi+1 берём ВНУТРЕННЮЮ нормаль N.
// Точка видна относительно ребра, если N · (P(t) - Vi) >= 0.
//   num = N · (P1 - Vi),  den = N · D
//   den == 0 : отрезок параллелен ребру; если num < 0 — он целиком снаружи.
//   den  > 0 : точка входа,  tE = max(tE, -num/den)
//   den  < 0 : точка выхода, tL = min(tL, -num/den)
// Отрезок видим, если tE <= tL.
// ---------------------------------------------------------------------------
std::optional<Segment> Clipper::cyrusBeck(const Segment& seg, const std::vector<Point>& poly) {
    const size_t n = poly.size();
    if (n < 3) return std::nullopt;

    const double area = signedArea(poly);
    if (std::abs(area) < EPS) return std::nullopt;
    const bool ccw = area > 0;                        // направление обхода определяет сторону нормали

    const Point d = sub(seg.p2, seg.p1);
    double tE = 0.0, tL = 1.0;

    for (size_t i = 0; i < n; ++i) {
        const Point& v1 = poly[i];
        const Point& v2 = poly[(i + 1) % n];
        Point e = sub(v2, v1);
        Point normal = ccw ? Point{-e.y, e.x} : Point{e.y, -e.x};   // внутренняя нормаль

        double num = dot(normal, sub(seg.p1, v1));
        double den = dot(normal, d);

        if (std::abs(den) < EPS) {
            if (num < 0) return std::nullopt;        // параллелен ребру и снаружи
            continue;
        }
        double t = -num / den;
        if (den > 0) tE = std::max(tE, t);
        else         tL = std::min(tL, t);
        if (tE > tL) return std::nullopt;
    }
    return Segment{lerp(seg, tE), lerp(seg, tL)};
}
