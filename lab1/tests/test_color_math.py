

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from model import color_math as cm


class TestRgbCmykRoundTrip(unittest.TestCase):
    def test_pure_red(self):
        c, m, y, k = cm.rgb_to_cmyk(255, 0, 0, "GCR")
        self.assertAlmostEqual(c, 0.0, places=1)
        self.assertAlmostEqual(m, 100.0, places=1)
        self.assertAlmostEqual(y, 100.0, places=1)
        self.assertAlmostEqual(k, 0.0, places=1)

    def test_black_and_white(self):
        self.assertEqual(cm.rgb_to_cmyk(255, 255, 255, "GCR"), (0.0, 0.0, 0.0, 0.0))
        c, m, y, k = cm.rgb_to_cmyk(0, 0, 0, "GCR")
        self.assertAlmostEqual(k, 100.0, places=1)

    def test_round_trip_matches_reference_formula(self):
        for original in [(120, 80, 60), (30, 200, 90), (10, 10, 250)]:
            c, m, y, k = cm.rgb_to_cmyk(*original, "GCR")
            r, g, b = cm.cmyk_to_rgb(c, m, y, k)
            self.assertAlmostEqual(r, original[0], places=0)
            self.assertAlmostEqual(g, original[1], places=0)
            self.assertAlmostEqual(b, original[2], places=0)

    def test_ucr_also_round_trips(self):
        original = (120, 80, 60)
        c, m, y, k = cm.rgb_to_cmyk(*original, "UCR")
        r, g, b = cm.cmyk_to_rgb(c, m, y, k)
        self.assertAlmostEqual(r, original[0], places=0)
        self.assertAlmostEqual(g, original[1], places=0)
        self.assertAlmostEqual(b, original[2], places=0)


class TestGcrVsUcr(unittest.TestCase):
    def test_disagree_on_a_shadow_color(self):
        gcr = cm.rgb_to_cmyk(20, 30, 10, "GCR")
        ucr = cm.rgb_to_cmyk(20, 30, 10, "UCR")
        self.assertNotEqual(gcr, ucr)

    def test_agree_on_a_light_color(self):
        # UCR only kicks in past the halfway (shadow) point.
        _, _, _, k_ucr = cm.rgb_to_cmyk(230, 210, 220, "UCR")
        self.assertAlmostEqual(k_ucr, 0.0, places=1)


class TestRgbHlsRoundTrip(unittest.TestCase):
    def test_primary_colors(self):
        self.assertEqual(cm.rgb_to_hls(255, 0, 0), (0.0, 50.0, 100.0))

    def test_gray_has_no_hue_or_saturation(self):
        h, l, s = cm.rgb_to_hls(128, 128, 128)
        self.assertEqual(s, 0.0)

    def test_round_trip(self):
        for original in [(30, 200, 90), (10, 10, 250), (220, 40, 130)]:
            h, l, s = cm.rgb_to_hls(*original)
            r, g, b = cm.hls_to_rgb(h, l, s)
            self.assertAlmostEqual(r, original[0], places=0)
            self.assertAlmostEqual(g, original[1], places=0)
            self.assertAlmostEqual(b, original[2], places=0)


class TestConstrain(unittest.TestCase):
    def test_in_range_values_are_unchanged(self):
        values, changed = cm.constrain((10, 20, 30), [(0, 255)] * 3, "clip")
        self.assertEqual(values, [10, 20, 30])
        self.assertFalse(changed)

    def test_clip_pushes_each_value_to_its_own_edge(self):
        values, changed = cm.constrain((300, 100, -20), [(0, 255)] * 3, "clip")
        self.assertEqual(values, [255, 100, 0])
        self.assertTrue(changed)

    def test_scale_keeps_proportions(self):
        values, changed = cm.constrain((300, 100, -20), [(0, 255)] * 3, "scale")
        factor = 255 / 300
        self.assertAlmostEqual(values[0], 255)
        self.assertAlmostEqual(values[1], 100 * factor)
        self.assertTrue(changed)


class TestRgbToLab(unittest.TestCase):


    def setUp(self):
        self.d65 = cm.ILLUMINANTS["D65"]

    def test_pure_red_matches_known_reference(self):
        L, a, b = cm.rgb_to_lab(255, 0, 0, self.d65)
        self.assertAlmostEqual(L, 53.24, places=1)
        self.assertAlmostEqual(a, 80.09, places=1)
        self.assertAlmostEqual(b, 67.20, places=1)

    def test_white_is_neutral(self):
        L, a, b = cm.rgb_to_lab(255, 255, 255, self.d65)
        self.assertAlmostEqual(L, 100.0, places=0)
        self.assertAlmostEqual(a, 0.0, places=1)
        self.assertAlmostEqual(b, 0.0, places=1)

    def test_round_trip_rgb_lab_rgb(self):
        original = (30, 200, 90)
        L, a, b = cm.rgb_to_lab(*original, self.d65)
        result = cm.lab_to_rgb(L, a, b, self.d65)
        for got, expected in zip(result, original):
            self.assertAlmostEqual(got, expected, places=0)

    def test_different_illuminants_give_different_matrices(self):

        m_d65 = cm.rgb_to_xyz_matrix(cm.ILLUMINANTS["D65"])
        m_d50 = cm.rgb_to_xyz_matrix(cm.ILLUMINANTS["D50"])
        self.assertNotEqual(m_d65, m_d50)

    def test_clip_and_scale_disagree_on_an_out_of_gamut_lab_value(self):

        raw = cm.lab_to_rgb(50, 100, -120, self.d65)
        clipped, _ = cm.constrain(raw, [(0, 255)] * 3, "clip")
        scaled, _ = cm.constrain(raw, [(0, 255)] * 3, "scale")
        self.assertNotEqual(clipped, scaled)


if __name__ == "__main__":
    unittest.main()