from PyQt5 import QtCore
from PyQt5.QtCore import Qt
#from PyQt5.QtGui import QPixmap
from PyQt5.QtWidgets import (
    QAbstractItemView,
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
    QTableView,
    QTableWidget,
    QTabWidget,
    QTextEdit,
    QHBoxLayout,
    QVBoxLayout,
    QWidget,
    QMainWindow,
    QMessageBox,
    QSplitter,
)

import sys
import os
import subprocess
from subprocess import Popen, PIPE

# from Gui.GUIutils.settings import dblist
from Gui.GUIutils.DBConnection import (
    QtStartConnection,
    isActive,
)
from Gui.python.logging_config import logger


class QtLoginDialog(QDialog):
    def __init__(self, username, password, address, database, connection):
        super(QtLoginDialog, self).__init__()
        self.expertMode = False
        self.connection = connection
        self.TryUsername = username
        self.TryPassword = password
        self.TryHostAddress = address
        self.TryDatabase = database

        self.mainLayout = QGridLayout()

        self.createLogin()
        self.setLayout(self.mainLayout)

    def createLogin(self):
        self.LoginGroupBox = QGroupBox("")
        self.LoginGroupBox.setCheckable(False)

        UsernameLabel = QLabel("Username:")
        self.UsernameEdit = QLineEdit("")
        self.UsernameEdit.setEchoMode(QLineEdit.Normal)
        self.UsernameEdit.setPlaceholderText("Your username")
        self.UsernameEdit.setMinimumWidth(150)
        self.UsernameEdit.setMaximumHeight(30)

        PasswordLabel = QLabel("Password:")
        self.PasswordEdit = QLineEdit("")
        self.PasswordEdit.setEchoMode(QLineEdit.Password)
        self.PasswordEdit.setPlaceholderText("Your password")
        self.PasswordEdit.setMinimumWidth(150)
        self.PasswordEdit.setMaximumHeight(30)

        HostLabel = QLabel("HostName:")

        if not self.expertMode:
            self.HostName = QComboBox()
            # self.HostName.addItems(dblist)
            # self.HostName.currentIndexChanged.connect(self.changeDBList)
            HostLabel.setBuddy(self.HostName)
        else:
            self.HostEdit = QLineEdit("128.146.38.1")
            self.HostEdit.setEchoMode(QLineEdit.Normal)
            self.HostEdit.setMinimumWidth(150)
            self.HostEdit.setMaximumHeight(30)

        DatabaseLabel = QLabel("Database:")
        if self.expertMode:
            self.DatabaseCombo = QComboBox()
            self.DBNames = eval(self.HostName.currentText() + ".All_list")
            self.DatabaseCombo.addItems(self.DBNames)
            self.DatabaseCombo.setCurrentIndex(0)
        else:
            self.DatabaseEdit = QLineEdit("phase2pixel_test")
            self.DatabaseEdit.setEchoMode(QLineEdit.Normal)
            self.DatabaseEdit.setMinimumWidth(150)
            self.DatabaseEdit.setMaximumHeight(30)

        self.expertCheckBox = QCheckBox("&Manual Entry")
        self.expertCheckBox.setMaximumHeight(30)
        self.expertCheckBox.setChecked(self.expertMode)
        self.expertCheckBox.clicked.connect(self.switchMode)

        button_login = QPushButton("&Login")
        button_login.setDefault(True)
        button_login.clicked.connect(self.checkLogin)

        layout = QGridLayout()
        layout.setSpacing(20)
        layout.addWidget(UsernameLabel, 0, 1, 1, 1, Qt.AlignRight)
        layout.addWidget(self.UsernameEdit, 0, 2, 1, 2)
        layout.addWidget(PasswordLabel, 1, 1, 1, 1, Qt.AlignRight)
        layout.addWidget(self.PasswordEdit, 1, 2, 1, 2)
        layout.addWidget(HostLabel, 2, 1, 1, 1, Qt.AlignRight)
        if self.expertMode:
            layout.addWidget(self.HostEdit, 2, 2, 1, 2)
        else:
            layout.addWidget(self.HostName, 2, 2, 1, 2)
        layout.addWidget(DatabaseLabel, 3, 1, 1, 1, Qt.AlignRight)
        if self.expertMode:
            layout.addWidget(self.DatabaseEdit, 3, 2, 1, 2)
        else:
            layout.addWidget(self.DatabaseCombo, 3, 2, 1, 2)
        layout.addWidget(self.expertCheckBox, 4, 2, 1, 1, Qt.AlignRight)
        layout.addWidget(button_login, 5, 1, 1, 3)
        layout.setRowMinimumHeight(5, 50)

        layout.setColumnStretch(0, 1)
        layout.setColumnStretch(1, 1)
        layout.setColumnStretch(2, 2)
        layout.setColumnStretch(3, 1)
        self.LoginGroupBox.setLayout(layout)
        self.mainLayout.addWidget(self.LoginGroupBox, 0, 0)

    def destroyLogin(self):
        self.LoginGroupBox.deleteLater()
        self.mainLayout.removeWidget(self.LoginGroupBox)

    def checkLogin(self):
        expertList = ["mjoyce", "kwei"]  # temporary fix for demonstration
        if self.UsernameEdit.text() in expertList:
            self.expertMode = True
        if self.expertMode:
            self.TryUsername = self.UsernameEdit.text()
            self.TryPassword = self.PasswordEdit.text()
            # self.TryHostAddress = DBServerIP[str(self.HostName.currentText())]
            self.TryHostAddress = eval(self.HostName.currentText() + ".DBIP")
            self.TryDatabase = str(self.DatabaseCombo.currentText())
        # 			self.TryHostAddress = self.HostEdit.text()
        # 			self.TryDatabase = self.DatabaseEdit.text()
        else:
            self.TryUsername = self.UsernameEdit.text()
            self.TryPassword = self.PasswordEdit.text()
            self.TryHostAddress = eval(self.HostName.currentText() + ".DBIP")
            self.TryDatabase = str(self.DatabaseCombo.currentText())

        self.connection = QtStartConnection(
            self.TryUsername, self.TryPassword, self.TryHostAddress, self.TryDatabase
        )

        if isActive(self.connection):
            self.accept()
        else:
            QMessageBox().information(
                None, "Error", "Database not connected", QMessageBox.Ok
            )

    def switchMode(self):
        if self.expertMode:
            self.expertMode = False
        else:
            self.expertMode = True
        self.destroyLogin()
        self.createLogin()

    def changeDBList(self):
        self.DBNames = eval(self.HostName.currentText() + ".DBName")
        self.DatabaseCombo.clear()
        self.DatabaseCombo.addItems(self.DBNames)
        self.DatabaseCombo.setCurrentIndex(0)
