"""
Use to launch in to Ph2_ACF GUI.

Will launch QtLoginDialog which will then connect to the simplified
GUI or the expert GUI. The rest of the execution of the code will
continue from these widgets.
"""
from PyQt5.QtWidgets import QApplication, QWidget, QMessageBox, QStyleFactory
from PyQt5.QtGui import QPalette
from Gui.QtGUIutils.QtApplication import QtApplication
import sys
import os
import traceback
from Gui.QtGUIutils.QtLoginDialog import QtLoginDialog
from Gui.GUIutils.DBConnection import connect_to_database
from Gui.QtGUIutils.QtExpertWindow import QtExpertWindow
import Gui.Config.siteConfig as site_config
from Gui.python.WindowAppearanceTools import apply_dark_mode


# from Gui.QtGUIutils.QtSimplifiedWindow import QtSimplifiedWindow
def custom_excepthook(type, value, traceback):
    # Handle the exception here in a custom way
    print("An error occurred:", value)
    print(traceback)
    sys.exit(1)
    # Optionally, you can log the error or display a user-friendly message


# Set the custom excepthook
sys.excepthook = custom_excepthook


if __name__ == "__main__":
    palette = QPalette()
    QApplication.setStyle(QStyleFactory.create("Fusion"))
    QApplication.setPalette(apply_dark_mode(palette))
    app = QApplication([])
    application = QtApplication()

    sys.exit(app.exec_())
