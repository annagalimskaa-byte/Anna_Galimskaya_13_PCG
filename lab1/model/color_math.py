
def constrain(values, ranges, mode="clip"):

    values = list(values)

    if mode == "scale":
        factor = 1.0
        for v, (lo, hi) in zip(values, ranges):
            if hi > 0 and v > hi:
                factor = min(factor, hi / v)
        values = [v * factor for v in values]

    result = [max(lo, min(hi, v)) for v, (lo, hi) in zip(values, ranges)]
    changed = any(abs(a - b) > 1e-6 for a, b in zip(values, result))
    return result, changed



def rgb_to_cmyk(r, g, b, method="GCR"):

    c = 1 - r / 255.0
    m = 1 - g / 255.0
    y = 1 - b / 255.0
    shared = min(c, m, y)

    if method == "UCR":
        threshold = 0.5
        k = max(0.0, shared - threshold)
    else:  # GCR
        k = shared

    if k >= 1.0:
        c2 = m2 = y2 = 0.0
    else:
        c2 = (c - k) / (1 - k)
        m2 = (m - k) / (1 - k)
        y2 = (y - k) / (1 - k)

    return c2 * 100, m2 * 100, y2 * 100, k * 100


def cmyk_to_rgb(c, m, y, k):

    c, m, y, k = c / 100.0, m / 100.0, y / 100.0, k / 100.0
    r = 255 * (1 - c) * (1 - k)
    g = 255 * (1 - m) * (1 - k)
    b = 255 * (1 - y) * (1 - k)
    return r, g, b



def rgb_to_hls(r, g, b):
    r, g, b = r / 255.0, g / 255.0, b / 255.0
    mx, mn = max(r, g, b), min(r, g, b)
    diff = mx - mn
    l = (mx + mn) / 2

    if diff == 0:
        h = 0.0
        s = 0.0
    else:
        s = diff / (1 - abs(2 * l - 1))
        if mx == r:
            h = (60 * ((g - b) / diff) + 360) % 360
        elif mx == g:
            h = (60 * ((b - r) / diff) + 120) % 360
        else:
            h = (60 * ((r - g) / diff) + 240) % 360

    return h, l * 100, s * 100


def _hls_value(hue, m1, m2):

    hue = hue % 360
    if hue < 60:
        return m1 + (m2 - m1) * hue / 60
    if hue < 180:
        return m2
    if hue < 240:
        return m1 + (m2 - m1) * (240 - hue) / 60
    return m1


def hls_to_rgb(h, l, s):

    l, s = l / 100.0, s / 100.0

    if s == 0:
        r = g = b = l
    else:
        m2 = l * (1 + s) if l < 0.5 else (l + s - l * s)
        m1 = 2 * l - m2
        r = _hls_value(h + 120, m1, m2)
        g = _hls_value(h, m1, m2)
        b = _hls_value(h - 120, m1, m2)

    return r * 255, g * 255, b * 255



ILLUMINANTS = {
    "D65": (95.047, 100.000, 108.883),
    "D50": (96.422, 100.000, 82.521),
    "E":   (100.000, 100.000, 100.000),
}


_SRGB_PRIMARIES_XY = {"R": (0.6400, 0.3300), "G": (0.3000, 0.6000), "B": (0.1500, 0.0600)}


def _xy_to_xyz_unit(x, y):
    return (x / y, 1.0, (1 - x - y) / y)


def _matmul(m, v):
    return tuple(sum(m[r][c] * v[c] for c in range(3)) for r in range(3))


def _matinv(m):
    a, b, c = m[0]
    d, e, f = m[1]
    g, h, i = m[2]
    det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g)
    adj = [
        [(e * i - f * h), -(b * i - c * h), (b * f - c * e)],
        [-(d * i - f * g), (a * i - c * g), -(a * f - c * d)],
        [(d * h - e * g), -(a * h - b * g), (a * e - b * d)],
    ]
    return [[adj[r][c] / det for c in range(3)] for r in range(3)]


def rgb_to_xyz_matrix(white_point):

    Xr, Yr, Zr = _xy_to_xyz_unit(*_SRGB_PRIMARIES_XY["R"])
    Xg, Yg, Zg = _xy_to_xyz_unit(*_SRGB_PRIMARIES_XY["G"])
    Xb, Yb, Zb = _xy_to_xyz_unit(*_SRGB_PRIMARIES_XY["B"])
    M = [[Xr, Xg, Xb], [Yr, Yg, Yb], [Zr, Zg, Zb]]
    Sr, Sg, Sb = _matmul(_matinv(M), white_point)
    return [
        [M[0][0] * Sr, M[0][1] * Sg, M[0][2] * Sb],
        [M[1][0] * Sr, M[1][1] * Sg, M[1][2] * Sb],
        [M[2][0] * Sr, M[2][1] * Sg, M[2][2] * Sb],
    ]


def _srgb_to_linear(v):
    v = v / 255.0
    return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4


def _linear_to_srgb(v):
    out = v * 12.92 if v <= 0.0031308 else 1.055 * (v ** (1 / 2.4)) - 0.055
    return out * 255.0


def rgb_to_xyz(r, g, b, white_point):
    matrix = rgb_to_xyz_matrix(white_point)
    lr, lg, lb = _srgb_to_linear(r), _srgb_to_linear(g), _srgb_to_linear(b)
    return _matmul(matrix, (lr, lg, lb))


def xyz_to_rgb(X, Y, Z, white_point):

    inv = _matinv(rgb_to_xyz_matrix(white_point))
    lr, lg, lb = _matmul(inv, (X, Y, Z))
    return _linear_to_srgb(lr), _linear_to_srgb(lg), _linear_to_srgb(lb)


_DELTA = 6 / 29


def _f(t):
    return t ** (1 / 3) if t > _DELTA ** 3 else t / (3 * _DELTA ** 2) + 4 / 29


def _f_inv(t):
    return t ** 3 if t > _DELTA else 3 * _DELTA ** 2 * (t - 4 / 29)


def xyz_to_lab(X, Y, Z, white_point):
    Xn, Yn, Zn = white_point
    fx, fy, fz = _f(X / Xn), _f(Y / Yn), _f(Z / Zn)
    return 116 * fy - 16, 500 * (fx - fy), 200 * (fy - fz)


def lab_to_xyz(L, a, b, white_point):
    Xn, Yn, Zn = white_point
    fy = (L + 16) / 116
    fx, fz = fy + a / 500, fy - b / 200
    return Xn * _f_inv(fx), Yn * _f_inv(fy), Zn * _f_inv(fz)


def rgb_to_lab(r, g, b, white_point):
    return xyz_to_lab(*rgb_to_xyz(r, g, b, white_point), white_point)


def lab_to_rgb(L, a, b, white_point):

    return xyz_to_rgb(*lab_to_xyz(L, a, b, white_point), white_point)



if __name__ == "__main__":
    print("Pure red (255, 0, 0):")
    print("  CMYK (GCR):", rgb_to_cmyk(255, 0, 0, "GCR"))
    print("  CMYK (UCR):", rgb_to_cmyk(255, 0, 0, "UCR"))
    print("  HLS:", rgb_to_hls(255, 0, 0), " (expect H=0 L=50 S=100)")
    print()

    print("Round trip RGB -> CMYK -> RGB for (120, 80, 60):")
    c, m, y, k = rgb_to_cmyk(120, 80, 60, "GCR")
    print("  CMYK =", (c, m, y, k))
    print("  back to RGB =", cmyk_to_rgb(c, m, y, k), " (expect ~120, 80, 60)")
    print()

    print("Round trip RGB -> HLS -> RGB for (30, 200, 90):")
    h, l, s = rgb_to_hls(30, 200, 90)
    print("  HLS =", (h, l, s))
    print("  back to RGB =", hls_to_rgb(h, l, s), " (expect ~30, 200, 90)")
    print()

    print("constrain() clip vs scale on an out-of-range RGB triple (300, 100, -20):")
    ranges = [(0, 255), (0, 255), (0, 255)]
    print("  clip :", constrain((300, 100, -20), ranges, "clip"))
    print("  scale:", constrain((300, 100, -20), ranges, "scale"))
    print()

    print("Pure red (255, 0, 0) -> Lab under D65 (expect roughly L=53.2 a=80.1 b=67.2):")
    d65 = ILLUMINANTS["D65"]
    print(" ", rgb_to_lab(255, 0, 0, d65))
    print()

    print("Switching illuminant changes the matrix (no hardcoded constant):")
    print("  D65 matrix row 0:", rgb_to_xyz_matrix(ILLUMINANTS["D65"])[0])
    print("  D50 matrix row 0:", rgb_to_xyz_matrix(ILLUMINANTS["D50"])[0])
    print()

    print("Out-of-gamut Lab converted with clip vs scale (raw, unclamped output shown too):")
    L, a, b = 50, 100, -120
    raw = lab_to_rgb(L, a, b, d65)
    print("  raw  :", raw)
    print("  clip :", constrain(raw, ranges, "clip"))
    print("  scale:", constrain(raw, ranges, "scale"))
