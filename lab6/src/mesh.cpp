#include "mesh.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>

namespace mesh {

using m3d::Vec3;

namespace {

double cross2(Vec3 a, Vec3 b) { return a.x * b.y - a.y * b.x; }

double orient(Vec3 a, Vec3 b, Vec3 c) { return cross2(b - a, c - a); }

bool segmentsCross(Vec3 a, Vec3 b, Vec3 c, Vec3 d) {
    const double d1 = orient(a, b, c), d2 = orient(a, b, d);
    const double d3 = orient(c, d, a), d4 = orient(c, d, b);
    return ((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0));
}

bool pointInTriangle(Vec3 p, Vec3 a, Vec3 b, Vec3 c, double eps) {
    return orient(a, b, p) >= -eps && orient(b, c, p) >= -eps && orient(c, a, p) >= -eps;
}

double distance2(Vec3 a, Vec3 b) { return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y); }

}

double starInscribedRadius(int n, double rOut, double rIn) {
    const double t = m3d::kPi / n;
    const double num = rOut * rIn * std::sin(t);
    const double den = std::sqrt(rOut * rOut + rIn * rIn - 2.0 * rOut * rIn * std::cos(t));
    return num / den;
}

Params paramsFromSeed(int seed, int layers) {
    Random rnd(seed);
    Params p;
    p.seed = seed;
    p.layers = std::max(1, layers);
    p.N = rnd.integer(5, 8);
    p.M = rnd.integer(3, 6);
    p.rOut = 100.0;
    p.rIn = rnd.uniform(55.0, 75.0);
    p.rInscribed = starInscribedRadius(p.N, p.rOut, p.rIn);
    p.rHole = rnd.uniform(0.35, 0.55) * p.rInscribed;
    p.outerPhaseDeg = rnd.uniform(0.0, 180.0 / p.N);
    p.holePhaseDeg = rnd.uniform(0.0, 360.0 / p.M);
    const double maxShift = 0.85 * p.rInscribed - p.rHole;
    const double shift = rnd.uniform(0.35, 1.0) * maxShift;
    const double phi = rnd.uniform(0.0, 2.0 * m3d::kPi);
    p.dx = shift * std::cos(phi);
    p.dy = shift * std::sin(phi);
    p.H = rnd.uniform(40.0, 80.0);
    p.twistDeg = rnd.uniform(15.0, 45.0);
    p.taper = rnd.uniform(0.75, 0.95);
    return p;
}

std::vector<Vec3> outerContour(const Params& p) {
    std::vector<Vec3> pts;
    const int count = 2 * p.N;
    for (int i = 0; i < count; ++i) {
        const double a = m3d::deg2rad(p.outerPhaseDeg) + 2.0 * m3d::kPi * i / count;
        const double r = (i % 2 == 0) ? p.rOut : p.rIn;
        pts.push_back({r * std::cos(a), r * std::sin(a), 0.0});
    }
    return pts;
}

std::vector<Vec3> holeContour(const Params& p) {
    std::vector<Vec3> pts;
    for (int k = 0; k < p.M; ++k) {
        const double a = m3d::deg2rad(p.holePhaseDeg) + 2.0 * m3d::kPi * k / p.M;
        pts.push_back({p.dx + p.rHole * std::cos(a), p.dy + p.rHole * std::sin(a), 0.0});
    }
    return pts;
}

std::vector<std::array<int, 3>> triangulateWithHole(const std::vector<Vec3>& outer,
                                                    const std::vector<Vec3>& hole) {
    const int n = static_cast<int>(outer.size());
    const int m = static_cast<int>(hole.size());
    std::vector<Vec3> P(outer);
    P.insert(P.end(), hole.begin(), hole.end());

    int h = 0;
    for (int k = 1; k < m; ++k)
        if (hole[k].x > hole[h].x) h = k;
    std::vector<int> order(n);
    for (int i = 0; i < n; ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return distance2(outer[a], hole[h]) < distance2(outer[b], hole[h]);
    });
    int v = order[0];
    for (int cand : order) {
        bool visible = true;
        for (int i = 0; i < n && visible; ++i) {
            const int j = (i + 1) % n;
            if (i == cand || j == cand) continue;
            if (segmentsCross(hole[h], outer[cand], outer[i], outer[j])) visible = false;
        }
        for (int k = 0; k < m && visible; ++k) {
            const int l = (k + 1) % m;
            if (k == h || l == h) continue;
            if (segmentsCross(hole[h], outer[cand], hole[k], hole[l])) visible = false;
        }
        if (visible) { v = cand; break; }
    }

    std::vector<int> poly;
    for (int i = 0; i <= v; ++i) poly.push_back(i);
    for (int s = 0; s <= m; ++s) poly.push_back(n + ((h - s) % m + m) % m);
    for (int i = v; i < n; ++i) poly.push_back(i);

    double scale = 0;
    for (const auto& q : P) scale = std::max(scale, std::max(std::fabs(q.x), std::fabs(q.y)));
    const double eps = 1e-9 * std::max(1.0, scale * scale);

    std::vector<std::array<int, 3>> tris;
    while (poly.size() > 3) {
        const int cnt = static_cast<int>(poly.size());
        int earAt = -1;
        double bestConvex = -1e300;
        int bestAt = 0;
        for (int i = 0; i < cnt; ++i) {
            const int a = poly[(i + cnt - 1) % cnt], b = poly[i], c = poly[(i + 1) % cnt];
            const double area = orient(P[a], P[b], P[c]);
            if (area > bestConvex) { bestConvex = area; bestAt = i; }
            if (area <= eps) continue;
            bool empty = true;
            for (int j = 0; j < cnt && empty; ++j) {
                const int q = poly[j];
                if (q == a || q == b || q == c) continue;
                if (pointInTriangle(P[q], P[a], P[b], P[c], eps)) empty = false;
            }
            if (empty) { earAt = i; break; }
        }
        if (earAt < 0) earAt = bestAt;
        const int a = poly[(earAt + cnt - 1) % cnt], b = poly[earAt], c = poly[(earAt + 1) % cnt];
        if (orient(P[a], P[b], P[c]) > eps) tris.push_back({a, b, c});
        poly.erase(poly.begin() + earAt);
    }
    if (orient(P[poly[0]], P[poly[1]], P[poly[2]]) > eps) tris.push_back({poly[0], poly[1], poly[2]});
    return tris;
}

Mesh buildMesh(const Params& p) {
    Mesh mesh;
    mesh.params = p;
    const auto outer = outerContour(p);
    const auto hole = holeContour(p);
    const int n = static_cast<int>(outer.size());
    const int m = static_cast<int>(hole.size());
    const int ring = n + m;
    const int L = std::max(1, p.layers);
    mesh.ringSize = ring;

    for (int j = 0; j <= L; ++j) {
        const double t = static_cast<double>(j) / L;
        const double s = 1.0 + t * (p.taper - 1.0);
        const m3d::Mat4 tr = m3d::translation(0, 0, t * p.H) * m3d::rotationZ(t * p.twistDeg) *
                             m3d::scaling(s, s, 1.0);
        for (const auto& q : outer) mesh.vertices.push_back(tr * m3d::toVec4(q));
        for (const auto& q : hole) mesh.vertices.push_back(tr * m3d::toVec4(q));
    }
    auto O = [&](int j, int i) { return j * ring + ((i % n) + n) % n; };
    auto Hh = [&](int j, int k) { return j * ring + n + ((k % m) + m) % m; };
    auto add = [&](int a, int b, int c, FaceGroup g) {
        mesh.triangles.push_back({a, b, c});
        mesh.groups.push_back(g);
    };

    for (int j = 0; j < L; ++j) {
        for (int i = 0; i < n; ++i) {
            add(O(j, i), O(j, i + 1), O(j + 1, i + 1), FaceGroup::OuterWall);
            add(O(j, i), O(j + 1, i + 1), O(j + 1, i), FaceGroup::OuterWall);
        }
        for (int k = 0; k < m; ++k) {
            add(Hh(j, k), Hh(j + 1, k + 1), Hh(j, k + 1), FaceGroup::HoleWall);
            add(Hh(j, k), Hh(j + 1, k), Hh(j + 1, k + 1), FaceGroup::HoleWall);
        }
    }
    const auto caps = triangulateWithHole(outer, hole);
    for (const auto& t : caps) add(t[0], t[2], t[1], FaceGroup::FrontCap);
    for (const auto& t : caps) add(L * ring + t[0], L * ring + t[1], L * ring + t[2], FaceGroup::BackCap);

    const long long V = static_cast<long long>(mesh.vertices.size());
    std::map<long long, int> seen;
    for (const auto& t : mesh.triangles)
        for (int e = 0; e < 3; ++e) {
            int a = t[e], b = t[(e + 1) % 3];
            if (a > b) std::swap(a, b);
            const long long key = a * V + b;
            if (seen.emplace(key, static_cast<int>(mesh.edges.size())).second) mesh.edges.push_back({a, b});
        }
    return mesh;
}

Mesh generate(int seed, int layers) { return buildMesh(paramsFromSeed(seed, layers)); }

bool isClosedManifold(const Mesh& mesh) {
    const long long V = static_cast<long long>(mesh.vertices.size());
    std::unordered_map<long long, int> directed;
    for (const auto& t : mesh.triangles)
        for (int e = 0; e < 3; ++e) ++directed[t[e] * V + t[(e + 1) % 3]];
    for (const auto& [key, cnt] : directed) {
        if (cnt != 1) return false;
        const long long a = key / V, b = key % V;
        auto it = directed.find(b * V + a);
        if (it == directed.end() || it->second != 1) return false;
    }
    return true;
}

void computeChecksum(const Mesh& mesh, const m3d::Mat4& model, Vec3& centroid, double& area) {
    std::vector<Vec3> w;
    w.reserve(mesh.vertices.size());
    Vec3 sum;
    for (const auto& v : mesh.vertices) {
        const Vec3 p = m3d::perspectiveDivide(model * v);
        w.push_back(p);
        sum = sum + p;
    }
    centroid = w.empty() ? Vec3{} : sum / static_cast<double>(w.size());
    area = 0;
    for (const auto& t : mesh.triangles)
        area += 0.5 * m3d::length(m3d::cross(w[t[1]] - w[t[0]], w[t[2]] - w[t[0]]));
}

Stats computeStats(const Mesh& mesh, const m3d::Mat4& model) {
    Stats s;
    s.V = static_cast<int>(mesh.vertices.size());
    s.E = static_cast<int>(mesh.edges.size());
    s.F = static_cast<int>(mesh.triangles.size());
    s.euler = s.V - s.E + s.F;
    s.closedManifold = isClosedManifold(mesh);
    s.det = m3d::det3(model);
    computeChecksum(mesh, model, s.centroid, s.area);
    return s;
}

}
