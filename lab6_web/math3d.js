// Векторы и матрицы 4x4, написанные вручную (порт lab6/src/math3d.cpp).
// Векторы-столбцы: v' = M · v. Матрица хранится построчно: m[r * 4 + c].
'use strict';

const M3D = (() => {
  const kPi = Math.PI;
  const deg2rad = (d) => d * kPi / 180;
  const rad2deg = (r) => r * 180 / kPi;

  const vec3 = (x = 0, y = 0, z = 0) => ({ x, y, z });
  const add = (a, b) => vec3(a.x + b.x, a.y + b.y, a.z + b.z);
  const sub = (a, b) => vec3(a.x - b.x, a.y - b.y, a.z - b.z);
  const mul = (a, k) => vec3(a.x * k, a.y * k, a.z * k);
  const neg = (a) => vec3(-a.x, -a.y, -a.z);
  const dot = (a, b) => a.x * b.x + a.y * b.y + a.z * b.z;
  const cross = (a, b) => vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
  const length = (a) => Math.sqrt(dot(a, a));
  const normalize = (a) => { const l = length(a); return l > 1e-12 ? mul(a, 1 / l) : vec3(); };

  // Однородные координаты [x, y, z, w]
  const vec4 = (x = 0, y = 0, z = 0, w = 1) => ({ x, y, z, w });
  const toVec4 = (p, w = 1) => vec4(p.x, p.y, p.z, w);
  const toVec3 = (p) => vec3(p.x, p.y, p.z);
  const perspectiveDivide = (p) => vec3(p.x / p.w, p.y / p.w, p.z / p.w);

  function zero() { return new Float64Array(16); }
  function identity() { const r = zero(); r[0] = r[5] = r[10] = r[15] = 1; return r; }

  function mulMat(a, b) {
    const r = zero();
    for (let i = 0; i < 4; i++)
      for (let j = 0; j < 4; j++) {
        let s = 0;
        for (let k = 0; k < 4; k++) s += a[i * 4 + k] * b[k * 4 + j];
        r[i * 4 + j] = s;
      }
    return r;
  }
  // Произведение нескольких матриц слева направо: chain(A, B, C) = A·B·C
  const chain = (...ms) => ms.reduce((acc, m) => mulMat(acc, m));

  function mulVec(a, v) {
    return vec4(
      a[0] * v.x + a[1] * v.y + a[2] * v.z + a[3] * v.w,
      a[4] * v.x + a[5] * v.y + a[6] * v.z + a[7] * v.w,
      a[8] * v.x + a[9] * v.y + a[10] * v.z + a[11] * v.w,
      a[12] * v.x + a[13] * v.y + a[14] * v.z + a[15] * v.w);
  }
  function transformPoint(a, p) {
    const r = mulVec(a, toVec4(p, 1));
    return Math.abs(r.w) > 1e-15 ? perspectiveDivide(r) : toVec3(r);
  }
  const transformDirection = (a, d) => toVec3(mulVec(a, toVec4(d, 0)));

  function det3(m) {
    return m[0] * (m[5] * m[10] - m[6] * m[9]) -
           m[1] * (m[4] * m[10] - m[6] * m[8]) +
           m[2] * (m[4] * m[9] - m[5] * m[8]);
  }
  function maxAbsDiff(a, b) {
    let d = 0;
    for (let i = 0; i < 16; i++) d = Math.max(d, Math.abs(a[i] - b[i]));
    return d;
  }

  // --- Базовые аффинные преобразования ---
  function translation(dx, dy, dz) {
    if (typeof dx === 'object') ({ x: dx, y: dy, z: dz } = dx);
    const r = identity(); r[3] = dx; r[7] = dy; r[11] = dz; return r;
  }
  function scaling(sx, sy, sz) { const r = identity(); r[0] = sx; r[5] = sy; r[10] = sz; return r; }
  function rotationX(deg) {
    const c = Math.cos(deg2rad(deg)), s = Math.sin(deg2rad(deg)), r = identity();
    r[5] = c; r[6] = -s; r[9] = s; r[10] = c; return r;
  }
  function rotationY(deg) {
    const c = Math.cos(deg2rad(deg)), s = Math.sin(deg2rad(deg)), r = identity();
    r[0] = c; r[2] = s; r[8] = -s; r[10] = c; return r;
  }
  function rotationZ(deg) {
    const c = Math.cos(deg2rad(deg)), s = Math.sin(deg2rad(deg)), r = identity();
    r[0] = c; r[1] = -s; r[4] = s; r[5] = c; return r;
  }

  // Матрица Родрига: R = cos θ·E + (1 − cos θ)·u·uᵀ + sin θ·[u]×
  function rotationAxisOrigin(u, deg) {
    u = normalize(u);
    const c = Math.cos(deg2rad(deg)), s = Math.sin(deg2rad(deg)), t = 1 - c;
    const { x, y, z } = u, r = identity();
    r[0] = c + t * x * x;     r[1] = t * x * y - s * z; r[2] = t * x * z + s * y;
    r[4] = t * x * y + s * z; r[5] = c + t * y * y;     r[6] = t * y * z - s * x;
    r[8] = t * x * z - s * y; r[9] = t * y * z + s * x; r[10] = c + t * z * z;
    return r;
  }
  // Поворот вокруг оси P1P2: T(P1) · R_u(θ) · T(−P1)
  function rotationAroundLine(p1, p2, deg) {
    const u = normalize(sub(p2, p1));
    return chain(translation(p1), rotationAxisOrigin(u, deg), translation(neg(p1)));
  }
  // То же через совмещение оси с Oz: T(P1)·Rx⁻¹·Ry⁻¹·Rz(θ)·Ry·Rx·T(−P1)
  function rotationAroundLineComposite(p1, p2, deg) {
    const u = normalize(sub(p2, p1)), a = u.x, b = u.y, c = u.z;
    const d = Math.sqrt(b * b + c * c);
    const rx = identity();
    if (d > 1e-12) { rx[5] = c / d; rx[6] = -b / d; rx[9] = b / d; rx[10] = c / d; }
    const ry = identity();
    ry[0] = d; ry[2] = -a; ry[8] = a; ry[10] = d;
    const transpose = (m) => { const r = identity(); for (let i = 0; i < 3; i++) for (let j = 0; j < 3; j++) r[i * 4 + j] = m[j * 4 + i]; return r; };
    return chain(translation(p1), transpose(rx), transpose(ry), rotationZ(deg), ry, rx, translation(neg(p1)));
  }

  // --- Камера ---
  function lookAt(eye, target, up) {
    const f = normalize(sub(target, eye));   // Forward
    const r = normalize(cross(f, up));       // Right
    const u = cross(r, f);                   // Up
    const v = identity();
    v[0] = r.x;  v[1] = r.y;  v[2] = r.z;  v[3] = -dot(r, eye);
    v[4] = u.x;  v[5] = u.y;  v[6] = u.z;  v[7] = -dot(u, eye);
    v[8] = -f.x; v[9] = -f.y; v[10] = -f.z; v[11] = dot(f, eye);
    return v;
  }
  function perspective(fovYdeg, aspect, zNear, zFar) {
    const f = 1 / Math.tan(deg2rad(fovYdeg) / 2), p = zero();
    p[0] = f / aspect;
    p[5] = f;
    p[10] = (zFar + zNear) / (zNear - zFar);
    p[11] = 2 * zFar * zNear / (zNear - zFar);
    p[14] = -1;
    return p;
  }
  function orthographic(l, r, b, t, n, f) {
    const o = identity();
    o[0] = 2 / (r - l);  o[3] = -(r + l) / (r - l);
    o[5] = 2 / (t - b);  o[7] = -(t + b) / (t - b);
    o[10] = -2 / (f - n); o[11] = -(f + n) / (f - n);
    return o;
  }

  return {
    kPi, deg2rad, rad2deg, vec3, add, sub, mul, neg, dot, cross, length, normalize,
    vec4, toVec4, toVec3, perspectiveDivide, identity, mulMat, chain, mulVec, transformPoint,
    transformDirection, det3, maxAbsDiff, translation, scaling, rotationX, rotationY, rotationZ,
    rotationAxisOrigin, rotationAroundLine, rotationAroundLineComposite, lookAt, perspective, orthographic,
  };
})();

if (typeof module !== 'undefined') module.exports = M3D;
