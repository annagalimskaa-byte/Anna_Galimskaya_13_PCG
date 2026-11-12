(function (root) {
  "use strict";

  const EPS = 1e-9;

  const sub = (a, b) => ({ x: a.x - b.x, y: a.y - b.y });
  const add = (a, b) => ({ x: a.x + b.x, y: a.y + b.y });
  const mul = (a, k) => ({ x: a.x * k, y: a.y * k });
  const dot = (a, b) => a.x * b.x + a.y * b.y;
  const cross = (a, b) => a.x * b.y - a.y * b.x;
  const len = (a) => Math.hypot(a.x, a.y);
  const mid = (a, b) => ({ x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 });

  function signedArea(poly) {
    let s = 0;
    for (let i = 0; i < poly.length; i++) s += cross(poly[i], poly[(i + 1) % poly.length]);
    return s / 2;
  }

  function isConvex(poly) {
    const n = poly.length;
    if (n < 3) return false;
    let sign = 0;
    let angleSum = 0;
    for (let i = 0; i < n; i++) {
      const a = poly[i], b = poly[(i + 1) % n], c = poly[(i + 2) % n];
      const e1 = sub(b, a), e2 = sub(c, b);
      if (len(e1) < EPS) return false;
      const cr = cross(e1, e2);
      if (Math.abs(cr) > EPS) {
        const s = cr > 0 ? 1 : -1;
        if (sign === 0) sign = s;
        else if (s !== sign) return false;
      }
      angleSum += Math.atan2(cr, dot(e1, e2));
    }
    if (sign === 0) return false;
    return Math.abs(Math.abs(angleSum) - 2 * Math.PI) < 1e-6;
  }

  const rectToPolygon = (r) => [
    { x: r.xmin, y: r.ymin }, { x: r.xmax, y: r.ymin },
    { x: r.xmax, y: r.ymax }, { x: r.xmin, y: r.ymax },
  ];

  const LEFT = 1, RIGHT = 2, BOTTOM = 4, TOP = 8;

  function outCode(p, r) {
    let c = 0;
    if (p.x < r.xmin) c |= LEFT; else if (p.x > r.xmax) c |= RIGHT;
    if (p.y < r.ymin) c |= BOTTOM; else if (p.y > r.ymax) c |= TOP;
    return c;
  }

  const rectContains = (r, p, eps) =>
    p.x >= r.xmin - eps && p.x <= r.xmax + eps && p.y >= r.ymin - eps && p.y <= r.ymax + eps;

  function midpointClip(seg, win, eps) {
    if (!(eps > 0)) eps = Math.max(win.xmax - win.xmin, win.ymax - win.ymin, 1) * 1e-7;
    const st = { steps: 0, found: false, first: null, last: null, trace: [] };
    const addVisible = (a, b) => {
      if (!st.found) { st.first = a; st.found = true; }
      st.last = b;
    };

    function rec(a, b, ca, cb, depth) {
      st.steps++;
      if ((ca | cb) === 0) { addVisible(a, b); return; }
      if ((ca & cb) !== 0) return;
      const m = mid(a, b);
      if (len(sub(b, a)) < eps || depth > 64) {
        if (ca === 0) addVisible(a, a);
        else if (cb === 0) addVisible(b, b);
        else if (rectContains(win, m, eps)) addVisible(m, m);
        return;
      }
      if (depth < 12) st.trace.push(m);
      const cm = outCode(m, win);
      rec(a, m, ca, cm, depth + 1);
      rec(m, b, cm, cb, depth + 1);
    }

    rec(seg.a, seg.b, outCode(seg.a, win), outCode(seg.b, win), 0);
    return {
      visible: st.found ? { a: st.first, b: st.last } : null,
      steps: st.steps,
      trace: st.trace,
    };
  }

  function cyrusBeckClip(seg, poly) {
    const res = { visible: null, steps: 0, trace: [] };
    const n = poly.length;
    if (n < 3) return res;
    const orient = signedArea(poly) > 0 ? 1 : -1;
    const d = sub(seg.b, seg.a);
    let tL = 0, tU = 1;

    for (let i = 0; i < n; i++) {
      res.steps++;
      const vi = poly[i];
      const edge = sub(poly[(i + 1) % n], vi);
      const normal = { x: -edge.y * orient, y: edge.x * orient };
      const num = dot(normal, sub(seg.a, vi));
      const den = dot(normal, d);
      if (Math.abs(den) < EPS) {
        if (num < -EPS) return res;
        continue;
      }
      const t = -num / den;
      if (t >= 0 && t <= 1) res.trace.push(add(seg.a, mul(d, t)));
      if (den > 0) tL = Math.max(tL, t); else tU = Math.min(tU, t);
      if (tL > tU + EPS) return res;
    }
    res.visible = { a: add(seg.a, mul(d, tL)), b: add(seg.a, mul(d, tU)) };
    return res;
  }

  class ParseError extends Error {}

  function stripComments(raw) {
    let out = "", inStars = false;
    for (const c of raw) {
      if (c === "#" && !inStars) break;
      if (c === "*") { inStars = !inStars; continue; }
      if (!inStars) out += c;
    }
    return out;
  }

  function tokenize(text) {
    const lines = [];
    text.split(/\n/).forEach((raw, i) => {
      const s = stripComments(raw).replace(/[,;\t\r]/g, " ").trim();
      if (!s) return;
      const values = s.split(/\s+/).map((tok) => {
        const v = Number(tok);
        if (tok === "" || !Number.isFinite(v)) {
          throw new ParseError(`Строка ${i + 1}: «${tok}» не является числом`);
        }
        return v;
      });
      lines.push({ number: i + 1, values });
    });
    return lines;
  }

  function readCount(line, what, minValue) {
    if (line.values.length !== 1) throw new ParseError(`Строка ${line.number}: ожидалось одно число — ${what}`);
    const v = line.values[0];
    if (!Number.isInteger(v) || v < minValue || v > 1e6) {
      throw new ParseError(`Строка ${line.number}: ${what} должно быть целым числом не меньше ${minValue}`);
    }
    return v;
  }

  function expectValues(line, count, what) {
    if (line.values.length !== count) {
      throw new ParseError(`Строка ${line.number}: для ${what} нужно ${count} числа, найдено ${line.values.length}`);
    }
  }

  function parseInput(text) {
    const lines = tokenize(text);
    if (!lines.length) throw new ParseError("Файл пуст");
    let idx = 0;
    const n = readCount(lines[idx++], "число отрезков", 0);
    if (lines.length < idx + n) {
      throw new ParseError(`Указано ${n} отрезков, но в файле только ${lines.length - idx} строк после первой`);
    }
    const segments = [];
    for (let i = 0; i < n; i++) {
      const l = lines[idx++];
      expectValues(l, 4, `отрезка №${i + 1}`);
      const [x1, y1, x2, y2] = l.values;
      segments.push({ a: { x: x1, y: y1 }, b: { x: x2, y: y2 } });
    }
    const rest = lines.length - idx;
    if (rest === 0) throw new ParseError("Не задано отсекающее окно (последняя строка файла)");

    if (rest === 1 && lines[idx].values.length === 4) {
      const l = lines[idx];
      const [xmin, ymin, xmax, ymax] = l.values;
      if (!(xmin < xmax) || !(ymin < ymax)) {
        throw new ParseError(`Строка ${l.number}: для окна должно выполняться Xmin < Xmax и Ymin < Ymax`);
      }
      return { segments, kind: "rect", rect: { xmin, ymin, xmax, ymax }, polygon: null };
    }

    const m = readCount(lines[idx++], "число вершин многоугольника", 3);
    if (lines.length - idx !== m) {
      throw new ParseError(`Ожидалось ${m} строк с вершинами окна, найдено ${lines.length - idx}`);
    }
    const polygon = [];
    for (let i = 0; i < m; i++) {
      const l = lines[idx++];
      expectValues(l, 2, `вершины №${i + 1}`);
      polygon.push({ x: l.values[0], y: l.values[1] });
    }
    if (!isConvex(polygon)) {
      throw new ParseError("Отсекающий многоугольник не является выпуклым (или вершины перечислены не в порядке обхода)");
    }
    return { segments, kind: "polygon", rect: null, polygon };
  }

  function clipAll(data) {
    return data.segments.map((s) =>
      data.kind === "rect" ? midpointClip(s, data.rect) : cyrusBeckClip(s, data.polygon));
  }

  const algorithmName = (kind) => (kind === "rect" ? "Алгоритм средней точки" : "Алгоритм Кируса–Бека");

  const api = {
    EPS, outCode, LEFT, RIGHT, BOTTOM, TOP, isConvex, signedArea, rectToPolygon,
    midpointClip, cyrusBeckClip, parseInput, clipAll, algorithmName, ParseError,
  };
  if (typeof module !== "undefined" && module.exports) module.exports = api;
  else root.Clipping = api;
})(typeof window !== "undefined" ? window : globalThis);
