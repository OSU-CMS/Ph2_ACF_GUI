from PyQt5.QtCore import *
from PyQt5.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QDateTimeEdit,
    QDial,
    QDialog,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QProgressBar,
    QPushButton,
    QRadioButton,
    QScrollBar,
    QSizePolicy,
    QSlider,
    QSpinBox,
    QStyleFactory,
    QTableWidget,
    QTabWidget,
    QTextEdit,
    QHBoxLayout,
    QVBoxLayout,
    QWidget,
    QMainWindow,
    QMessageBox,
)

import sys
import os
import math

from Gui.python.CustomizedWidget import StatusBox
#from Gui.GUIutils.DBConnection import *
#from Gui.GUIutils.FirmwareUtil import *
#from Gui.GUIutils.settings import *
from Gui.python.logging_config import logger


#!!!!!!!!!!!!!!!!!!!!DEVELOPMENT required!!!!!!!!!!!!!!!!!!!!
class QtPowerSupplyConfig(QWidget):
    closedSignal = pyqtSignal()

    def __init__(self, master, config):
        super(QtPowerSupplyConfig, self).__init__()
        self.master = master

        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)

        self.setLoginUI()
        self.createMain()
        self.createApp()

    def setLoginUI(self):
        self.setGeometry(400, 400, 400, 400)
        self.setWindowTitle("Power Supply Configuration")
        # QApplication.setStyle(QStyleFactory.create('macintosh'))
        # QApplication.setPalette(QApplication.style().standardPalette())
        # QApplication.setPalette(QApplication.palette())
        self.show()

    def createMain(self):
        self.firmwareCheckBox = QGroupBox()
        firmwarePar = QGridLayout()
        self.FEStatusDict = {}
        for i, value in enumerate(self.verboseResult.values()):
            ChipStatusBox = StatusBox(value, i)
            self.FEStatusDict[i] = ChipStatusBox
            firmwarePar.addWidget(
                ChipStatusBox, math.floor(i / 2), math.ceil(i % 2 / 2), 1, 1
            )

        self.firmwareCheckBox.setLayout(firmwarePar)
        self.mainLayout.addWidget(self.firmwareCheckBox, 0, 0)

    def destroyMain(self):
        self.firmwareCheckBox.deleteLater()
        self.mainLayout.removeWidget(self.firmwareCheckBox)

    def createApp(self):
        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()

        self.CloseButton = QPushButton("&Close")
        self.CloseButton.clicked.connect(self.closeWindow)

        self.StartLayout.addStretch(1)
        self.StartLayout.addWidget(self.CloseButton)
        self.AppOption.setLayout(self.StartLayout)

        self.mainLayout.addWidget(self.AppOption, 2, 0)

    def closeWindow(self):
        self.close()

    def checkFwPar(self):
        GlobalCheck = True
        for key, item in self.FEStatusDict.items():
            GlobalCheck = GlobalCheck and item.checkFwPar()
        return GlobalCheck

    def closeEvent(self, event):
        self.closedSignal.emit()
