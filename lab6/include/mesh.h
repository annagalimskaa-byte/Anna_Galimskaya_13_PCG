#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "math3d.h"

namespace mesh {

class Random {
public:
    explicit Random(int32_t seed) : state_(static_cast<uint32_t>(seed)) {}

    double next() {
        state_ += 0x6D2B79F5u;
        uint32_t t = (state_ ^ (state_ >> 15)) * (1u | state_);
        t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
        return static_cast<double>(t ^ (t >> 14)) / 4294967296.0;
    }
    double uniform(double a, double b) { return a + (b - a) * next(); }
    int integer(int a, int b) { return a + static_cast<int>(next() * (b - a + 1)); }

private:
    uint32_t state_;
};

struct Params {
    int seed = 0;
    int N = 6;
    int M = 4;
    double rOut = 100;
    double rIn = 65;
    double rInscribed = 0;
    double rHole = 20;
    double dx = 0, dy = 0;
    double H = 60;
    double twistDeg = 30;
    double taper = 0.85;
    double outerPhaseDeg = 0;
    double holePhaseDeg = 0;
    int layers = 1;
};

enum class FaceGroup : uint8_t { OuterWall = 0, HoleWall = 1, FrontCap = 2, BackCap = 3 };

struct Mesh {
    Params params;
    std::vector<m3d::Vec4> vertices;
    std::vector<std::array<int, 3>> triangles;
    std::vector<std::array<int, 2>> edges;
    std::vector<FaceGroup> groups;
    int ringSize = 0;
};

Params paramsFromSeed(int seed, int layers = 1);

double starInscribedRadius(int n, double rOut, double rIn);

std::vector<m3d::Vec3> outerContour(const Params& p);
std::vector<m3d::Vec3> holeContour(const Params& p);

std::vector<std::array<int, 3>> triangulateWithHole(const std::vector<m3d::Vec3>& outer,
                                                    const std::vector<m3d::Vec3>& hole);

Mesh buildMesh(const Params& p);
Mesh generate(int seed, int layers = 1);

struct Stats {
    int V = 0, E = 0, F = 0;
    int euler = 0;
    bool closedManifold = false;
    m3d::Vec3 centroid;
    double area = 0;
    double det = 1;
};

Stats computeStats(const Mesh& mesh, const m3d::Mat4& model);
void computeChecksum(const Mesh& mesh, const m3d::Mat4& model, m3d::Vec3& centroid, double& area);
bool isClosedManifold(const Mesh& mesh);

}
