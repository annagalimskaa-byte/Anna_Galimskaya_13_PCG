#pragma once
// Процедурная генерация невыпуклой детали со сквозным отверстием и винтовой закруткой.
// Модель хранится индексно: массив уникальных вершин [x, y, z, 1] + тройки/пары индексов.

#include <array>
#include <cstdint>
#include <vector>

#include "math3d.h"

namespace mesh {

// Генератор псевдослучайных чисел Random(Seed) — алгоритм Mulberry32.
// Тот же алгоритм в веб-версии, поэтому одинаковый Seed даёт одинаковую деталь.
class Random {
public:
    explicit Random(int32_t seed) : state_(static_cast<uint32_t>(seed)) {}

    double next() {  // [0, 1)
        state_ += 0x6D2B79F5u;
        uint32_t t = (state_ ^ (state_ >> 15)) * (1u | state_);
        t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
        return static_cast<double>(t ^ (t >> 14)) / 4294967296.0;
    }
    double uniform(double a, double b) { return a + (b - a) * next(); }
    int integer(int a, int b) { return a + static_cast<int>(next() * (b - a + 1)); }  // [a, b]

private:
    uint32_t state_;
};

struct Params {
    int seed = 0;
    int N = 6;              // число зубьев (лучей) внешнего контура, {5..8}
    int M = 4;              // число углов отверстия, {3..6}
    double rOut = 100;      // внешний радиус зубьев
    double rIn = 65;        // внутренний радиус зубьев, [55, 75]
    double rInscribed = 0;  // радиус вписанной окружности звезды (для подбора отверстия)
    double rHole = 20;      // радиус отверстия, < 0.6·Rin
    double dx = 0, dy = 0;  // смещение центра отверстия
    double H = 60;          // глубина выдавливания, [40, 80]
    double twistDeg = 30;   // угол закручивания, [15°, 45°]
    double taper = 0.85;    // коэффициент конусности, [0.75, 0.95]
    double outerPhaseDeg = 0;  // начальный угол внешнего контура
    double holePhaseDeg = 0;   // начальный угол отверстия
    int layers = 1;            // слоёв по высоте (детализация; по заданию = 1)
};

enum class FaceGroup : uint8_t { OuterWall = 0, HoleWall = 1, FrontCap = 2, BackCap = 3 };

struct Mesh {
    Params params;
    std::vector<m3d::Vec4> vertices;              // Vertices: уникальные вершины [x, y, z, 1]
    std::vector<std::array<int, 3>> triangles;    // Triangles: тройки индексов (обход CCW снаружи)
    std::vector<std::array<int, 2>> edges;        // Edges: уникальные рёбра (пары индексов)
    std::vector<FaceGroup> groups;                // тип каждой грани (для раскраски)
    int ringSize = 0;  // вершин в одном слое: 2N + M
};

Params paramsFromSeed(int seed, int layers = 1);

// Радиус вписанной окружности звёздчатого многоугольника (минимальное расстояние от центра до рёбер)
double starInscribedRadius(int n, double rOut, double rIn);

// 2D-контуры передней грани (Z = 0): внешний (2N вершин) и отверстие (M вершин), оба против часовой
std::vector<m3d::Vec3> outerContour(const Params& p);
std::vector<m3d::Vec3> holeContour(const Params& p);

// Триангуляция многоугольника с одной дыркой методом «отрезания ушей» (Ear Clipping).
// outer и hole заданы против часовой стрелки; индексы результата: 0..n-1 — outer, n..n+m-1 — hole.
// Возвращаемые треугольники ориентированы против часовой стрелки (в плоскости Oxy).
std::vector<std::array<int, 3>> triangulateWithHole(const std::vector<m3d::Vec3>& outer,
                                                    const std::vector<m3d::Vec3>& hole);

Mesh buildMesh(const Params& p);
Mesh generate(int seed, int layers = 1);

// --- Самодиагностика ---
struct Stats {
    int V = 0, E = 0, F = 0;
    int euler = 0;            // V − E + F
    bool closedManifold = false;  // каждое ребро принадлежит ровно двум граням с противоположным обходом
    m3d::Vec3 centroid;       // центр масс (среднее арифметическое вершин после M)
    double area = 0;          // площадь поверхности после M
    double det = 1;           // det левой верхней 3x3 матрицы M
};

Stats computeStats(const Mesh& mesh, const m3d::Mat4& model);
// Только контрольный маркер (центр масс и площадь) — дёшево, считается на каждом кадре
void computeChecksum(const Mesh& mesh, const m3d::Mat4& model, m3d::Vec3& centroid, double& area);
bool isClosedManifold(const Mesh& mesh);

}  // namespace mesh
