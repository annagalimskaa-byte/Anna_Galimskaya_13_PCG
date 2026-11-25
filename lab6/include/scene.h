#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "math3d.h"
#include "mesh.h"
#include "renderer.h"

struct Scene {
    mesh::Mesh mesh;
    mesh::Stats stats;
    m3d::Mat4 model = m3d::Mat4::identity();
    std::vector<m3d::Vec3> world;
    std::vector<std::string> history;

    m3d::Vec3 p1{80, -120, 20};
    m3d::Vec3 p2{80, 120, 60};

    render::OrbitCamera camera;
    render::Options options;
    bool showAxisLine = true;
    bool showWorldAxes = true;

    void generate(int seed, int layers) {
        mesh = mesh::generate(seed, layers);
        stats = mesh::computeStats(mesh, model);
        updateWorld();
    }

    void apply(const m3d::Mat4& op, const std::string& description) {
        model = op * model;
        history.push_back(description);
        updateWorld();
    }

    void applySilently(const m3d::Mat4& op) {
        model = op * model;
        updateWorld();
    }

    void resetTransform() {
        model = m3d::Mat4::identity();
        history.clear();
        updateWorld();
    }

    void updateWorld() {
        world = render::transformVertices(mesh, model);
        mesh::computeChecksum(mesh, model, stats.centroid, stats.area);
        stats.det = m3d::det3(model);
    }

    std::vector<m3d::Vec3> fitPoints() const {
        std::vector<m3d::Vec3> pts{{0, 0, 0}, p1, p2};
        const m3d::Vec3 c = stats.centroid;
        double r = 0;
        for (const auto& p : world) r = std::max(r, m3d::length(p - c));
        for (const m3d::Vec3 d : {m3d::Vec3{1, 0, 0}, m3d::Vec3{0, 1, 0}, m3d::Vec3{0, 0, 1}}) {
            pts.push_back(c + d * r);
            pts.push_back(c - d * r);
        }
        return pts;
    }

    void resetCamera() {
        camera = render::OrbitCamera{};
        camera.target = {0, 0, mesh.params.H / 2};
    }
};
