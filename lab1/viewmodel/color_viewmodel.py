"""
viewmodel/color_viewmodel.py
=============================

The "Link" layer, and the only file allowed to import model/color_math.
The View never touches color_math directly - it only calls methods on
this class.

Design: the color is always stored internally as canonical RGB. When
the user edits CMYK or HLS, this class converts THAT model's values to
RGB and overwrites the canonical color; when the View then asks every
model for its current values, CMYK and HLS are freshly recomputed from
that same canonical RGB. That is what makes "change one model, the
other two update automatically" work, no matter which of the three the
user was just typing into.
"""

from model import color_math


class ColorViewModel:
    def __init__(self):
        self.rgb = [200, 60, 40]
        self.cmyk_method = "GCR"     # "GCR" | "UCR"
        self.gamut_mode = "clip"     # "clip" | "scale"
        self.illuminant_name = "D65"  # "D65" | "D50" | "E" - subgroup-10A addition #2
        self.last_warning = ""       # set whenever a typed value had to be constrained

    # ------------------------------------------------------------------
    # Settings the View can change
    # ------------------------------------------------------------------
    def set_cmyk_method(self, method):
        self.cmyk_method = method

    def set_gamut_mode(self, mode):
        self.gamut_mode = mode

    def set_illuminant(self, name):
        self.illuminant_name = name

    # ------------------------------------------------------------------
    # Setting the color FROM any one of the three models
    # ------------------------------------------------------------------
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

    # ------------------------------------------------------------------
    # Reading the color back out, in each of the three models
    # ------------------------------------------------------------------
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

    # ------------------------------------------------------------------
    # XYZ / Lab - subgroup-10A addition #2. Read-only: they are shown
    # alongside the three required models, always recomputed from the
    # same canonical RGB using whichever illuminant is selected, but you
    # don't edit the color through them directly (see the Lab -> RGB
    # demo box below for the one place they DO feed back in).
    # ------------------------------------------------------------------
    def get_white_point(self):
        return color_math.ILLUMINANTS[self.illuminant_name]

    def get_xyz(self):
        r, g, b = self.rgb
        return color_math.rgb_to_xyz(r, g, b, self.get_white_point())

    def get_lab(self):
        r, g, b = self.rgb
        return color_math.rgb_to_lab(r, g, b, self.get_white_point())

    def convert_lab_to_rgb_preview(self, L, a, b, gamut_mode):
        """
        Used only by the Lab -> RGB demo box. Deliberately does NOT
        touch the canonical color - it just answers "if I converted
        this Lab value right now, what RGB would I get, and did it
        need clipping/scaling?" so you can compare the two strategies
        without disturbing whatever color the rest of the app is
        showing.
        """
        raw = color_math.lab_to_rgb(L, a, b, self.get_white_point())
        values, changed = color_math.constrain(raw, [(0, 255)] * 3, gamut_mode)
        return [round(v) for v in values], changed

    # ------------------------------------------------------------------
    # "Preview" conversions used only for drawing gradient sliders - the
    # View calls these instead of importing color_math itself.
    # ------------------------------------------------------------------
    def preview_rgb_for_rgb(self, r, g, b):
        return r, g, b

    def preview_rgb_for_cmyk(self, c, m, y, k):
        return color_math.cmyk_to_rgb(c, m, y, k)

    def preview_rgb_for_hls(self, h, l, s):
        return color_math.hls_to_rgb(h, l, s)
