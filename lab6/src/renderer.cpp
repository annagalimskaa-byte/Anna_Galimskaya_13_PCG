#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace render {

using m3d::Vec3;
using m3d::Vec4;

void Framebuffer::resize(int w, int h) {
    width = std::max(1, w);
    height = std::max(1, h);
    color.assign(static_cast<size_t>(width) * height, 0u);
    depth.assign(static_cast<size_t>(width) * height, std::numeric_limits<float>::infinity());
}

void Framebuffer::clear() {
    std::fill(color.begin(), color.end(), 0u);
    std::fill(depth.begin(), depth.end(), std::numeric_limits<float>::infinity());
}

Vec3 View::directionToViewer(Vec3 p) const { return perspective ? eye - p : toViewer; }

namespace {

struct ClipVtx {
    Vec4 c;
};

struct ScreenVtx {
    double x, y, z;
};

ScreenVtx toScreen(const View& v, Vec4 clip) {
    const Vec3 ndc = m3d::perspectiveDivide(clip);  // перспективное деление на w
    return {(ndc.x + 1.0) * 0.5 * v.width, (1.0 - ndc.y) * 0.5 * v.height, ndc.z};
}

inline double nearDist(Vec4 c) { return c.z + c.w; }  // ближняя плоскость: z_clip ≥ −w

Vec4 lerp4(Vec4 a, Vec4 b, double t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
}

inline uint32_t blend(uint32_t dst, uint32_t src, double alpha) {
    if (alpha <= 0) return dst;
    if (alpha >= 1) return src | 0xFF000000u;
    const double da = ((dst >> 24) & 255) / 255.0;
    const double oa = alpha + da * (1 - alpha);
    auto ch = [&](int shift) {
        const double s = (src >> shift) & 255, d = (dst >> shift) & 255;
        return static_cast<uint32_t>(std::lround((s * alpha + d * da * (1 - alpha)) / std::max(oa, 1e-9)));
    };
    return (static_cast<uint32_t>(std::lround(oa * 255)) << 24) | (ch(16) << 16) | (ch(8) << 8) | ch(0);
}

uint32_t shade(int r, int g, int b, double k) {
    k = std::clamp(k, 0.0, 1.0);
    return rgba(static_cast<int>(r * k), static_cast<int>(g * k), static_cast<int>(b * k));
}

// Растеризация треугольника по барицентрическим координатам.
// edgeMask: бит 0 — ребро ab, бит 1 — bc, бит 2 — ca (рисуются поверх заливки).
void rasterTriangle(Framebuffer& fb, ScreenVtx a, ScreenVtx b, ScreenVtx c, uint32_t col,
                    uint32_t edgeCol, int edgeMask, bool ztest) {
    const double area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (std::fabs(area) < 1e-12) return;
    const int x0 = std::max(0, static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))));
    const int x1 = std::min(fb.width - 1, static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))));
    const int y0 = std::max(0, static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))));
    const int y1 = std::min(fb.height - 1, static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))));
    if (x0 > x1 || y0 > y1) return;
    const double lab = std::hypot(b.x - a.x, b.y - a.y);
    const double lbc = std::hypot(c.x - b.x, c.y - b.y);
    const double lca = std::hypot(a.x - c.x, a.y - c.y);
    const double inv = 1.0 / area;

    for (int y = y0; y <= y1; ++y) {
        const double py = y + 0.5;
        for (int x = x0; x <= x1; ++x) {
            const double px = x + 0.5;
            const double wa = (c.x - b.x) * (py - b.y) - (c.y - b.y) * (px - b.x);  // напротив a (ребро bc)
            const double wb = (a.x - c.x) * (py - c.y) - (a.y - c.y) * (px - c.x);  // ребро ca
            const double wc = (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);  // ребро ab
            const double ba = wa * inv, bb = wb * inv, bc = wc * inv;
            if (ba < -1e-9 || bb < -1e-9 || bc < -1e-9) continue;
            const double z = ba * a.z + bb * b.z + bc * c.z;
            const size_t idx = static_cast<size_t>(y) * fb.width + x;
            if (ztest) {
                if (z >= fb.depth[idx]) continue;  // тест глубины
                fb.depth[idx] = static_cast<float>(z);
            }
            uint32_t out = col;
            if (edgeMask) {
                double d = 1e9;  // расстояние до ближайшего отображаемого ребра в пикселях
                if ((edgeMask & 1) && lab > 0) d = std::min(d, std::fabs(wc) / lab);
                if ((edgeMask & 2) && lbc > 0) d = std::min(d, std::fabs(wa) / lbc);
                if ((edgeMask & 4) && lca > 0) d = std::min(d, std::fabs(wb) / lca);
                if (d < 1.2) out = blend(col, edgeCol, std::min(1.0, 1.2 - d) * 0.85);
            }
            fb.color[idx] = out;
        }
    }
}

void putPixel(Framebuffer& fb, int x, int y, uint32_t col, double alpha) {
    if (x < 0 || y < 0 || x >= fb.width || y >= fb.height) return;
    const size_t idx = static_cast<size_t>(y) * fb.width + x;
    fb.color[idx] = blend(fb.color[idx], col, alpha);
}

}  // namespace

ScreenPoint project(const View& v, Vec3 world) {
    const Vec4 c = v.projView * m3d::toVec4(world);
    ScreenPoint p;
    if (nearDist(c) < 0 || c.w <= 1e-9) return p;
    const ScreenVtx s = toScreen(v, c);
    p.x = s.x;
    p.y = s.y;
    p.z = s.z;
    p.visible = true;
    return p;
}

std::vector<Vec3> transformVertices(const mesh::Mesh& mesh, const m3d::Mat4& model) {
    std::vector<Vec3> out;
    out.reserve(mesh.vertices.size());
    for (const auto& v : mesh.vertices) out.push_back(m3d::perspectiveDivide(model * v));
    return out;
}

Vec3 faceNormal(Vec3 a, Vec3 b, Vec3 c, double det) {
    const Vec3 n = m3d::cross(b - a, c - a);
    return det < 0 ? -n : n;
}

FrameStats drawMesh(Framebuffer& fb, const View& v, const mesh::Mesh& mesh, const std::vector<Vec3>& world,
                    double det, const Options& opt) {
    static const int kColors[4][3] = {
        {84, 130, 196},   // внешние стенки
        {232, 140, 52},   // стенки отверстия
        {98, 178, 126},   // передняя крышка
        {164, 128, 206},  // задняя крышка
    };
    FrameStats st;
    const uint32_t wireCol = rgba(40, 60, 90);

    for (size_t t = 0; t < mesh.triangles.size(); ++t) {
        const auto& tri = mesh.triangles[t];
        const Vec3 a = world[tri[0]], b = world[tri[1]], c = world[tri[2]];
        const Vec3 n = faceNormal(a, b, c, det);

        // Back-Face Culling: N · V_cam ≤ 0 — грань повёрнута от наблюдателя
        if (opt.cull && m3d::dot(n, v.directionToViewer(a)) <= 0) {
            ++st.culled;
            continue;
        }

        if (!opt.fill) {  // каркасный режим
            drawLine3D(fb, v, a, b, wireCol, 0, 0, 1);
            drawLine3D(fb, v, b, c, wireCol, 0, 0, 1);
            drawLine3D(fb, v, c, a, wireCol, 0, 0, 1);
            ++st.drawn;
            continue;
        }

        // Плоское освещение по Ламберту: I = ambient + (1 − ambient) · max(0, N · L)
        const int g = opt.groupColors ? static_cast<int>(mesh.groups[t]) : 0;
        const int* base = kColors[g];
        double k = 1.0;
        if (opt.lighting) {
            const Vec3 center = (a + b + c) / 3.0;
            const Vec3 L = m3d::normalize(opt.light - center);
            k = opt.ambient + (1.0 - opt.ambient) * std::max(0.0, m3d::dot(m3d::normalize(n), L));
        }
        const uint32_t col = shade(base[0], base[1], base[2], k);
        const uint32_t edgeCol = shade(base[0], base[1], base[2], k * 0.45);

        // Отсечение по ближней плоскости (в пространстве отсечения), затем веер треугольников
        ClipVtx in[3] = {{v.projView * m3d::toVec4(a)}, {v.projView * m3d::toVec4(b)}, {v.projView * m3d::toVec4(c)}};
        ClipVtx poly[4];
        bool polyEdge[4];  // ребро poly[i] -> poly[i+1] — исходное ребро треугольника
        int cnt = 0;
        for (int i = 0; i < 3; ++i) {
            const ClipVtx& p = in[i];
            const ClipVtx& q = in[(i + 1) % 3];
            const double dp = nearDist(p.c), dq = nearDist(q.c);
            if (dp >= 0) {
                poly[cnt] = p;
                polyEdge[cnt++] = true;  // от p идёт (часть) исходного ребра pq
                if (dq < 0) {
                    poly[cnt] = {lerp4(p.c, q.c, dp / (dp - dq))};
                    polyEdge[cnt++] = false;  // дальше — новое ребро по ближней плоскости
                }
            } else if (dq >= 0) {
                poly[cnt] = {lerp4(p.c, q.c, dp / (dp - dq))};
                polyEdge[cnt] = true;
                ++cnt;
            }
        }
        if (cnt < 3) continue;
        ScreenVtx s[4];
        for (int i = 0; i < cnt; ++i) s[i] = toScreen(v, poly[i].c);
        for (int i = 1; i + 1 < cnt; ++i) {
            int mask = 0;
            if (opt.edges) {
                if (i == 1 && polyEdge[0]) mask |= 1;           // 0 -> 1
                if (polyEdge[i]) mask |= 2;                      // i -> i+1
                if (i + 1 == cnt - 1 && polyEdge[cnt - 1]) mask |= 4;  // last -> 0
            }
            rasterTriangle(fb, s[0], s[i], s[i + 1], col, edgeCol, mask, opt.zbuffer);
        }
        ++st.drawn;
    }
    return st;
}

void drawLine3D(Framebuffer& fb, const View& v, Vec3 a, Vec3 b, uint32_t color, uint32_t hiddenColor,
                double dash, int thickness) {
    Vec4 ca = v.projView * m3d::toVec4(a), cb = v.projView * m3d::toVec4(b);
    const double da = nearDist(ca), db = nearDist(cb);
    if (da < 0 && db < 0) return;
    if (da < 0) ca = lerp4(ca, cb, da / (da - db));
    if (db < 0) cb = lerp4(cb, ca, db / (db - da));
    const ScreenVtx p = toScreen(v, ca), q = toScreen(v, cb);
    const double dx = q.x - p.x, dy = q.y - p.y;
    const double len = std::hypot(dx, dy);
    const int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(dx), std::fabs(dy)))));
    if (steps > 20000) return;  // защита от вырожденных проекций
    const bool steep = std::fabs(dy) > std::fabs(dx);
    for (int i = 0; i <= steps; ++i) {
        const double t = static_cast<double>(i) / steps;
        if (dash > 0 && static_cast<long long>(std::floor(t * len / dash)) % 2 == 1) continue;  // пунктир
        const double x = p.x + dx * t, y = p.y + dy * t, z = p.z + (q.z - p.z) * t;
        const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
        if (ix < -thickness || iy < -thickness || ix >= fb.width + thickness || iy >= fb.height + thickness) continue;
        bool hidden = false;
        if (ix >= 0 && iy >= 0 && ix < fb.width && iy < fb.height)
            hidden = z > fb.depth[static_cast<size_t>(iy) * fb.width + ix];
        const uint32_t col = hidden ? hiddenColor : color;
        const double alpha = ((col >> 24) & 255) / 255.0;
        if (alpha <= 0) continue;
        for (int k = 0; k < thickness; ++k) {
            const int ox = steep ? k - thickness / 2 : 0, oy = steep ? 0 : k - thickness / 2;
            putPixel(fb, ix + ox, iy + oy, col, alpha);
        }
    }
}

Vec3 OrbitCamera::eye() const {
    const double y = m3d::deg2rad(yawDeg), p = m3d::deg2rad(pitchDeg);
    return target + Vec3{std::cos(p) * std::sin(y), std::sin(p), std::cos(p) * std::cos(y)} * distance;
}

View OrbitCamera::makeView(int width, int height) const {
    View v;
    v.kind = ViewKind::Perspective;
    v.width = std::max(1, width);
    v.height = std::max(1, height);
    v.perspective = true;
    v.eye = eye();
    v.view = m3d::lookAt(v.eye, target, {0, 1, 0});
    v.proj = m3d::perspective(fovDeg, static_cast<double>(v.width) / v.height, zNear, zFar);
    v.projView = v.proj * v.view;
    return v;
}

View makeOrthoView(ViewKind kind, int width, int height, const std::vector<Vec3>& fit) {
    View v;
    v.kind = kind;
    v.width = std::max(1, width);
    v.height = std::max(1, height);
    v.perspective = false;
    Vec3 up;
    switch (kind) {
        case ViewKind::Top:  // вид сверху: проекция на Oxz, X вправо, Z вниз
            v.toViewer = {0, 1, 0}; up = {0, 0, -1};
            v.hAxis = 0; v.hSign = 1; v.vAxis = 2; v.vSign = -1;
            break;
        case ViewKind::Side:  // вид слева: проекция на Oyz, Z вправо, Y вверх
            v.toViewer = {-1, 0, 0}; up = {0, 1, 0};
            v.hAxis = 2; v.hSign = 1; v.vAxis = 1; v.vSign = 1;
            break;
        default:  // вид спереди: проекция на Oxy, X вправо, Y вверх
            v.toViewer = {0, 0, 1}; up = {0, 1, 0};
            v.hAxis = 0; v.hSign = 1; v.vAxis = 1; v.vSign = 1;
            break;
    }
    auto comp = [](Vec3 p, int i) { return i == 0 ? p.x : (i == 1 ? p.y : p.z); };
    double hMin = 1e300, hMax = -1e300, vMin = 1e300, vMax = -1e300;
    for (const auto& p : fit) {
        const double h = comp(p, v.hAxis) * v.hSign, w = comp(p, v.vAxis) * v.vSign;
        hMin = std::min(hMin, h); hMax = std::max(hMax, h);
        vMin = std::min(vMin, w); vMax = std::max(vMax, w);
    }
    if (fit.empty()) { hMin = vMin = -100; hMax = vMax = 100; }
    const double rangeH = std::max(hMax - hMin, 1e-6), rangeV = std::max(vMax - vMin, 1e-6);
    const double ppu = std::min(v.width / (rangeH * 1.18), v.height / (rangeV * 1.18));  // одинаковый масштаб по осям
    const double cH = (hMin + hMax) / 2, cV = (vMin + vMax) / 2;
    const double halfW = v.width / ppu / 2, halfH = v.height / ppu / 2;

    // Центр в мировых координатах (на плоскости вида) и камера на расстоянии D
    Vec3 center;
    auto setComp = [](Vec3& p, int i, double val) { (i == 0 ? p.x : (i == 1 ? p.y : p.z)) = val; };
    setComp(center, v.hAxis, cH * v.hSign);
    setComp(center, v.vAxis, cV * v.vSign);
    const double D = 10000;
    v.eye = center + v.toViewer * D;
    v.view = m3d::lookAt(v.eye, center, up);
    v.proj = m3d::orthographic(-halfW, halfW, -halfH, halfH, 1.0, 2.0 * D);
    v.projView = v.proj * v.view;
    v.pixelsPerUnit = ppu;
    v.originX = v.width / 2.0 - cH * ppu;
    v.originY = v.height / 2.0 + cV * ppu;
    return v;
}

}  // namespace render
