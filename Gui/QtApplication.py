import os
os.environ["QT_QPA_PLATFORM"] = "xcb"
os.environ["QT_XCB_GL_INTEGRATION"] = "none"
os.environ["LIBGL_ALWAYS_SOFTWARE"] = "1"
os.environ["XDG_RUNTIME_DIR"] = "/tmp"

from PyQt5.QtWidgets import QApplication

import sys

from Gui.QtGUIutils.QtApplication import QtApplication

if __name__ == "__main__":
    app = QApplication([])
    dimension = app.screens()[0].size()
    qtApp = QtApplication(dimension)
    sys.exit(app.exec_())
