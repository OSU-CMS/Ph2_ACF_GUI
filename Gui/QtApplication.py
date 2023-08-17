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
import logging

if __name__ == "__main__":
    logging.basicConfig(
        filename="Ph2_ACF_GUI_logfile.log",
        level=logging.DEBUG,
        format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    )
    palette = QPalette()
    QApplication.setStyle(QStyleFactory.create("Fusion"))
    QApplication.setPalette(apply_dark_mode(palette))
    app = QApplication([])
    application = QtApplication()

    sys.exit(app.exec_())
