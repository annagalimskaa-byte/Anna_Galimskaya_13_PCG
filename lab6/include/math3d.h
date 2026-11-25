#pragma once
// Векторы и матрицы 4x4, написанные вручную (без готовых математических классов).
// Соглашение: векторы-столбцы, v' = M * v. Новое преобразование умножается слева:
// M_new = Op * M_old (операции задаются в мировых координатах).

#include <array>
#include <string>

namespace m3d {

constexpr double kPi = 3.14159265358979323846;

inline double deg2rad(double d) { return d * kPi / 180.0; }
inline double rad2deg(double r) { return r * 180.0 / kPi; }

struct Vec3 {
    double x = 0, y = 0, z = 0;
};

// Однородные координаты [x, y, z, w]
struct Vec4 {
    double x = 0, y = 0, z = 0, w = 1;
};

Vec3 operator+(Vec3 a, Vec3 b);
Vec3 operator-(Vec3 a, Vec3 b);
Vec3 operator-(Vec3 a);
Vec3 operator*(Vec3 a, double k);
Vec3 operator*(double k, Vec3 a);
Vec3 operator/(Vec3 a, double k);

double dot(Vec3 a, Vec3 b);
Vec3 cross(Vec3 a, Vec3 b);  // векторное произведение
double length(Vec3 a);
Vec3 normalize(Vec3 a);

Vec4 toVec4(Vec3 p, double w = 1.0);
Vec3 toVec3(Vec4 p);  // без деления на w
Vec3 perspectiveDivide(Vec4 p);  // (x/w, y/w, z/w)

struct Mat4 {
    std::array<std::array<double, 4>, 4> m{};  // m[row][col]

    static Mat4 identity();
    double& operator()(int r, int c) { return m[r][c]; }
    double operator()(int r, int c) const { return m[r][c]; }
};

Mat4 operator*(const Mat4& a, const Mat4& b);
Vec4 operator*(const Mat4& a, Vec4 v);
Vec3 transformPoint(const Mat4& a, Vec3 p);      // с делением на w
Vec3 transformDirection(const Mat4& a, Vec3 d);  // w = 0

double det3(const Mat4& a);  // определитель левой верхней 3x3 (знак < 0 — зеркальное отражение)
double maxAbsDiff(const Mat4& a, const Mat4& b);

// --- Базовые аффинные преобразования ---
Mat4 translation(double dx, double dy, double dz);
Mat4 translation(Vec3 d);
Mat4 scaling(double sx, double sy, double sz);
Mat4 rotationX(double deg);
Mat4 rotationY(double deg);
Mat4 rotationZ(double deg);

// Поворот на угол deg вокруг единичного вектора u, проходящего через начало координат (матрица Родрига)
Mat4 rotationAxisOrigin(Vec3 u, double deg);

// Поворот вокруг оси P1P2 (не обязательно через начало координат):
// T(P1) * R_u(theta) * T(-P1), u = (P2 - P1) / |P2 - P1|
Mat4 rotationAroundLine(Vec3 p1, Vec3 p2, double deg);

// То же самое, но через композицию: перенос P1 в начало, совмещение оси с Oz
// поворотами вокруг Ox и Oy, поворот вокруг Oz, обратные преобразования.
Mat4 rotationAroundLineComposite(Vec3 p1, Vec3 p2, double deg);

// --- Камера ---
// Матрица вида LookAt: ортонормированный базис камеры (Right, Up, Forward).
Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up);
// Перспективная проекция (fovY в градусах). NDC: x, y, z в [-1, 1], z = -1 на near.
Mat4 perspective(double fovYdeg, double aspect, double zNear, double zFar);
// Ортографическая проекция
Mat4 orthographic(double l, double r, double b, double t, double n, double f);

std::string toString(const Mat4& a, int precision = 3);

}  // namespace m3d
