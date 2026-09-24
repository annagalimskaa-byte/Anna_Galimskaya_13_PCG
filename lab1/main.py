
from viewmodel.color_viewmodel import ColorViewModel
from view.main_window import MainWindow

if __name__ == "__main__":
    viewmodel = ColorViewModel()
    window = MainWindow(viewmodel)
    window.mainloop()