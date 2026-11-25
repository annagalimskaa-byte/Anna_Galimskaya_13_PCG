#pragma once

#include <cstdint>
#include <vector>

#include "math3d.h"
#include "mesh.h"

namespace render {

inline uint32_t rgba(int r, int g, int b, int a = 255) {
    return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

struct Framebuffer {
    int width = 0, height = 0;
    std::vector<uint32_t> color;
    std::vector<float> depth;

    void resize(int w, int h);
    void clear();
};

enum class ViewKind { Perspective, Front, Top, Side };

struct View {
    ViewKind kind = ViewKind::Perspective;
    m3d::Mat4 view;
    m3d::Mat4 proj;
    m3d::Mat4 projView;
    int width = 1, height = 1;
    m3d::Vec3 eye;
    m3d::Vec3 toViewer;
    bool perspective = true;

    int hAxis = 0, vAxis = 1;
    double hSign = 1, vSign = 1;
    double pixelsPerUnit = 1;
    double originX = 0, originY = 0;

    m3d::Vec3 directionToViewer(m3d::Vec3 p) const;
};

struct ScreenPoint {
    double x = 0, y = 0, z = 0;
    bool visible = false;
};

ScreenPoint project(const View& v, m3d::Vec3 world);

struct Options {
    bool fill = true;
    bool edges = true;
    bool cull = true;
    bool zbuffer = true;
    bool lighting = true;
    bool groupColors = true;
    m3d::Vec3 light{220, 260, 340};
    double ambient = 0.2;
};

struct FrameStats {
    int drawn = 0;
    int culled = 0;
};

std::vector<m3d::Vec3> transformVertices(const mesh::Mesh& mesh, const m3d::Mat4& model);

m3d::Vec3 faceNormal(m3d::Vec3 a, m3d::Vec3 b, m3d::Vec3 c, double det);

FrameStats drawMesh(Framebuffer& fb, const View& v, const mesh::Mesh& mesh,
                    const std::vector<m3d::Vec3>& world, double det, const Options& opt);

void drawLine3D(Framebuffer& fb, const View& v, m3d::Vec3 a, m3d::Vec3 b, uint32_t color,
                uint32_t hiddenColor, double dash, int thickness);

struct OrbitCamera {
    m3d::Vec3 target{0, 0, 30};
    double yawDeg = 35, pitchDeg = 25, distance = 380;
    double fovDeg = 50, zNear = 1, zFar = 5000;

    m3d::Vec3 eye() const;
    View makeView(int width, int height) const;
};

View makeOrthoView(ViewKind kind, int width, int height, const std::vector<m3d::Vec3>& fit);

}
