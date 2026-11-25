#include <cstdio>
#include <cstdlib>
#include "mesh.h"
#include "renderer.h"
using namespace render;
int main(int argc, char** argv) {
    int seed = argc > 1 ? std::atoi(argv[1]) : 42;
    const char* out = argc > 2 ? argv[2] : "demo.ppm";
    double sx = argc > 3 ? std::atof(argv[3]) : 1.0;
    int layers = argc > 4 ? std::atoi(argv[4]) : 1;
    auto m = mesh::generate(seed, layers);
    m3d::Vec3 p1{80, -120, 20}, p2{80, 120, 60};
    m3d::Mat4 model = m3d::rotationAroundLine(p1, p2, 30) * m3d::scaling(sx, 1, 1);
    auto world = transformVertices(m, model);
    double det = m3d::det3(model);
    const int W = 480, H = 360;
    std::vector<uint32_t> img(4 * W * H, 0xFFFFFFFF);
    std::vector<m3d::Vec3> fit = world; fit.push_back({0,0,0}); fit.push_back(p1); fit.push_back(p2);
    View views[4] = {OrbitCamera{}.makeView(W, H), makeOrthoView(ViewKind::Front, W, H, fit),
                     makeOrthoView(ViewKind::Top, W, H, fit), makeOrthoView(ViewKind::Side, W, H, fit)};
    Options opt;
    for (int k = 0; k < 4; ++k) {
        Framebuffer fb; fb.resize(W, H);
        auto st = drawMesh(fb, views[k], m, world, det, opt);
        drawLine3D(fb, views[k], p1 + (p1 - p2) * 0.4, p2 + (p2 - p1) * 0.4, rgba(220, 30, 140), rgba(220, 30, 140, 90), 7, 2);
        drawLine3D(fb, views[k], {0,0,0}, {120,0,0}, rgba(220,50,50), 0, 0, 1);
        drawLine3D(fb, views[k], {0,0,0}, {0,120,0}, rgba(40,160,60), 0, 0, 1);
        drawLine3D(fb, views[k], {0,0,0}, {0,0,120}, rgba(40,80,220), 0, 0, 1);
        std::fprintf(stderr, "view %d drawn %d culled %d\n", k, st.drawn, st.culled);
        int ox = (k % 2) * W, oy = (k / 2) * H;
        for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
            uint32_t c = fb.color[y * W + x];
            if (c >> 24) img[(oy + y) * 2 * W + ox + x] = c;
        }
    }
    FILE* f = std::fopen(out, "wb");
    std::fprintf(f, "P6 %d %d 255\n", 2 * W, 2 * H);
    for (auto c : img) { unsigned char px[3] = {(unsigned char)(c >> 16), (unsigned char)(c >> 8), (unsigned char)c}; std::fwrite(px, 1, 3, f); }
    std::fclose(f);
}
