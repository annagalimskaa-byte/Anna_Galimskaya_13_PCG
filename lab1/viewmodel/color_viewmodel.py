

from model import color_math


class ColorViewModel:
    def __init__(self):
        self.rgb = [200, 60, 40]
        self.cmyk_method = "GCR"
        self.gamut_mode = "clip"
        self.illuminant_name = "D65"
        self.last_warning = ""

    def set_cmyk_method(self, method):
        self.cmyk_method = method

    def set_gamut_mode(self, mode):
        self.gamut_mode = mode

    def set_illuminant(self, name):
        self.illuminant_name = name


    def set_from_rgb(self, r, g, b):
        values, changed = color_math.constrain((r, g, b), [(0, 255)] * 3, self.gamut_mode)
        self.rgb = [round(v) for v in values]
        self._note_warning(changed)

    def set_from_cmyk(self, c, m, y, k):
        ranges = [(0, 100)] * 4
        (c, m, y, k), changed = color_math.constrain((c, m, y, k), ranges, self.gamut_mode)
        r, g, b = color_math.cmyk_to_rgb(c, m, y, k)
        self.rgb = [round(r), round(g), round(b)]
        self._note_warning(changed)

    def set_from_hls(self, h, l, s):
        ranges = [(0, 360), (0, 100), (0, 100)]
        (h, l, s), changed = color_math.constrain((h, l, s), ranges, self.gamut_mode)
        r, g, b = color_math.hls_to_rgb(h, l, s)
        self.rgb = [round(r), round(g), round(b)]
        self._note_warning(changed)

    def _note_warning(self, changed):
        self.last_warning = (
            "A value was outside the valid range for that model, so it was "
            f"adjusted ({self.gamut_mode})."
            if changed else ""
        )


    def get_rgb(self):
        return tuple(self.rgb)

    def get_hex(self):
        r, g, b = self.rgb
        return "#{:02x}{:02x}{:02x}".format(r, g, b)

    def get_cmyk(self):
        r, g, b = self.rgb
        return color_math.rgb_to_cmyk(r, g, b, self.cmyk_method)

    def get_hls(self):
        r, g, b = self.rgb
        return color_math.rgb_to_hls(r, g, b)


    def get_white_point(self):
        return color_math.ILLUMINANTS[self.illuminant_name]

    def get_xyz(self):
        r, g, b = self.rgb
        return color_math.rgb_to_xyz(r, g, b, self.get_white_point())

    def get_lab(self):
        r, g, b = self.rgb
        return color_math.rgb_to_lab(r, g, b, self.get_white_point())

    def convert_lab_to_rgb_preview(self, L, a, b, gamut_mode):

        raw = color_math.lab_to_rgb(L, a, b, self.get_white_point())
        values, changed = color_math.constrain(raw, [(0, 255)] * 3, gamut_mode)
        return [round(v) for v in values], changed



    def preview_rgb_for_rgb(self, r, g, b):
        return r, g, b

    def preview_rgb_for_cmyk(self, c, m, y, k):
        return color_math.cmyk_to_rgb(c, m, y, k)

    def preview_rgb_for_hls(self, h, l, s):
        return color_math.hls_to_rgb(h, l, s)