from PyQt5 import QtCore
from PyQt5.QtCore import Qt, QSize
from PyQt5.QtGui import QFont, QPixmap, QPalette, QImage
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
import io
from PIL.ImageQt import ImageQt
from Gui.python.logging_config import logger


class QtImageViewer(QWidget):
    def __init__(self, master, data):
        super(QtImageViewer, self).__init__()
        self.master = master
        self.module_id = data[0]
        self.caption = data[1]
        self.data = data[2]
        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)
        self.setLoginUI()
        self.produceImg()
        self.createMain()

    def setLoginUI(self):
        self.setGeometry(400, 400, 400, 600)
        self.setWindowTitle(self.caption)
        self.show()

    def produceImg(self):
        self.image = QImage()
        self.image.loadFromData(self.data)
        self.image.scaled(QSize(300, 300), Qt.KeepAspectRatio, Qt.SmoothTransformation)
        # image = Image.open(io.BytesIO(self.data))
        # image.save("/Users/czkaiweb/Desktop/test.jpg")

    def createMain(self):
        self.MainOption = QGroupBox()

        ImageLayout = QGridLayout()

        ImageView = QLabel()
        ImageMap = QPixmap.fromImage(self.image)
        ImageView.setPixmap(ImageMap)

        ImageLayout.addWidget(ImageView)
        self.MainOption.setLayout(ImageLayout)

        self.AppOption = QGroupBox()
        self.AppLayout = QHBoxLayout()

        self.CloseButton = QPushButton("&Close")
        self.CloseButton.clicked.connect(self.closeWindow)

        self.AppLayout.addStretch(1)
        self.AppLayout.addWidget(self.CloseButton)
        self.AppOption.setLayout(self.AppLayout)

        self.mainLayout.addWidget(self.MainOption)
        self.mainLayout.addWidget(self.AppOption)

    def closeWindow(self):
        self.close()
