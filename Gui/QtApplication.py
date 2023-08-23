import logging

logging.basicConfig(
    format="%(asctime)s - %(filename)s - %(funcName)s: %(message)s",
    level=logging.INFO,
    handlers=[
        logging.FileHandler("Ph2_ACF_GUI.log", mode="w"),
        logging.StreamHandler(),
    ],
)
from PyQt5.QtWidgets import QApplication
from Gui.QtGUIutils.QtApplication import QtApplication
import sys
import os

if __name__ == "__main__":
    app = QApplication([])
    dimension = app.screens()[0].size()
    qtApp = QtApplication(dimension)
    sys.exit(app.exec_())
