'use strict';

const RENDER = (() => {
  const M = (typeof M3D !== 'undefined') ? M3D : require('./math3d.js');

  const rgba = (r, g, b, a = 255) => (((a & 255) << 24) | ((b & 255) << 16) | ((g & 255) << 8) | (r & 255)) >>> 0;

  class Framebuffer {
    constructor() { this.width = 0; this.height = 0; this.color = new Uint32Array(0); this.depth = new Float32Array(0); }
    resize(w, h) {
      this.width = Math.max(1, w | 0); this.height = Math.max(1, h | 0);
      this.color = new Uint32Array(this.width * this.height);
      this.depth = new Float32Array(this.width * this.height).fill(Infinity);
    }
    clear() { this.color.fill(0); this.depth.fill(Infinity); }
  }

  const KIND = { Perspective: 0, Front: 1, Top: 2, Side: 3 };

  const directionToViewer = (v, p) => (v.perspective ? M.sub(v.eye, p) : v.toViewer);
  const nearDist = (c) => c.z + c.w;
  const lerp4 = (a, b, t) => M.vec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);

  function toScreen(v, clip) {
    const ndc = M.perspectiveDivide(clip);
    return { x: (ndc.x + 1) * 0.5 * v.width, y: (1 - ndc.y) * 0.5 * v.height, z: ndc.z };
  }

  function project(v, world) {
    const c = M.mulVec(v.projView, M.toVec4(world));
    if (nearDist(c) < 0 || c.w <= 1e-9) return { visible: false, x: 0, y: 0, z: 0 };
    const s = toScreen(v, c);
    return { visible: true, x: s.x, y: s.y, z: s.z };
  }

  function blend(dst, src, alpha) {
    if (alpha <= 0) return dst;
    if (alpha >= 1) return (src | 0xFF000000) >>> 0;
    const da = (dst >>> 24) / 255, oa = alpha + da * (1 - alpha);
    const ch = (sh) => {
      const s = (src >>> sh) & 255, d = (dst >>> sh) & 255;
      return Math.round((s * alpha + d * da * (1 - alpha)) / Math.max(oa, 1e-9)) & 255;
    };
    return ((Math.round(oa * 255) << 24) | (ch(16) << 16) | (ch(8) << 8) | ch(0)) >>> 0;
  }

  const shade = (c, k) => { k = Math.min(1, Math.max(0, k)); return rgba(c[0] * k, c[1] * k, c[2] * k); };

  function rasterTriangle(fb, a, b, c, col, edgeCol, edgeMask, ztest) {
    const area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (Math.abs(area) < 1e-12) return;
    const x0 = Math.max(0, Math.floor(Math.min(a.x, b.x, c.x)));
    const x1 = Math.min(fb.width - 1, Math.ceil(Math.max(a.x, b.x, c.x)));
    const y0 = Math.max(0, Math.floor(Math.min(a.y, b.y, c.y)));
    const y1 = Math.min(fb.height - 1, Math.ceil(Math.max(a.y, b.y, c.y)));
    if (x0 > x1 || y0 > y1) return;
    const lab = Math.hypot(b.x - a.x, b.y - a.y), lbc = Math.hypot(c.x - b.x, c.y - b.y), lca = Math.hypot(a.x - c.x, a.y - c.y);
    const inv = 1 / area, W = fb.width, color = fb.color, depth = fb.depth;
    for (let y = y0; y <= y1; y++) {
      const py = y + 0.5;
      for (let x = x0; x <= x1; x++) {
        const px = x + 0.5;
        const wa = (c.x - b.x) * (py - b.y) - (c.y - b.y) * (px - b.x);
        const wb = (a.x - c.x) * (py - c.y) - (a.y - c.y) * (px - c.x);
        const wc = (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
        const ba = wa * inv, bb = wb * inv, bc = wc * inv;
        if (ba < -1e-9 || bb < -1e-9 || bc < -1e-9) continue;
        const z = ba * a.z + bb * b.z + bc * c.z;
        const idx = y * W + x;
        if (ztest) {
          if (z >= depth[idx]) continue;
          depth[idx] = z;
        }
        let out = col;
        if (edgeMask) {
          let d = 1e9;
          if ((edgeMask & 1) && lab > 0) d = Math.min(d, Math.abs(wc) / lab);
          if ((edgeMask & 2) && lbc > 0) d = Math.min(d, Math.abs(wa) / lbc);
          if ((edgeMask & 4) && lca > 0) d = Math.min(d, Math.abs(wb) / lca);
          if (d < 1.2) out = blend(col, edgeCol, Math.min(1, 1.2 - d) * 0.85);
        }
        color[idx] = out;
      }
    }
  }

  function putPixel(fb, x, y, col, alpha) {
    if (x < 0 || y < 0 || x >= fb.width || y >= fb.height) return;
    const idx = y * fb.width + x;
    fb.color[idx] = blend(fb.color[idx], col, alpha);
  }

  const transformVertices = (mesh, model) => mesh.vertices.map((v) => M.perspectiveDivide(M.mulVec(model, v)));

  function faceNormal(a, b, c, det) {
    const n = M.cross(M.sub(b, a), M.sub(c, a));
    return det < 0 ? M.neg(n) : n;
  }

  const COLORS = [[84, 130, 196], [232, 140, 52], [98, 178, 126], [164, 128, 206]];

  function drawMesh(fb, v, mesh, world, det, opt) {
    const st = { drawn: 0, culled: 0 };
    const wireCol = rgba(40, 60, 90);
    for (let t = 0; t < mesh.triangles.length; t++) {
      const tri = mesh.triangles[t];
      const a = world[tri[0]], b = world[tri[1]], c = world[tri[2]];
      const n = faceNormal(a, b, c, det);
      if (opt.cull && M.dot(n, directionToViewer(v, a)) <= 0) { st.culled++; continue; }
      if (!opt.fill) {
        drawLine3D(fb, v, a, b, wireCol, 0, 0, 1);
        drawLine3D(fb, v, b, c, wireCol, 0, 0, 1);
        drawLine3D(fb, v, c, a, wireCol, 0, 0, 1);
        st.drawn++;
        continue;
      }
      const base = COLORS[opt.groupColors ? mesh.groups[t] : 0];
      let k = 1;
      if (opt.lighting) {
        const center = M.mul(M.add(M.add(a, b), c), 1 / 3);
        const L = M.normalize(M.sub(opt.light, center));
        k = opt.ambient + (1 - opt.ambient) * Math.max(0, M.dot(M.normalize(n), L));
      }
      const col = shade(base, k), edgeCol = shade(base, k * 0.45);

      const inp = [M.mulVec(v.projView, M.toVec4(a)), M.mulVec(v.projView, M.toVec4(b)), M.mulVec(v.projView, M.toVec4(c))];
      const poly = [], polyEdge = [];
      for (let i = 0; i < 3; i++) {
        const p = inp[i], q = inp[(i + 1) % 3], dp = nearDist(p), dq = nearDist(q);
        if (dp >= 0) {
          poly.push(p); polyEdge.push(true);
          if (dq < 0) { poly.push(lerp4(p, q, dp / (dp - dq))); polyEdge.push(false); }
        } else if (dq >= 0) {
          poly.push(lerp4(p, q, dp / (dp - dq))); polyEdge.push(true);
        }
      }
      const cnt = poly.length;
      if (cnt < 3) continue;
      const s = poly.map((p) => toScreen(v, p));
      for (let i = 1; i + 1 < cnt; i++) {
        let mask = 0;
        if (opt.edges) {
          if (i === 1 && polyEdge[0]) mask |= 1;
          if (polyEdge[i]) mask |= 2;
          if (i + 1 === cnt - 1 && polyEdge[cnt - 1]) mask |= 4;
        }
        rasterTriangle(fb, s[0], s[i], s[i + 1], col, edgeCol, mask, opt.zbuffer);
      }
      st.drawn++;
    }
    return st;
  }

  function drawLine3D(fb, v, a, b, color, hiddenColor, dash, thickness) {
    let ca = M.mulVec(v.projView, M.toVec4(a)), cb = M.mulVec(v.projView, M.toVec4(b));
    const da = nearDist(ca), db = nearDist(cb);
    if (da < 0 && db < 0) return;
    if (da < 0) ca = lerp4(ca, cb, da / (da - db));
    if (db < 0) cb = lerp4(cb, ca, db / (db - da));
    const p = toScreen(v, ca), q = toScreen(v, cb);
    const dx = q.x - p.x, dy = q.y - p.y, len = Math.hypot(dx, dy);
    const steps = Math.max(1, Math.ceil(Math.max(Math.abs(dx), Math.abs(dy))));
    if (steps > 20000) return;
    const steep = Math.abs(dy) > Math.abs(dx);
    for (let i = 0; i <= steps; i++) {
      const t = i / steps;
      if (dash > 0 && Math.floor(t * len / dash) % 2 === 1) continue;
      const x = p.x + dx * t, y = p.y + dy * t, z = p.z + (q.z - p.z) * t;
      const ix = Math.floor(x), iy = Math.floor(y);
      if (ix < -thickness || iy < -thickness || ix >= fb.width + thickness || iy >= fb.height + thickness) continue;
      let hidden = false;
      if (ix >= 0 && iy >= 0 && ix < fb.width && iy < fb.height) hidden = z > fb.depth[iy * fb.width + ix];
      const col = hidden ? hiddenColor : color;
      const alpha = (col >>> 24) / 255;
      if (alpha <= 0) continue;
      for (let k = 0; k < thickness; k++) {
        const ox = steep ? k - (thickness >> 1) : 0, oy = steep ? 0 : k - (thickness >> 1);
        putPixel(fb, ix + ox, iy + oy, col, alpha);
      }
    }
  }

  function cameraEye(cam) {
    const y = M.deg2rad(cam.yawDeg), p = M.deg2rad(cam.pitchDeg);
    return M.add(cam.target, M.mul(M.vec3(Math.cos(p) * Math.sin(y), Math.sin(p), Math.cos(p) * Math.cos(y)), cam.distance));
  }

  function makePerspectiveView(cam, width, height) {
    const v = { kind: KIND.Perspective, width: Math.max(1, width), height: Math.max(1, height), perspective: true };
    v.eye = cameraEye(cam);
    v.view = M.lookAt(v.eye, cam.target, M.vec3(0, 1, 0));
    v.proj = M.perspective(cam.fovDeg, v.width / v.height, cam.zNear, cam.zFar);
    v.projView = M.mulMat(v.proj, v.view);
    return v;
  }

  function makeOrthoView(kind, width, height, fit) {
    const v = { kind, width: Math.max(1, width), height: Math.max(1, height), perspective: false };
    let up;
    if (kind === KIND.Top) {
      v.toViewer = M.vec3(0, 1, 0); up = M.vec3(0, 0, -1);
      v.hAxis = 0; v.hSign = 1; v.vAxis = 2; v.vSign = -1;
    } else if (kind === KIND.Side) {
      v.toViewer = M.vec3(-1, 0, 0); up = M.vec3(0, 1, 0);
      v.hAxis = 2; v.hSign = 1; v.vAxis = 1; v.vSign = 1;
    } else {
      v.toViewer = M.vec3(0, 0, 1); up = M.vec3(0, 1, 0);
      v.hAxis = 0; v.hSign = 1; v.vAxis = 1; v.vSign = 1;
    }
    const comp = (p, i) => (i === 0 ? p.x : i === 1 ? p.y : p.z);
    let hMin = 1e300, hMax = -1e300, vMin = 1e300, vMax = -1e300;
    for (const p of fit) {
      const h = comp(p, v.hAxis) * v.hSign, w = comp(p, v.vAxis) * v.vSign;
      hMin = Math.min(hMin, h); hMax = Math.max(hMax, h); vMin = Math.min(vMin, w); vMax = Math.max(vMax, w);
    }
    if (!fit.length) { hMin = vMin = -100; hMax = vMax = 100; }
    const rangeH = Math.max(hMax - hMin, 1e-6), rangeV = Math.max(vMax - vMin, 1e-6);
    const ppu = Math.min(v.width / (rangeH * 1.18), v.height / (rangeV * 1.18));
    const cH = (hMin + hMax) / 2, cV = (vMin + vMax) / 2;
    const halfW = v.width / ppu / 2, halfH = v.height / ppu / 2;
    const center = M.vec3();
    const set = (p, i, val) => { if (i === 0) p.x = val; else if (i === 1) p.y = val; else p.z = val; };
    set(center, v.hAxis, cH * v.hSign);
    set(center, v.vAxis, cV * v.vSign);
    const D = 10000;
    v.eye = M.add(center, M.mul(v.toViewer, D));
    v.view = M.lookAt(v.eye, center, up);
    v.proj = M.orthographic(-halfW, halfW, -halfH, halfH, 1, 2 * D);
    v.projView = M.mulMat(v.proj, v.view);
    v.pixelsPerUnit = ppu;
    v.originX = v.width / 2 - cH * ppu;
    v.originY = v.height / 2 + cV * ppu;
    return v;
  }

  return { rgba, Framebuffer, KIND, project, transformVertices, faceNormal, drawMesh, drawLine3D, cameraEye,
           makePerspectiveView, makeOrthoView, directionToViewer };
})();

if (typeof module !== 'undefined') module.exports = RENDER;
