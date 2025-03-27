from PyQt5.QtWidgets import QApplication

import sys

from Gui.QtGUIutils.QtApplication import QtApplication

a = 4
if __name__ == "__main__":
    app = QApplication([])
    dimension = app.screens()[0].size()
    qtApp = QtApplication(dimension)
    sys.exit(app.exec_())
