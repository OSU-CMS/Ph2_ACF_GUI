#from PyQt5 import QtCore
#from PyQt5.QtCore import *
#from PyQt5.QtGui import QFont, QPixmap, QPalette, QImage, QIcon
from PyQt5.QtWidgets import (
    QApplication,
    QButtonGroup,
    QCheckBox,
    QComboBox,
    QDateTimeEdit,
    QDial,
    QDialog,
    QFormLayout,
    QFrame,
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
from PyQt5 import QtSvg

import sys
import os
import re
from Gui.python.logging_config import logger


class QResultDialog(QDialog):
    def __init__(self, parent=None, image=None):
        super(QResultDialog, self).__init__()
        self.DisplayW = 600
        self.DisplayH = 800
        self.setWindowTitle("Please check the result")
        self.layout = QGridLayout()
        self.setupUI(image)
        self.setLayout(self.layout)
        self.show()

    def setupUI(self, imagefile):
        # self.setGeometry(300, 300, 600, 600)
        self.DisplayLabel = QLabel()
        self.DisplayLabel.setScaledContents(True)
        # image = QImage(imagefile).scaled(QSize(300,300), Qt.KeepAspectRatio, Qt.SmoothTransformation)
        # pixmap = QPixmap.fromImage(image)
        self.DisplayView = QtSvg.QSvgWidget()
        self.DisplayView.load(imagefile)
        # self.DisplayView = QPixmap(imagefile).scaled(QSize(self.DisplayW,self.DisplayH), Qt.KeepAspectRatio, Qt.SmoothTransformation)
        # self.DisplayLabel.setPixmap(self.DisplayView)
        self.YesButtom = QPushButton("&Pass")
        self.YesButtom.clicked.connect(self.accept)
        self.NoButtom = QPushButton("&Failed")
        self.NoButtom.clicked.connect(self.reject)
        # self.layout.addWidget(self.DisplayLabel,0,0,6,6)
        self.layout.addWidget(self.DisplayView, 0, 0, 6, 6)
        self.layout.addWidget(self.YesButtom, 6, 5, 1, 1)
        self.layout.addWidget(self.NoButtom, 6, 4, 1, 1)
