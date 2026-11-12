#include <cmath>
#include <cstdio>
#include <string>

#include "clipping.h"
#include "input_parser.h"

using geom::Point;
using geom::Polygon;
using geom::Rect;
using geom::Segment;

static int g_failed = 0;
static int g_total = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        ++g_total;                                                             \
        if (!(cond)) {                                                         \
            ++g_failed;                                                        \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                      \
    } while (0)

static bool near(Point a, Point b, double eps = 1e-5) {
    return std::fabs(a.x - b.x) < eps && std::fabs(a.y - b.y) < eps;
}

static bool sameSegment(const Segment& s, Point a, Point b, double eps = 1e-5) {
    return (near(s.a, a, eps) && near(s.b, b, eps)) || (near(s.a, b, eps) && near(s.b, a, eps));
}

static const Rect kWin{-5, -3, 5, 4};

static void testOutCodes() {
    std::puts("[outcodes]");
    CHECK(clip::computeOutCode({0, 0}, kWin) == clip::kInside);
    CHECK(clip::computeOutCode({-6, 0}, kWin) == clip::kLeft);
    CHECK(clip::computeOutCode({6, 5}, kWin) == (clip::kRight | clip::kTop));
    CHECK(clip::computeOutCode({-6, -4}, kWin) == (clip::kLeft | clip::kBottom));
    CHECK(clip::computeOutCode({5, 4}, kWin) == clip::kInside);
}

static void testMidpoint() {
    std::puts("[midpoint]");
    auto r = clip::midpointClip({{-2, -1}, {3, 2}}, kWin);
    CHECK(r.visible && sameSegment(*r.visible, {-2, -1}, {3, 2}));
    CHECK(r.steps == 1);
    r = clip::midpointClip({{-9, 5}, {-5.5, 9}}, kWin);
    CHECK(!r.visible);
    r = clip::midpointClip({{-9, 0}, {9, 0}}, kWin);
    CHECK(r.visible && sameSegment(*r.visible, {-5, 0}, {5, 0}));
    r = clip::midpointClip({{4, -7}, {4, 7}}, kWin);
    CHECK(r.visible && sameSegment(*r.visible, {4, -3}, {4, 4}));
    r = clip::midpointClip({{-8, -6}, {8, 6}}, kWin);
    CHECK(r.visible && sameSegment(*r.visible, {-4, -3}, {5, 3.75}));
    r = clip::midpointClip({{0, 0}, {10, 0}}, kWin);
    CHECK(r.visible && sameSegment(*r.visible, {0, 0}, {5, 0}));
    r = clip::midpointClip({{-8, 0.5 + 1}, {-1, 7.5 + 1}}, kWin);
    CHECK(!r.visible);
    CHECK(r.steps > 1);
    r = clip::midpointClip({{-9, 0}, {-1, 8}}, kWin);
    CHECK(r.visible && near(r.visible->a, {-5, 4}, 1e-4) && near(r.visible->b, {-5, 4}, 1e-4));
    r = clip::midpointClip({{-7, 4}, {7, 4}}, kWin);
    CHECK(r.visible && sameSegment(*r.visible, {-5, 4}, {5, 4}));
    r = clip::midpointClip({{1, 1}, {1, 1}}, kWin);
    CHECK(r.visible && near(r.visible->a, {1, 1}));
    r = clip::midpointClip({{9, 9}, {9, 9}}, kWin);
    CHECK(!r.visible);
    r = clip::midpointClip({{9, 0}, {-9, 0}}, kWin);
    CHECK(r.visible && near(r.visible->a, {5, 0}) && near(r.visible->b, {-5, 0}));
}

static void testCyrusBeck() {
    std::puts("[cyrus-beck]");
    const Polygon square = geom::rectToPolygon(kWin);
    const Segment cases[] = {{{-8, -6}, {8, 6}}, {{-9, 0}, {9, 0}}, {{4, -7}, {4, 7}},
                             {{-2, -1}, {3, 2}}, {{0, 0}, {10, 3}}, {{-7, -5}, {-1, 7}}};
    for (const auto& s : cases) {
        auto a = clip::midpointClip(s, kWin);
        auto b = clip::cyrusBeckClip(s, square);
        CHECK(a.visible.has_value() == b.visible.has_value());
        if (a.visible && b.visible) CHECK(sameSegment(*b.visible, a.visible->a, a.visible->b, 1e-5));
    }
    const Polygon tri{{0, 0}, {4, 0}, {0, 4}};
    auto r = clip::cyrusBeckClip({{-1, 1}, {5, 1}}, tri);
    CHECK(r.visible && sameSegment(*r.visible, {0, 1}, {3, 1}));
    const Polygon triCw{{0, 0}, {0, 4}, {4, 0}};
    r = clip::cyrusBeckClip({{-1, 1}, {5, 1}}, triCw);
    CHECK(r.visible && sameSegment(*r.visible, {0, 1}, {3, 1}));
    r = clip::cyrusBeckClip({{-1, -1}, {5, -1}}, tri);
    CHECK(!r.visible);
    r = clip::cyrusBeckClip({{3, 3}, {6, 1}}, tri);
    CHECK(!r.visible);
    r = clip::cyrusBeckClip({{0.5, 0.5}, {1, 2}}, tri);
    CHECK(r.visible && sameSegment(*r.visible, {0.5, 0.5}, {1, 2}));
    const Polygon hex{{2, 0}, {1, 1.7320508}, {-1, 1.7320508}, {-2, 0}, {-1, -1.7320508}, {1, -1.7320508}};
    r = clip::cyrusBeckClip({{-5, 0}, {5, 0}}, hex);
    CHECK(r.visible && sameSegment(*r.visible, {-2, 0}, {2, 0}));
    r = clip::cyrusBeckClip({{0.2, 0.2}, {0.2, 0.2}}, tri);
    CHECK(r.visible.has_value());
    r = clip::cyrusBeckClip({{5, 5}, {5, 5}}, tri);
    CHECK(!r.visible);
}

static void testConvexity() {
    std::puts("[convexity]");
    CHECK(geom::isConvex({{0, 0}, {4, 0}, {0, 4}}));
    CHECK(geom::isConvex({{0, 0}, {0, 4}, {4, 0}}));
    CHECK(!geom::isConvex({{0, 0}, {4, 4}, {0, 2}, {-4, 4}}));
    CHECK(!geom::isConvex({{0, 0}, {1, 1}, {2, 2}}));
    Polygon star;
    for (int i = 0; i < 5; ++i) {
        const double a = geom::kPi / 2 + i * 4 * geom::kPi / 5;
        star.push_back({std::cos(a), std::sin(a)});
    }
    CHECK(!geom::isConvex(star));
}

static void testParser() {
    std::puts("[parser]");
    auto d = io::parseInput("2 *число отрезков*\n0 0 1 1\n2 2 3 3\n-1 -1 5 5 *окно*\n");
    CHECK(d.kind == io::WindowKind::Rectangle);
    CHECK(d.segments.size() == 2);
    CHECK(d.rect.xmax == 5);

    d = io::parseInput("# комментарий\n1\n0 0 1 1\n3\n0 0\n4 0\n0 4\n");
    CHECK(d.kind == io::WindowKind::ConvexPolygon);
    CHECK(d.polygon.size() == 3);

    auto throws = [](const std::string& text) {
        try {
            io::parseInput(text);
        } catch (const io::ParseError&) {
            return true;
        }
        return false;
    };
    CHECK(throws(""));
    CHECK(throws("2\n0 0 1 1\n0 0 5 5\n"));
    CHECK(throws("1\n0 0 1\n0 0 5 5\n"));
    CHECK(throws("1\n0 0 1 1\n5 5 0 0\n"));
    CHECK(throws("1\n0 0 1 abc\n0 0 5 5\n"));
    CHECK(throws("1.5\n0 0 1 1\n0 0 5 5\n"));
    CHECK(throws("1\n0 0 1 1\n4\n0 0\n4 4\n0 2\n-4 4\n"));
}

int main() {
    testOutCodes();
    testMidpoint();
    testCyrusBeck();
    testConvexity();
    testParser();
    std::printf("\n%d/%d проверок пройдено\n", g_total - g_failed, g_total);
    return g_failed == 0 ? 0 : 1;
}
