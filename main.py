"""
main.py
=======

Entry point. This file's only job is to create the ViewModel, hand it
to the View, and start the window - it's the "wiring", nothing else.

Run with:  python main.py
(Tkinter ships with Python already, so there is nothing extra to
install on Windows/macOS. On some Linux distros you may need to
`sudo apt install python3-tk` first.)
"""

from viewmodel.color_viewmodel import ColorViewModel
from view.main_window import MainWindow

if __name__ == "__main__":
    viewmodel = ColorViewModel()
    window = MainWindow(viewmodel)
    window.mainloop()
