'use strict';

const MESH = (() => {
  const M = (typeof M3D !== 'undefined') ? M3D : require('./math3d.js');

  class Random {
    constructor(seed) { this.state = seed | 0; }
    next() {
      this.state = (this.state + 0x6D2B79F5) | 0;
      const a = this.state;
      let t = Math.imul(a ^ (a >>> 15), 1 | a);
      t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    }
    uniform(a, b) { return a + (b - a) * this.next(); }
    integer(a, b) { return a + Math.floor(this.next() * (b - a + 1)); }
  }

  const GROUP = { OuterWall: 0, HoleWall: 1, FrontCap: 2, BackCap: 3 };

  function starInscribedRadius(n, rOut, rIn) {
    const t = Math.PI / n;
    return rOut * rIn * Math.sin(t) / Math.sqrt(rOut * rOut + rIn * rIn - 2 * rOut * rIn * Math.cos(t));
  }

  function paramsFromSeed(seed, layers = 1) {
    const rnd = new Random(seed);
    const p = { seed, layers: Math.max(1, layers | 0) };
    p.N = rnd.integer(5, 8);
    p.M = rnd.integer(3, 6);
    p.rOut = 100;
    p.rIn = rnd.uniform(55, 75);
    p.rInscribed = starInscribedRadius(p.N, p.rOut, p.rIn);
    p.rHole = rnd.uniform(0.35, 0.55) * p.rInscribed;
    p.outerPhaseDeg = rnd.uniform(0, 180 / p.N);
    p.holePhaseDeg = rnd.uniform(0, 360 / p.M);
    const maxShift = 0.85 * p.rInscribed - p.rHole;
    const shift = rnd.uniform(0.35, 1) * maxShift;
    const phi = rnd.uniform(0, 2 * Math.PI);
    p.dx = shift * Math.cos(phi);
    p.dy = shift * Math.sin(phi);
    p.H = rnd.uniform(40, 80);
    p.twistDeg = rnd.uniform(15, 45);
    p.taper = rnd.uniform(0.75, 0.95);
    return p;
  }

  function outerContour(p) {
    const pts = [], count = 2 * p.N;
    for (let i = 0; i < count; i++) {
      const a = M.deg2rad(p.outerPhaseDeg) + 2 * Math.PI * i / count;
      const r = i % 2 === 0 ? p.rOut : p.rIn;
      pts.push(M.vec3(r * Math.cos(a), r * Math.sin(a), 0));
    }
    return pts;
  }

  function holeContour(p) {
    const pts = [];
    for (let k = 0; k < p.M; k++) {
      const a = M.deg2rad(p.holePhaseDeg) + 2 * Math.PI * k / p.M;
      pts.push(M.vec3(p.dx + p.rHole * Math.cos(a), p.dy + p.rHole * Math.sin(a), 0));
    }
    return pts;
  }

  const orient = (a, b, c) => (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  function segmentsCross(a, b, c, d) {
    const d1 = orient(a, b, c), d2 = orient(a, b, d), d3 = orient(c, d, a), d4 = orient(c, d, b);
    return ((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0));
  }
  const pointInTriangle = (p, a, b, c, eps) =>
    orient(a, b, p) >= -eps && orient(b, c, p) >= -eps && orient(c, a, p) >= -eps;
  const dist2 = (a, b) => (a.x - b.x) ** 2 + (a.y - b.y) ** 2;

  function triangulateWithHole(outer, hole) {
    const n = outer.length, m = hole.length;
    const P = outer.concat(hole);

    let h = 0;
    for (let k = 1; k < m; k++) if (hole[k].x > hole[h].x) h = k;
    const order = [...Array(n).keys()].sort((a, b) => dist2(outer[a], hole[h]) - dist2(outer[b], hole[h]));
    let v = order[0];
    for (const cand of order) {
      let visible = true;
      for (let i = 0; i < n && visible; i++) {
        const j = (i + 1) % n;
        if (i === cand || j === cand) continue;
        if (segmentsCross(hole[h], outer[cand], outer[i], outer[j])) visible = false;
      }
      for (let k = 0; k < m && visible; k++) {
        const l = (k + 1) % m;
        if (k === h || l === h) continue;
        if (segmentsCross(hole[h], outer[cand], hole[k], hole[l])) visible = false;
      }
      if (visible) { v = cand; break; }
    }

    const poly = [];
    for (let i = 0; i <= v; i++) poly.push(i);
    for (let s = 0; s <= m; s++) poly.push(n + (((h - s) % m) + m) % m);
    for (let i = v; i < n; i++) poly.push(i);

    let scale = 0;
    for (const q of P) scale = Math.max(scale, Math.abs(q.x), Math.abs(q.y));
    const eps = 1e-9 * Math.max(1, scale * scale);

    const tris = [];
    while (poly.length > 3) {
      const cnt = poly.length;
      let earAt = -1, bestConvex = -1e300, bestAt = 0;
      for (let i = 0; i < cnt; i++) {
        const a = poly[(i + cnt - 1) % cnt], b = poly[i], c = poly[(i + 1) % cnt];
        const area = orient(P[a], P[b], P[c]);
        if (area > bestConvex) { bestConvex = area; bestAt = i; }
        if (area <= eps) continue;
        let empty = true;
        for (let j = 0; j < cnt && empty; j++) {
          const q = poly[j];
          if (q === a || q === b || q === c) continue;
          if (pointInTriangle(P[q], P[a], P[b], P[c], eps)) empty = false;
        }
        if (empty) { earAt = i; break; }
      }
      if (earAt < 0) earAt = bestAt;
      const a = poly[(earAt + cnt - 1) % cnt], b = poly[earAt], c = poly[(earAt + 1) % cnt];
      if (orient(P[a], P[b], P[c]) > eps) tris.push([a, b, c]);
      poly.splice(earAt, 1);
    }
    if (orient(P[poly[0]], P[poly[1]], P[poly[2]]) > eps) tris.push([poly[0], poly[1], poly[2]]);
    return tris;
  }

  function buildMesh(p) {
    const outer = outerContour(p), hole = holeContour(p);
    const n = outer.length, m = hole.length, ring = n + m, L = Math.max(1, p.layers | 0);
    const mesh = { params: p, vertices: [], triangles: [], edges: [], groups: [], ringSize: ring };

    for (let j = 0; j <= L; j++) {
      const t = j / L, s = 1 + t * (p.taper - 1);
      const tr = M.chain(M.translation(0, 0, t * p.H), M.rotationZ(t * p.twistDeg), M.scaling(s, s, 1));
      for (const q of outer) mesh.vertices.push(M.mulVec(tr, M.toVec4(q)));
      for (const q of hole) mesh.vertices.push(M.mulVec(tr, M.toVec4(q)));
    }
    const O = (j, i) => j * ring + (((i % n) + n) % n);
    const H = (j, k) => j * ring + n + (((k % m) + m) % m);
    const add = (a, b, c, g) => { mesh.triangles.push([a, b, c]); mesh.groups.push(g); };

    for (let j = 0; j < L; j++) {
      for (let i = 0; i < n; i++) {
        add(O(j, i), O(j, i + 1), O(j + 1, i + 1), GROUP.OuterWall);
        add(O(j, i), O(j + 1, i + 1), O(j + 1, i), GROUP.OuterWall);
      }
      for (let k = 0; k < m; k++) {
        add(H(j, k), H(j + 1, k + 1), H(j, k + 1), GROUP.HoleWall);
        add(H(j, k), H(j + 1, k), H(j + 1, k + 1), GROUP.HoleWall);
      }
    }
    const caps = triangulateWithHole(outer, hole);
    for (const t of caps) add(t[0], t[2], t[1], GROUP.FrontCap);
    for (const t of caps) add(L * ring + t[0], L * ring + t[1], L * ring + t[2], GROUP.BackCap);

    const V = mesh.vertices.length, seen = new Set();
    for (const t of mesh.triangles)
      for (let e = 0; e < 3; e++) {
        let a = t[e], b = t[(e + 1) % 3];
        if (a > b) [a, b] = [b, a];
        const key = a * V + b;
        if (!seen.has(key)) { seen.add(key); mesh.edges.push([a, b]); }
      }
    return mesh;
  }

  const generate = (seed, layers = 1) => buildMesh(paramsFromSeed(seed, layers));

  function isClosedManifold(mesh) {
    const V = mesh.vertices.length, directed = new Map();
    for (const t of mesh.triangles)
      for (let e = 0; e < 3; e++) {
        const key = t[e] * V + t[(e + 1) % 3];
        directed.set(key, (directed.get(key) || 0) + 1);
      }
    for (const [key, cnt] of directed) {
      if (cnt !== 1) return false;
      const a = Math.floor(key / V), b = key % V;
      if (directed.get(b * V + a) !== 1) return false;
    }
    return true;
  }

  function computeChecksum(mesh, model) {
    const w = mesh.vertices.map((v) => M.perspectiveDivide(M.mulVec(model, v)));
    let sum = M.vec3();
    for (const p of w) sum = M.add(sum, p);
    const centroid = w.length ? M.mul(sum, 1 / w.length) : M.vec3();
    let area = 0;
    for (const t of mesh.triangles)
      area += 0.5 * M.length(M.cross(M.sub(w[t[1]], w[t[0]]), M.sub(w[t[2]], w[t[0]])));
    return { centroid, area, world: w };
  }

  function computeStats(mesh, model) {
    const V = mesh.vertices.length, E = mesh.edges.length, F = mesh.triangles.length;
    const { centroid, area } = computeChecksum(mesh, model);
    return { V, E, F, euler: V - E + F, closedManifold: isClosedManifold(mesh), centroid, area, det: M.det3(model) };
  }

  return {
    Random, GROUP, starInscribedRadius, paramsFromSeed, outerContour, holeContour, triangulateWithHole,
    buildMesh, generate, isClosedManifold, computeChecksum, computeStats,
  };
})();

if (typeof module !== 'undefined') module.exports = MESH;
