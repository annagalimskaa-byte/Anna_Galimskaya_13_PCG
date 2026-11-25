#pragma once
// Программный конвейер: P·V·M·v -> отсечение по ближней плоскости -> деление на w ->
// NDC [-1, 1] -> пиксели -> растеризация с Z-буфером и плоским освещением по Ламберту.

#include <cstdint>
#include <vector>

#include "math3d.h"
#include "mesh.h"

namespace render {

// Цвет 0xAARRGGBB (совпадает с QImage::Format_ARGB32). Альфа 0 — пиксель не закрашен.
inline uint32_t rgba(int r, int g, int b, int a = 255) {
    return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

struct Framebuffer {
    int width = 0, height = 0;
    std::vector<uint32_t> color;
    std::vector<float> depth;  // Z-буфер: значение NDC z, меньше — ближе

    void resize(int w, int h);
    void clear();
};

enum class ViewKind { Perspective, Front, Top, Side };

// Параметры одного окна просмотра
struct View {
    ViewKind kind = ViewKind::Perspective;
    m3d::Mat4 view;        // матрица вида
    m3d::Mat4 proj;        // матрица проекции
    m3d::Mat4 projView;    // P · V
    int width = 1, height = 1;
    m3d::Vec3 eye;         // положение камеры (перспектива)
    m3d::Vec3 toViewer;    // направление на наблюдателя (ортография)
    bool perspective = true;

    // Для ортографических видов: экранные оси (для сетки и подписей)
    int hAxis = 0, vAxis = 1;    // индексы мировых осей (0 = x, 1 = y, 2 = z)
    double hSign = 1, vSign = 1; // направление оси на экране (+1: вправо / вверх)
    double pixelsPerUnit = 1;
    double originX = 0, originY = 0;  // экранные координаты начала координат

    // Вектор из точки p на наблюдателя (для отсечения нелицевых граней)
    m3d::Vec3 directionToViewer(m3d::Vec3 p) const;
};

struct ScreenPoint {
    double x = 0, y = 0, z = 0;  // пиксели и NDC z
    bool visible = false;        // false — точка за ближней плоскостью
};

ScreenPoint project(const View& v, m3d::Vec3 world);

struct Options {
    bool fill = true;          // заливка граней (иначе каркас)
    bool edges = true;         // рёбра поверх заливки
    bool cull = true;          // Back-Face Culling
    bool zbuffer = true;       // Z-буфер
    bool lighting = true;      // Ламберт (иначе цвет без освещения)
    bool groupColors = true;   // разные цвета для стенок, отверстия и крышек
    m3d::Vec3 light{220, 260, 340};  // точечный источник света (мировые координаты)
    double ambient = 0.2;
};

struct FrameStats {
    int drawn = 0;   // треугольников растеризовано
    int culled = 0;  // отброшено Back-Face Culling
};

// Мировые координаты вершин после матрицы моделирования (с делением на w)
std::vector<m3d::Vec3> transformVertices(const mesh::Mesh& mesh, const m3d::Mat4& model);

// Нормаль грани (не нормированная). При det(M) < 0 порядок обхода инвертирован —
// нормаль разворачивается, чтобы не «выворачивать» деталь наизнанку.
m3d::Vec3 faceNormal(m3d::Vec3 a, m3d::Vec3 b, m3d::Vec3 c, double det);

FrameStats drawMesh(Framebuffer& fb, const View& v, const mesh::Mesh& mesh,
                    const std::vector<m3d::Vec3>& world, double det, const Options& opt);

// Отрезок в 3D с проверкой глубины. hiddenColor с альфой 0 — скрытые части не рисуются.
void drawLine3D(Framebuffer& fb, const View& v, m3d::Vec3 a, m3d::Vec3 b, uint32_t color,
                uint32_t hiddenColor, double dash, int thickness);

// --- Камеры ---
struct OrbitCamera {
    m3d::Vec3 target{0, 0, 30};
    double yawDeg = 35, pitchDeg = 25, distance = 380;
    double fovDeg = 50, zNear = 1, zFar = 5000;

    m3d::Vec3 eye() const;
    View makeView(int width, int height) const;
};

// Ортографический вид с автоматическим центрированием и масштабом по точкам fit
View makeOrthoView(ViewKind kind, int width, int height, const std::vector<m3d::Vec3>& fit);

}  // namespace render
