import sys
import os
from PyQt5 import QtCore
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
from Gui.python.logging_config import logger


class QtFwCheckWindow(QWidget):
    def __init__(self, master, fileName):
        super(QtFwCheckWindow, self).__init__()
        self.master = master
        self.fileName = fileName
        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)
        self.setLoginUI()
        self.createMain()
        self.occupied()
        self.run_process = QProcess(self)
        self.run_process.readyReadStandardOutput.connect(
            self.on_readyReadStandardOutput
        )
        self.run_process.start("tail", ["-f", "-n", "999", self.fileName])
        # self.run_process.waitForFinished()

    def setLoginUI(self):
        self.setGeometry(400, 400, 400, 600)
        self.setWindowTitle("Firmware Status Check")
        self.show()

    def createMain(self):
        self.MainOption = QGroupBox("Log: {0}".format(self.fileName))

        ConsoleLayout = QGridLayout()

        self.ConsoleView = QTextEdit()
        self.ConsoleView.setStyleSheet(
            "QTextEdit { background-color: rgb(10, 10, 10); color : white; }"
        )

        ConsoleLayout.addWidget(self.ConsoleView)

        self.MainOption.setLayout(ConsoleLayout)

        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()

        self.CloseButton = QPushButton("&Close")
        self.CloseButton.clicked.connect(self.release)
        self.CloseButton.clicked.connect(self.closeWindow)

        self.StartLayout.addStretch(1)
        self.StartLayout.addWidget(self.CloseButton)
        self.AppOption.setLayout(self.StartLayout)

        self.mainLayout.addWidget(self.MainOption)
        self.mainLayout.addWidget(self.AppOption)

    def destroyMain(self):
        self.MainOption.deleteLater()
        self.AppOption.deleteLater()
        self.mainLayout.removeWidget(self.MainOption)
        self.mainLayout.removeWidget(self.AppOption)

    def closeWindow(self):
        self.run_process.kill()
        self.close()

    def occupied(self):
        pass

    def release(self):
        pass

    @QtCore.pyqtSlot()
    def on_readyReadStandardOutput(self):
        text = self.run_process.readAllStandardOutput().data().decode()
        self.ConsoleView.append(text)
