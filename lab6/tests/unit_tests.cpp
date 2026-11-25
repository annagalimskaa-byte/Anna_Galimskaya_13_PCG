#include <cmath>
#include <cstdio>

#include "math3d.h"
#include "mesh.h"
#include "renderer.h"

using namespace m3d;

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

static bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) < eps; }
static bool near(Vec3 a, Vec3 b, double eps = 1e-9) {
    return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps);
}

static void testBasicMatrices() {
    std::puts("[базовые матрицы]");
    const Mat4 I = Mat4::identity();
    const Mat4 T = translation(1, 2, 3);
    CHECK(maxAbsDiff(I * T, T) < 1e-12 && maxAbsDiff(T * I, T) < 1e-12);
    CHECK(near(transformPoint(T, {1, 1, 1}), {2, 3, 4}));
    CHECK(near(transformDirection(T, {1, 1, 1}), {1, 1, 1}));
    CHECK(near(transformPoint(scaling(2, -3, 0.5), {1, 1, 4}), {2, -3, 2}));
    CHECK(near(transformPoint(rotationZ(90), {1, 0, 0}), {0, 1, 0}));
    CHECK(near(transformPoint(rotationX(90), {0, 1, 0}), {0, 0, 1}));
    CHECK(near(transformPoint(rotationY(90), {0, 0, 1}), {1, 0, 0}));
    CHECK(maxAbsDiff(rotationX(30) * rotationX(-30), I) < 1e-12);
    CHECK(near(det3(rotationZ(37)), 1.0));
    CHECK(near(det3(scaling(-1, 1, 1)), -1.0));
    CHECK(near(det3(scaling(2, 3, 4)), 24.0));
    Mat4 W = Mat4::identity();
    W(3, 3) = 2;
    CHECK(near(transformPoint(W, {2, 4, 6}), {1, 2, 3}));
    CHECK(near(cross({1, 0, 0}, {0, 1, 0}), {0, 0, 1}));
    CHECK(near(dot({1, 2, 3}, {4, 5, 6}), 32));
}

static void testNonCommutative() {
    std::puts("[некоммутативность]");
    const Mat4 T = translation(50, 0, 0);
    const Mat4 R = rotationZ(90);
    const Mat4 TR = T * R, RT = R * T;
    CHECK(maxAbsDiff(TR, RT) > 1.0);
    CHECK(near(transformPoint(TR, {10, 0, 0}), {50, 10, 0}));
    CHECK(near(transformPoint(RT, {10, 0, 0}), {0, 60, 0}));
    CHECK(maxAbsDiff(rotationX(90) * rotationY(90), rotationY(90) * rotationX(90)) > 0.5);
    CHECK(maxAbsDiff(rotationZ(20) * rotationZ(50), rotationZ(50) * rotationZ(20)) < 1e-12);
}

static void testArbitraryAxis() {
    std::puts("[поворот вокруг оси P1P2]");
    const Vec3 axes[][2] = {
        {{80, -120, 20}, {80, 120, 60}},
        {{1, 2, 3}, {4, -5, 6}},
        {{-30, 10, 0}, {70, 10, 0}},
        {{0, 0, 0}, {0, 0, 1}},
        {{5, 5, 5}, {5, 5, -20}},
    };
    const double angles[] = {0, 15, 90, -73.5, 180, 359};
    for (const auto& ax : axes)
        for (double ang : angles) {
            const Mat4 A = rotationAroundLine(ax[0], ax[1], ang);
            const Mat4 B = rotationAroundLineComposite(ax[0], ax[1], ang);
            CHECK(maxAbsDiff(A, B) < 1e-9);
            CHECK(near(transformPoint(A, ax[0]), ax[0], 1e-9));
            CHECK(near(transformPoint(A, ax[0] * 0.3 + ax[1] * 0.7), ax[0] * 0.3 + ax[1] * 0.7, 1e-8));
            CHECK(near(det3(A), 1.0, 1e-12));
        }
    CHECK(maxAbsDiff(rotationAroundLine({0, 0, 0}, {0, 0, 5}, 33), rotationZ(33)) < 1e-12);
    const Vec3 p1{80, -120, 20}, p2{80, 120, 60}, q{10, 20, 30};
    const Vec3 u = normalize(p2 - p1);
    auto distToAxis = [&](Vec3 p) { return length(cross(p - p1, u)); };
    CHECK(near(distToAxis(transformPoint(rotationAroundLine(p1, p2, 47), q)), distToAxis(q), 1e-9));
    CHECK(maxAbsDiff(rotationAroundLine(p1, p2, 360), Mat4::identity()) < 1e-9);
    CHECK(maxAbsDiff(rotationAroundLine(p1, p2, 40), rotationAxisOrigin(u, 40)) > 1.0);
}

static void testCamera() {
    std::puts("[камера]");
    const Vec3 eye{100, 50, 200}, target{0, 10, 0};
    const Mat4 V = lookAt(eye, target, {0, 1, 0});
    CHECK(near(transformPoint(V, eye), {0, 0, 0}, 1e-9));
    const double d = length(target - eye);
    CHECK(near(transformPoint(V, target), {0, 0, -d}, 1e-9));
    CHECK(near(det3(V), 1.0, 1e-12));
    const Mat4 P = perspective(60, 1.5, 2, 1000);
    CHECK(near(perspectiveDivide(P * Vec4{0, 0, -2, 1}).z, -1.0, 1e-12));
    CHECK(near(perspectiveDivide(P * Vec4{0, 0, -1000, 1}).z, 1.0, 1e-9));
    const Vec4 c = P * Vec4{3, 4, -10, 1};
    CHECK(near(c.w, 10.0));
    CHECK(std::fabs(perspectiveDivide(P * Vec4{10, 0, -100, 1}).x) <
          std::fabs(perspectiveDivide(P * Vec4{10, 0, -20, 1}).x));
    const Mat4 O = orthographic(-10, 10, -5, 5, 1, 100);
    CHECK(near(transformPoint(O, {10, 5, -1}), {1, 1, -1}));
    CHECK(near(transformPoint(O, {-10, -5, -100}), {-1, -1, 1}));
}

static double polyArea(const std::vector<Vec3>& p) {
    double a = 0;
    for (size_t i = 0; i < p.size(); ++i) {
        const Vec3 q = p[i], r = p[(i + 1) % p.size()];
        a += q.x * r.y - q.y * r.x;
    }
    return a / 2;
}

static void testGeneration() {
    std::puts("[генерация по Seed]");
    int badRange = 0, badTopo = 0, badTri = 0, badOrient = 0;
    for (int seed = -500; seed <= 3000; ++seed) {
        const auto p = mesh::paramsFromSeed(seed);
        if (p.N < 5 || p.N > 8 || p.M < 3 || p.M > 6 || p.rIn < 55 || p.rIn > 75 || p.rOut <= p.rIn ||
            p.rHole >= 0.6 * p.rIn || p.H < 40 || p.H > 80 || p.twistDeg < 15 || p.twistDeg > 45 ||
            p.taper < 0.75 || p.taper > 0.95 || std::hypot(p.dx, p.dy) + p.rHole >= p.rInscribed)
            ++badRange;
        for (int L : {1, 4}) {
            auto pl = p;
            pl.layers = L;
            const auto m = mesh::buildMesh(pl);
            const auto st = mesh::computeStats(m, Mat4::identity());
            const int ring = 2 * p.N + p.M;
            if (st.euler != 0 || !st.closedManifold || st.V != (L + 1) * ring || st.F != 2 * (L + 1) * ring ||
                st.E != 3 * (L + 1) * ring)
                ++badTopo;
            double vol = 0;
            for (const auto& t : m.triangles) {
                const Vec3 a = toVec3(m.vertices[t[0]]), b = toVec3(m.vertices[t[1]]), c = toVec3(m.vertices[t[2]]);
                vol += dot(a, cross(b, c)) / 6.0;
            }
            if (vol <= 0) ++badOrient;
        }
        const auto outer = mesh::outerContour(p), hole = mesh::holeContour(p);
        const auto tris = mesh::triangulateWithHole(outer, hole);
        std::vector<Vec3> all(outer);
        all.insert(all.end(), hole.begin(), hole.end());
        double s = 0;
        bool positive = true;
        for (const auto& t : tris) {
            const double a = ((all[t[1]].x - all[t[0]].x) * (all[t[2]].y - all[t[0]].y) -
                              (all[t[1]].y - all[t[0]].y) * (all[t[2]].x - all[t[0]].x)) / 2;
            if (a <= 0) positive = false;
            s += a;
        }
        const double expected = polyArea(outer) - polyArea(hole);
        if (!positive || static_cast<int>(tris.size()) != 2 * p.N + p.M || std::fabs(s - expected) > 1e-6 * expected)
            ++badTri;
    }
    CHECK(badRange == 0);
    CHECK(badTopo == 0);
    CHECK(badTri == 0);
    CHECK(badOrient == 0);

    const auto a = mesh::generate(42), b = mesh::generate(42), c = mesh::generate(43);
    CHECK(a.vertices.size() == b.vertices.size() && near(toVec3(a.vertices[5]), toVec3(b.vertices[5])));
    CHECK(a.params.N != c.params.N || a.params.M != c.params.M || !near(a.params.rIn, c.params.rIn));
    bool w1 = true;
    for (const auto& v : a.vertices) w1 = w1 && v.w == 1.0;
    CHECK(w1);
    const auto& p = a.params;
    const Vec3 f0 = toVec3(a.vertices[0]), b0 = toVec3(a.vertices[a.ringSize]);
    CHECK(near(length({b0.x, b0.y, 0}), p.taper * length({f0.x, f0.y, 0}), 1e-9));
    CHECK(near(rad2deg(std::atan2(b0.y, b0.x) - std::atan2(f0.y, f0.x)), p.twistDeg, 1e-9) ||
          near(rad2deg(std::atan2(b0.y, b0.x) - std::atan2(f0.y, f0.x)) + 360, p.twistDeg, 1e-9));
    CHECK(near(b0.z, p.H));

    const auto st = mesh::computeStats(a, Mat4::identity());
    std::printf("  seed 42: N=%d M=%d Rin=%.4f rhole=%.4f d=(%.4f, %.4f) H=%.4f twist=%.4f k=%.4f\n", p.N, p.M,
                p.rIn, p.rHole, p.dx, p.dy, p.H, p.twistDeg, p.taper);
    std::printf("           V=%d E=%d F=%d  C=(%.4f, %.4f, %.4f)  S=%.4f\n", st.V, st.E, st.F, st.centroid.x,
                st.centroid.y, st.centroid.z, st.area);
}

static void testStatsUnderTransform() {
    std::puts("[центр масс и площадь после преобразований]");
    const auto m = mesh::generate(7);
    const auto s0 = mesh::computeStats(m, Mat4::identity());
    const auto sT = mesh::computeStats(m, translation(10, -20, 5));
    CHECK(near(sT.centroid, s0.centroid + Vec3{10, -20, 5}, 1e-9));
    CHECK(near(sT.area, s0.area, 1e-6));
    const auto sR = mesh::computeStats(m, rotationAroundLine({80, -120, 20}, {80, 120, 60}, 70));
    CHECK(near(sR.area, s0.area, 1e-6));
    const auto sS = mesh::computeStats(m, scaling(2, 2, 2));
    CHECK(near(sS.area, 4 * s0.area, 1e-6));
    const auto sM = mesh::computeStats(m, scaling(-1, 1, 1));
    CHECK(sM.det < 0 && near(sM.area, s0.area, 1e-6) && near(sM.centroid.x, -s0.centroid.x, 1e-9));
}

static void testCullingAndZBuffer() {
    std::puts("[отсечение нелицевых граней и Z-буфер]");
    const auto m = mesh::generate(42);
    render::OrbitCamera cam;
    const auto view = cam.makeView(320, 240);

    for (double sx : {1.0, -1.0}) {
        const Mat4 model = scaling(sx, 1, 1);
        const auto world = render::transformVertices(m, model);
        render::Framebuffer fb;
        fb.resize(320, 240);
        render::Options opt;
        const auto st = render::drawMesh(fb, view, m, world, det3(model), opt);
        CHECK(st.drawn + st.culled == static_cast<int>(m.triangles.size()));
        CHECK(st.drawn > 0 && st.culled > 0);
        render::Framebuffer fb2;
        fb2.resize(320, 240);
        opt.cull = false;
        render::drawMesh(fb2, view, m, world, det3(model), opt);
        int diff = 0;
        for (size_t i = 0; i < fb.color.size(); ++i)
            if ((fb.color[i] >> 24) != (fb2.color[i] >> 24)) ++diff;
        CHECK(diff < 50);
    }
    {
        const Mat4 model = scaling(-1, 1, 1);
        const auto world = render::transformVertices(m, model);
        const Vec3 eye = cam.eye();
        int frontCorrect = 0, frontWrong = 0;
        for (const auto& t : m.triangles) {
            const Vec3 a = world[t[0]], b = world[t[1]], c = world[t[2]];
            if (dot(render::faceNormal(a, b, c, -1), eye - a) > 0) ++frontCorrect;
            if (dot(cross(b - a, c - a), eye - a) > 0) ++frontWrong;
        }
        CHECK(frontCorrect + frontWrong == static_cast<int>(m.triangles.size()));
    }
    {
        mesh::Mesh twoTris;
        twoTris.vertices = {{-50, -50, 0, 1}, {50, -50, 0, 1}, {0, 50, 0, 1},
                            {-50, 50, 30, 1}, {0, -50, 30, 1}, {50, 50, 30, 1}};
        twoTris.triangles = {{0, 1, 2}, {3, 4, 5}};
        twoTris.groups = {mesh::FaceGroup::OuterWall, mesh::FaceGroup::HoleWall};
        const auto world = render::transformVertices(twoTris, Mat4::identity());
        render::OrbitCamera c2;
        c2.target = {0, 0, 0};
        c2.yawDeg = 0;
        c2.pitchDeg = 0;
        c2.distance = 300;
        const auto v2 = c2.makeView(200, 200);
        render::Options opt;
        opt.cull = false;
        opt.edges = false;
        render::Framebuffer f1, f2;
        f1.resize(200, 200);
        f2.resize(200, 200);
        render::drawMesh(f1, v2, twoTris, world, 1, opt);
        std::swap(twoTris.triangles[0], twoTris.triangles[1]);
        std::swap(twoTris.groups[0], twoTris.groups[1]);
        render::drawMesh(f2, v2, twoTris, world, 1, opt);
        CHECK(f1.color == f2.color);
        opt.zbuffer = false;
        f1.clear();
        f2.clear();
        render::drawMesh(f1, v2, twoTris, world, 1, opt);
        std::swap(twoTris.triangles[0], twoTris.triangles[1]);
        std::swap(twoTris.groups[0], twoTris.groups[1]);
        render::drawMesh(f2, v2, twoTris, world, 1, opt);
        CHECK(f1.color != f2.color);
    }
}

int main() {
    testBasicMatrices();
    testNonCommutative();
    testArbitraryAxis();
    testCamera();
    testGeneration();
    testStatsUnderTransform();
    testCullingAndZBuffer();
    std::printf("\nПроверок: %d, провалено: %d\n", g_total, g_failed);
    return g_failed == 0 ? 0 : 1;
}
