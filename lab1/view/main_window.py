

import tkinter as tk
from tkinter import ttk

SLIDER_WIDTH = 220
SLIDER_HEIGHT = 22


PALETTE_HUE_COLORS = [
    (244, 175, 175), (244, 226, 175), (209, 244, 175), (175, 244, 192), (175, 244, 244), (175, 192, 244), (209, 175, 244), (244, 175, 226),
    (255, 0, 0), (255, 191, 0), (128, 255, 0), (0, 255, 64), (0, 255, 255), (0, 64, 255), (127, 0, 255), (255, 0, 191),
    (193, 21, 21), (193, 150, 21), (107, 193, 21), (21, 193, 64), (21, 193, 193), (21, 64, 193), (107, 21, 193), (193, 21, 150),
    (142, 11, 11), (142, 109, 11), (77, 142, 11), (11, 142, 44), (11, 142, 142), (11, 44, 142), (76, 11, 142), (142, 11, 109),
    (75, 6, 6), (75, 58, 6), (41, 75, 6), (6, 75, 23), (6, 75, 75), (6, 23, 75), (41, 6, 75), (75, 6, 58),
]

PALETTE_GRAY_COLORS = [
    (36, 36, 36), (73, 73, 73), (109, 109, 109), (146, 146, 146), (182, 182, 182), (219, 219, 219),
]


class GradientSlider(tk.Frame):


    def __init__(self, parent, name, index, minv, maxv, get_values, to_rgb, on_change,
                 width=SLIDER_WIDTH, height=SLIDER_HEIGHT, label_width=2):
        super().__init__(parent)
        self.index = index
        self.minv, self.maxv = minv, maxv
        self.get_values = get_values
        self.to_rgb = to_rgb
        self.on_change = on_change
        self.width, self.height = width, height

        tk.Label(self, text=name, width=label_width, font=("TkDefaultFont", 10, "bold")).grid(row=0, column=0, padx=(0, 4))

        self.canvas = tk.Canvas(
            self, width=self.width, height=self.height,
            highlightthickness=1, highlightbackground="#999999", cursor="hand2",
        )
        self.canvas.grid(row=0, column=1)
        self.canvas.bind("<Button-1>", self._on_mouse)
        self.canvas.bind("<B1-Motion>", self._on_mouse)

        self.value_label = tk.Label(self, text="0", width=4, font=("TkDefaultFont", 10))
        self.value_label.grid(row=0, column=2, padx=(4, 0))

        self._image_ref = None
        self.redraw()

    def _on_mouse(self, event):
        x = max(0, min(self.width - 1, event.x))
        t = self.minv + (x / (self.width - 1)) * (self.maxv - self.minv)
        self.on_change(self.index, t)

    def redraw(self):
        values = list(self.get_values())
        current = values[self.index]

        pixels = []
        for x in range(self.width):
            t = self.minv + (x / (self.width - 1)) * (self.maxv - self.minv)
            values[self.index] = t
            r, g, b = self.to_rgb(*values)
            r = max(0, min(255, round(r)))
            g = max(0, min(255, round(g)))
            b = max(0, min(255, round(b)))
            pixels.append("#{:02x}{:02x}{:02x}".format(r, g, b))

        row = tk.PhotoImage(width=self.width, height=1)
        row.put("{" + " ".join(pixels) + "}", to=(0, 0))
        stretched = row.zoom(1, self.height)
        self._image_ref = stretched

        self.canvas.delete("all")
        self.canvas.create_image(0, 0, anchor="nw", image=stretched)

        handle_x = (current - self.minv) / (self.maxv - self.minv) * (self.width - 1)
        self.canvas.create_line(handle_x, 0, handle_x, self.height, fill="black", width=2)

        self.value_label.config(text=f"{current:.0f}")


class Palette(tk.Frame):


    def __init__(self, parent, colors, on_pick, columns=8, cell=22):
        super().__init__(parent)
        self.on_pick = on_pick

        rows = (len(colors) + columns - 1) // columns
        width = columns * cell
        height = rows * cell

        canvas = tk.Canvas(
            self, width=width, height=height,
            highlightthickness=0, bg=parent["bg"],
        )
        canvas.pack()

        for i, (r, g, b) in enumerate(colors):
            row, col = divmod(i, columns)
            x0, y0 = col * cell, row * cell
            x1, y1 = x0 + cell, y0 + cell
            fill = "#{:02x}{:02x}{:02x}".format(r, g, b)
            rect = canvas.create_rectangle(x0, y0, x1, y1, fill=fill, outline="#888888")
            canvas.tag_bind(
                rect, "<Button-1>",
                lambda e, rgb=(r, g, b): self.on_pick(rgb),
            )


class ModelSection(tk.LabelFrame):


    def __init__(self, parent, title, components, get_values, set_values, to_rgb, on_change):
        super().__init__(parent, text=title, padx=8, pady=8)
        self.components = components
        self.get_values = get_values
        self.set_values = set_values
        self.on_change = on_change

        self.sliders = []
        for i, (name, minv, maxv) in enumerate(components):
            slider = GradientSlider(self, name, i, minv, maxv, get_values, to_rgb, self._on_slider_change)
            slider.grid(row=i, column=0, sticky="w", pady=1)
            self.sliders.append(slider)

        entry_row = tk.Frame(self)
        entry_row.grid(row=len(components), column=0, sticky="w", pady=(8, 4))
        self.entries = []
        for i, (name, minv, maxv) in enumerate(components):
            tk.Label(entry_row, text=name + ":").grid(row=0, column=i * 2, padx=(0 if i == 0 else 6, 2))
            entry = tk.Entry(entry_row, width=5)
            entry.grid(row=0, column=i * 2 + 1)
            entry.bind("<Return>", lambda event: self._commit_entries())
            self.entries.append(entry)
        ttk.Button(entry_row, text="Set", command=self._commit_entries).grid(
            row=0, column=len(components) * 2, padx=(8, 0)
        )

    def _on_slider_change(self, index, new_value):
        values = list(self.get_values())
        values[index] = new_value
        self.set_values(tuple(values))
        self.on_change()

    def _commit_entries(self):
        values = list(self.get_values())
        for i in range(len(self.components)):
            text = self.entries[i].get().strip()
            try:
                values[i] = float(text)
            except ValueError:
                pass
        self.set_values(tuple(values))
        self.on_change()

    def refresh(self):
        for slider in self.sliders:
            slider.redraw()
        values = self.get_values()
        for i, v in enumerate(values):
            self.entries[i].delete(0, tk.END)
            self.entries[i].insert(0, f"{v:.0f}")


class MainWindow(tk.Tk):
    def __init__(self, viewmodel):
        super().__init__()
        self.vm = viewmodel
        self.title("Color Models Lab - RGB / CMYK / HLS (variant 6)")
        self.resizable(False, False)

        self.cmyk_method_var = tk.StringVar(value=self.vm.cmyk_method)
        self.gamut_mode_var = tk.StringVar(value=self.vm.gamut_mode)
        self.illuminant_var = tk.StringVar(value=self.vm.illuminant_name)

        self._build_top_bar()
        self._build_brightness_bar()
        self._build_sections()
        self._build_xyz_lab_bonus()

        self.refresh_all()


    def _build_top_bar(self):
        bar = tk.Frame(self, padx=10, pady=8)
        bar.grid(row=0, column=0, columnspan=3, sticky="ew")

        self.swatch = tk.Canvas(bar, width=70, height=50, highlightthickness=1, highlightbackground="#999999")
        self.swatch.grid(row=0, column=0, rowspan=2, padx=(0, 10))

        self.hex_label = tk.Label(bar, text="", font=("TkDefaultFont", 11, "bold"))
        self.hex_label.grid(row=0, column=1, sticky="w")

        self.warning_label = tk.Label(bar, text="", fg="#b35c00", font=("TkDefaultFont", 9))
        self.warning_label.grid(row=1, column=1, sticky="w")

        tk.Label(bar, text="CMYK method:").grid(row=0, column=2, padx=(20, 4))
        cmyk_menu = ttk.Combobox(bar, textvariable=self.cmyk_method_var, values=["GCR", "UCR"], state="readonly", width=6)
        cmyk_menu.grid(row=0, column=3)
        cmyk_menu.bind("<<ComboboxSelected>>", self._on_cmyk_method_change)

        tk.Label(bar, text="Out-of-range strategy:").grid(row=1, column=2, padx=(20, 4))
        gamut_menu = ttk.Combobox(bar, textvariable=self.gamut_mode_var, values=["clip", "scale"], state="readonly", width=6)
        gamut_menu.grid(row=1, column=3)
        gamut_menu.bind("<<ComboboxSelected>>", self._on_gamut_mode_change)

        tk.Label(bar, text="Lighting standard:").grid(row=0, column=4, padx=(20, 4))
        illum_menu = ttk.Combobox(bar, textvariable=self.illuminant_var, values=["D65", "D50", "E"], state="readonly", width=6)
        illum_menu.grid(row=0, column=5)
        illum_menu.bind("<<ComboboxSelected>>", self._on_illuminant_change)

        ttk.Button(bar, text="Palette...", command=self._open_palette_dialog).grid(
            row=1, column=4, columnspan=2, padx=(20, 0), sticky="w"
        )

    def _on_illuminant_change(self, event):
        self.vm.set_illuminant(self.illuminant_var.get())
        self.refresh_all()

    def _on_cmyk_method_change(self, event):
        self.vm.set_cmyk_method(self.cmyk_method_var.get())
        self.refresh_all()

    def _on_gamut_mode_change(self, event):
        self.vm.set_gamut_mode(self.gamut_mode_var.get())
        self.refresh_all()


    def _build_brightness_bar(self):
        bar = tk.Frame(self, padx=10, pady=4)
        bar.grid(row=1, column=0, columnspan=3, sticky="ew")

        self.brightness_slider = GradientSlider(
            bar, "\U0001F506", 1, 0, 100,
            get_values=self.vm.get_hls,
            to_rgb=self.vm.preview_rgb_for_hls,
            on_change=self._on_brightness_change,
            width=560, height=20, label_width=3,
        )
        self.brightness_slider.grid(row=0, column=0, sticky="w")

    def _on_brightness_change(self, index, new_value):
        h, l, s = self.vm.get_hls()
        self.vm.set_from_hls(h, new_value, s)
        self.refresh_all()


    def _build_sections(self):
        self.rgb_section = ModelSection(
            self, "RGB", [("R", 0, 255), ("G", 0, 255), ("B", 0, 255)],
            get_values=self.vm.get_rgb,
            set_values=lambda v: self.vm.set_from_rgb(*v),
            to_rgb=self.vm.preview_rgb_for_rgb,
            on_change=self.refresh_all,
        )
        self.rgb_section.grid(row=2, column=0, sticky="n", padx=10, pady=10)

        self.cmyk_section = ModelSection(
            self, "CMYK", [("C", 0, 100), ("M", 0, 100), ("Y", 0, 100), ("K", 0, 100)],
            get_values=self.vm.get_cmyk,
            set_values=lambda v: self.vm.set_from_cmyk(*v),
            to_rgb=self.vm.preview_rgb_for_cmyk,
            on_change=self.refresh_all,
        )
        self.cmyk_section.grid(row=2, column=1, sticky="n", padx=10, pady=10)

        self.hls_section = ModelSection(
            self, "HLS", [("H", 0, 360), ("L", 0, 100), ("S", 0, 100)],
            get_values=self.vm.get_hls,
            set_values=lambda v: self.vm.set_from_hls(*v),
            to_rgb=self.vm.preview_rgb_for_hls,
            on_change=self.refresh_all,
        )
        self.hls_section.grid(row=2, column=2, sticky="n", padx=10, pady=10)

    def _pick_from_palette(self, rgb):
        self.vm.set_from_rgb(*rgb)
        self.refresh_all()


    def _open_palette_dialog(self):
        if getattr(self, "_palette_dialog", None) is not None and self._palette_dialog.winfo_exists():
            self._palette_dialog.lift()
            return

        import colorsys

        dialog = tk.Toplevel(self)
        dialog.title("Palette")
        dialog.resizable(False, False)
        self._palette_dialog = dialog

        def pick(rgb):
            self._pick_from_palette(rgb)
            dialog.destroy()


        left = tk.Frame(dialog)
        left.grid(row=0, column=0, sticky="nw", padx=10, pady=10)

        tk.Label(left, text="Colors:", anchor="w").grid(row=0, column=0, sticky="w")
        Palette(left, PALETTE_HUE_COLORS, pick, columns=8).grid(row=1, column=0, sticky="w")

        tk.Label(left, text="Grays:", anchor="w").grid(row=2, column=0, sticky="w", pady=(10, 2))
        Palette(left, PALETTE_GRAY_COLORS, pick, columns=6).grid(row=3, column=0, sticky="w")

        sep = tk.Frame(left, height=2, bd=1, relief="sunken")
        sep.grid(row=4, column=0, sticky="ew", pady=12)

        bw = tk.Frame(left)
        bw.grid(row=5, column=0, sticky="w")
        tk.Label(bw, text="Black / White:", anchor="w").grid(row=0, column=0, columnspan=2, sticky="w")

        bw_canvas = tk.Canvas(
            bw, width=90, height=48,
            highlightthickness=0, bg=bw["bg"],
        )
        bw_canvas.grid(row=1, column=0, sticky="w", pady=4)


        bw_canvas.create_rectangle(0, 0, 40, 48, fill="#000000", outline="#888888")
        bw_canvas.tag_bind(
            bw_canvas.find_all()[-1], "<Button-1>",
            lambda e: pick((0, 0, 0)),
        )


        bw_canvas.create_rectangle(50, 0, 90, 48, fill="#ffffff", outline="#888888")
        bw_canvas.tag_bind(
            bw_canvas.find_all()[-1], "<Button-1>",
            lambda e: pick((255, 255, 255)),
        )


        HSV_SIZE = 180
        BAR_W = 22

        right = tk.Frame(dialog)
        right.grid(row=0, column=1, sticky="nw", padx=(10, 10), pady=10)

        hsv_canvas = tk.Canvas(
            right, width=HSV_SIZE, height=HSV_SIZE,
            highlightthickness=1, highlightbackground="#888888", cursor="crosshair",
        )
        hsv_canvas.grid(row=0, column=0, sticky="nw")

        bar_canvas = tk.Canvas(
            right, width=BAR_W, height=HSV_SIZE,
            highlightthickness=1, highlightbackground="#888888", cursor="hand2",
        )
        bar_canvas.grid(row=0, column=1, sticky="nw", padx=(6, 0))

        state = {"h": 0.0, "s": 1.0, "v": 1.0}

        def hsv_to_hex(h, s, v):
            r, g, b = colorsys.hsv_to_rgb(h, s, v)
            return "#{:02x}{:02x}{:02x}".format(int(r * 255), int(g * 255), int(b * 255))

        def redraw_hsv():
            img = tk.PhotoImage(width=HSV_SIZE, height=HSV_SIZE)
            rows = []
            for y in range(HSV_SIZE):
                sat = y / (HSV_SIZE - 1)
                row = []
                for x in range(HSV_SIZE):
                    hue = x / (HSV_SIZE - 1)
                    row.append(hsv_to_hex(hue, sat, state["v"]))
                rows.append("{" + " ".join(row) + "}")
            img.put(" ".join(rows), to=(0, 0))
            hsv_canvas._img = img
            hsv_canvas.delete("all")
            hsv_canvas.create_image(0, 0, anchor="nw", image=img)
            cx = state["h"] * (HSV_SIZE - 1)
            cy = state["s"] * (HSV_SIZE - 1)
            hsv_canvas.create_oval(cx - 5, cy - 5, cx + 5, cy + 5, outline="white", width=2)
            hsv_canvas.create_oval(cx - 6, cy - 6, cx + 6, cy + 6, outline="black", width=1)

        def redraw_bar():
            img = tk.PhotoImage(width=BAR_W, height=HSV_SIZE)
            rows = []
            for y in range(HSV_SIZE):
                v = 1.0 - y / (HSV_SIZE - 1)
                color = hsv_to_hex(state["h"], state["s"], v)
                rows.append("{" + " ".join([color] * BAR_W) + "}")
            img.put(" ".join(rows), to=(0, 0))
            bar_canvas._img = img
            bar_canvas.delete("all")
            bar_canvas.create_image(0, 0, anchor="nw", image=img)
            cy = (1.0 - state["v"]) * (HSV_SIZE - 1)
            bar_canvas.create_line(0, cy, BAR_W, cy, fill="white", width=3)
            bar_canvas.create_line(0, cy, BAR_W, cy, fill="black", width=1)

        def on_hsv_click(event):
            x = max(0, min(HSV_SIZE - 1, event.x))
            y = max(0, min(HSV_SIZE - 1, event.y))
            state["h"] = x / (HSV_SIZE - 1)
            state["s"] = y / (HSV_SIZE - 1)
            redraw_hsv()
            redraw_bar()

        def on_bar_click(event):
            y = max(0, min(HSV_SIZE - 1, event.y))
            state["v"] = 1.0 - y / (HSV_SIZE - 1)
            redraw_hsv()
            redraw_bar()

        hsv_canvas.bind("<Button-1>", on_hsv_click)
        hsv_canvas.bind("<B1-Motion>", on_hsv_click)
        bar_canvas.bind("<Button-1>", on_bar_click)
        bar_canvas.bind("<B1-Motion>", on_bar_click)


        def apply_picked():
            r, g, b = colorsys.hsv_to_rgb(state["h"], state["s"], state["v"])
            pick((int(r * 255), int(g * 255), int(b * 255)))

        ttk.Button(right, text="Выбрать", command=apply_picked, width=12).grid(
            row=1, column=0, columnspan=2, pady=(10, 0)
        )


        redraw_hsv()
        redraw_bar()


        ttk.Button(dialog, text="Close", command=dialog.destroy).grid(
            row=1, column=0, columnspan=2, pady=(0, 10)
        )

    def _build_xyz_lab_bonus(self):
        frame = tk.LabelFrame(self, text="XYZ & Lab (subgroup 10A, addition #2 - illuminant-aware)", padx=10, pady=8)
        frame.grid(row=3, column=0, columnspan=3, sticky="ew", padx=10, pady=(0, 10))

        self.xyz_label = tk.Label(frame, text="", anchor="w", justify="left")
        self.xyz_label.grid(row=0, column=0, sticky="w")

        self.lab_label = tk.Label(frame, text="", anchor="w", justify="left")
        self.lab_label.grid(row=1, column=0, sticky="w")

        demo = tk.Frame(frame)
        demo.grid(row=0, column=1, rowspan=2, padx=(30, 0))

        tk.Label(demo, text="Try an out-of-gamut Lab -> RGB:").grid(row=0, column=0, columnspan=8, sticky="w")

        self.lab_entries = {}
        for col, name in enumerate(("L*", "a*", "b*")):
            tk.Label(demo, text=name + ":").grid(row=1, column=col * 2, sticky="e")
            entry = tk.Entry(demo, width=6)
            entry.insert(0, {"L*": "50", "a*": "100", "b*": "-120"}[name])
            entry.grid(row=1, column=col * 2 + 1, padx=(2, 8))
            self.lab_entries[name] = entry

        ttk.Button(demo, text="Convert", command=self._run_lab_demo).grid(row=1, column=6, padx=(4, 6))
        self.lab_demo_swatch = tk.Canvas(demo, width=30, height=20, highlightthickness=1, highlightbackground="#999999")
        self.lab_demo_swatch.grid(row=1, column=7)

        self.lab_demo_result = tk.Label(demo, text="", anchor="w")
        self.lab_demo_result.grid(row=2, column=0, columnspan=8, sticky="w", pady=(4, 0))

    def _run_lab_demo(self):
        try:
            L = float(self.lab_entries["L*"].get())
            a = float(self.lab_entries["a*"].get())
            b = float(self.lab_entries["b*"].get())
        except ValueError:
            self.lab_demo_result.config(text="Please enter valid numbers.")
            return

        mode = self.gamut_mode_var.get()
        (r, g, b_), changed = self.vm.convert_lab_to_rgb_preview(L, a, b, mode)
        self.lab_demo_swatch.configure(bg="#{:02x}{:02x}{:02x}".format(r, g, b_))
        note = "  (was out of range - adjusted)" if changed else ""
        self.lab_demo_result.config(text=f"Lab({L:g},{a:g},{b:g}) --[{mode}]--> RGB({r},{g},{b_}){note}")


    def refresh_all(self):
        self.swatch.configure(bg=self.vm.get_hex())
        r, g, b = self.vm.get_rgb()
        self.hex_label.config(text=f"RGB({r}, {g}, {b})   {self.vm.get_hex()}")
        self.warning_label.config(text=self.vm.last_warning)

        self.brightness_slider.redraw()
        self.rgb_section.refresh()
        self.cmyk_section.refresh()
        self.hls_section.refresh()

        X, Y, Z = self.vm.get_xyz()
        self.xyz_label.config(text=f"XYZ ({self.vm.illuminant_name}):  X={X:.2f}  Y={Y:.2f}  Z={Z:.2f}")
        L, a, bb = self.vm.get_lab()
        self.lab_label.config(text=f"Lab ({self.vm.illuminant_name}):  L*={L:.2f}  a*={a:.2f}  b*={bb:.2f}")
