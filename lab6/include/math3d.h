#pragma once

#include <array>
#include <string>

namespace m3d {

constexpr double kPi = 3.14159265358979323846;

inline double deg2rad(double d) { return d * kPi / 180.0; }
inline double rad2deg(double r) { return r * 180.0 / kPi; }

struct Vec3 {
    double x = 0, y = 0, z = 0;
};

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
Vec3 cross(Vec3 a, Vec3 b);
double length(Vec3 a);
Vec3 normalize(Vec3 a);

Vec4 toVec4(Vec3 p, double w = 1.0);
Vec3 toVec3(Vec4 p);
Vec3 perspectiveDivide(Vec4 p);

struct Mat4 {
    std::array<std::array<double, 4>, 4> m{};

    static Mat4 identity();
    double& operator()(int r, int c) { return m[r][c]; }
    double operator()(int r, int c) const { return m[r][c]; }
};

Mat4 operator*(const Mat4& a, const Mat4& b);
Vec4 operator*(const Mat4& a, Vec4 v);
Vec3 transformPoint(const Mat4& a, Vec3 p);
Vec3 transformDirection(const Mat4& a, Vec3 d);

double det3(const Mat4& a);
double maxAbsDiff(const Mat4& a, const Mat4& b);

Mat4 translation(double dx, double dy, double dz);
Mat4 translation(Vec3 d);
Mat4 scaling(double sx, double sy, double sz);
Mat4 rotationX(double deg);
Mat4 rotationY(double deg);
Mat4 rotationZ(double deg);

Mat4 rotationAxisOrigin(Vec3 u, double deg);

Mat4 rotationAroundLine(Vec3 p1, Vec3 p2, double deg);

Mat4 rotationAroundLineComposite(Vec3 p1, Vec3 p2, double deg);

Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up);
Mat4 perspective(double fovYdeg, double aspect, double zNear, double zFar);
Mat4 orthographic(double l, double r, double b, double t, double n, double f);

std::string toString(const Mat4& a, int precision = 3);

}
