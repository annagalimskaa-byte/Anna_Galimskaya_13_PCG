#include "math3d.h"

#include <cmath>
#include <cstdio>

namespace m3d {

Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
Vec3 operator*(Vec3 a, double k) { return {a.x * k, a.y * k, a.z * k}; }
Vec3 operator*(double k, Vec3 a) { return a * k; }
Vec3 operator/(Vec3 a, double k) { return {a.x / k, a.y / k, a.z / k}; }

double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

double length(Vec3 a) { return std::sqrt(dot(a, a)); }

Vec3 normalize(Vec3 a) {
    const double len = length(a);
    return len > 1e-12 ? a / len : Vec3{0, 0, 0};
}

Vec4 toVec4(Vec3 p, double w) { return {p.x, p.y, p.z, w}; }
Vec3 toVec3(Vec4 p) { return {p.x, p.y, p.z}; }
Vec3 perspectiveDivide(Vec4 p) { return {p.x / p.w, p.y / p.w, p.z / p.w}; }

Mat4 Mat4::identity() {
    Mat4 r;
    for (int i = 0; i < 4; ++i) r.m[i][i] = 1.0;
    return r;
}

Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) {
            double s = 0;
            for (int k = 0; k < 4; ++k) s += a.m[i][k] * b.m[k][j];
            r.m[i][j] = s;
        }
    return r;
}

Vec4 operator*(const Mat4& a, Vec4 v) {
    const double in[4] = {v.x, v.y, v.z, v.w};
    double out[4];
    for (int i = 0; i < 4; ++i)
        out[i] = a.m[i][0] * in[0] + a.m[i][1] * in[1] + a.m[i][2] * in[2] + a.m[i][3] * in[3];
    return {out[0], out[1], out[2], out[3]};
}

Vec3 transformPoint(const Mat4& a, Vec3 p) {
    const Vec4 r = a * toVec4(p, 1.0);
    return std::fabs(r.w) > 1e-15 ? perspectiveDivide(r) : toVec3(r);
}

Vec3 transformDirection(const Mat4& a, Vec3 d) { return toVec3(a * toVec4(d, 0.0)); }

double det3(const Mat4& a) {
    const auto& m = a.m;
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
           m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

double maxAbsDiff(const Mat4& a, const Mat4& b) {
    double d = 0;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) d = std::fmax(d, std::fabs(a.m[i][j] - b.m[i][j]));
    return d;
}

Mat4 translation(double dx, double dy, double dz) {
    Mat4 r = Mat4::identity();
    r.m[0][3] = dx;
    r.m[1][3] = dy;
    r.m[2][3] = dz;
    return r;
}

Mat4 translation(Vec3 d) { return translation(d.x, d.y, d.z); }

Mat4 scaling(double sx, double sy, double sz) {
    Mat4 r = Mat4::identity();
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    r.m[2][2] = sz;
    return r;
}

Mat4 rotationX(double deg) {
    const double c = std::cos(deg2rad(deg)), s = std::sin(deg2rad(deg));
    Mat4 r = Mat4::identity();
    r.m[1][1] = c;  r.m[1][2] = -s;
    r.m[2][1] = s;  r.m[2][2] = c;
    return r;
}

Mat4 rotationY(double deg) {
    const double c = std::cos(deg2rad(deg)), s = std::sin(deg2rad(deg));
    Mat4 r = Mat4::identity();
    r.m[0][0] = c;  r.m[0][2] = s;
    r.m[2][0] = -s; r.m[2][2] = c;
    return r;
}

Mat4 rotationZ(double deg) {
    const double c = std::cos(deg2rad(deg)), s = std::sin(deg2rad(deg));
    Mat4 r = Mat4::identity();
    r.m[0][0] = c;  r.m[0][1] = -s;
    r.m[1][0] = s;  r.m[1][1] = c;
    return r;
}

Mat4 rotationAxisOrigin(Vec3 u, double deg) {
    u = normalize(u);
    const double c = std::cos(deg2rad(deg)), s = std::sin(deg2rad(deg)), t = 1.0 - c;
    const double x = u.x, y = u.y, z = u.z;
    Mat4 r = Mat4::identity();
    r.m[0][0] = c + t * x * x;     r.m[0][1] = t * x * y - s * z; r.m[0][2] = t * x * z + s * y;
    r.m[1][0] = t * x * y + s * z; r.m[1][1] = c + t * y * y;     r.m[1][2] = t * y * z - s * x;
    r.m[2][0] = t * x * z - s * y; r.m[2][1] = t * y * z + s * x; r.m[2][2] = c + t * z * z;
    return r;
}

Mat4 rotationAroundLine(Vec3 p1, Vec3 p2, double deg) {
    const Vec3 u = normalize(p2 - p1);
    return translation(p1) * rotationAxisOrigin(u, deg) * translation(-p1);
}

Mat4 rotationAroundLineComposite(Vec3 p1, Vec3 p2, double deg) {
    const Vec3 u = normalize(p2 - p1);
    const double a = u.x, b = u.y, c = u.z;
    const double d = std::sqrt(b * b + c * c);

    Mat4 rx = Mat4::identity();
    if (d > 1e-12) {
        rx.m[1][1] = c / d;  rx.m[1][2] = -b / d;
        rx.m[2][1] = b / d;  rx.m[2][2] = c / d;
    }
    Mat4 ry = Mat4::identity();
    ry.m[0][0] = d;   ry.m[0][2] = -a;
    ry.m[2][0] = a;   ry.m[2][2] = d;

    Mat4 rxInv = rx, ryInv = ry;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            rxInv.m[i][j] = rx.m[j][i];
            ryInv.m[i][j] = ry.m[j][i];
        }
    return translation(p1) * rxInv * ryInv * rotationZ(deg) * ry * rx * translation(-p1);
}

Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    const Vec3 f = normalize(target - eye);
    const Vec3 r = normalize(cross(f, up));
    const Vec3 u = cross(r, f);
    Mat4 v = Mat4::identity();
    v.m[0][0] = r.x;  v.m[0][1] = r.y;  v.m[0][2] = r.z;  v.m[0][3] = -dot(r, eye);
    v.m[1][0] = u.x;  v.m[1][1] = u.y;  v.m[1][2] = u.z;  v.m[1][3] = -dot(u, eye);
    v.m[2][0] = -f.x; v.m[2][1] = -f.y; v.m[2][2] = -f.z; v.m[2][3] = dot(f, eye);
    return v;
}

Mat4 perspective(double fovYdeg, double aspect, double zNear, double zFar) {
    const double f = 1.0 / std::tan(deg2rad(fovYdeg) / 2.0);
    Mat4 p;
    p.m[0][0] = f / aspect;
    p.m[1][1] = f;
    p.m[2][2] = (zFar + zNear) / (zNear - zFar);
    p.m[2][3] = 2.0 * zFar * zNear / (zNear - zFar);
    p.m[3][2] = -1.0;
    return p;
}

Mat4 orthographic(double l, double r, double b, double t, double n, double f) {
    Mat4 o = Mat4::identity();
    o.m[0][0] = 2.0 / (r - l);  o.m[0][3] = -(r + l) / (r - l);
    o.m[1][1] = 2.0 / (t - b);  o.m[1][3] = -(t + b) / (t - b);
    o.m[2][2] = -2.0 / (f - n); o.m[2][3] = -(f + n) / (f - n);
    return o;
}

std::string toString(const Mat4& a, int precision) {
    std::string s;
    char buf[64];
    for (int i = 0; i < 4; ++i) {
        s += "| ";
        for (int j = 0; j < 4; ++j) {
            double v = a.m[i][j];
            if (std::fabs(v) < 0.5 * std::pow(10.0, -precision)) v = 0.0;
            std::snprintf(buf, sizeof buf, "%10.*f ", precision, v);
            s += buf;
        }
        s += "|\n";
    }
    return s;
}

}
