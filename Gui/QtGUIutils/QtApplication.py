from PyQt5.QtCore import QDateTime, Qt, QTimer
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import sys
import os

from Gui.GUIutils.DBConnection import *

class QtApplication(QWidget):
	def __init__(self):
		super(QtApplication,self).__init__()
		self.setLoginUI()
		self.createLogin()

		mainLayout = QGridLayout()
		mainLayout.addWidget(self.LoginGroupBox, 0, 0)
		self.setLayout(mainLayout)

	
	def setLoginUI(self):
		self.setGeometry(300, 300, 600, 600)  
		#设置窗口的标题
		self.setWindowTitle('Phase2 Pixel Module Test GUI')  
		QApplication.setStyle(QStyleFactory.create('Fusion'))
		QApplication.setPalette(QApplication.style().standardPalette())
		#显示窗口
		self.show()

	def createLogin(self):
		self.LoginGroupBox = QGroupBox("")
		self.LoginGroupBox.setCheckable(False)

		TitleLabel = QLabel('<font size="12"> Phase2 Pixel Module Test </font>')
		TitleLabel.setMaximumHeight(30)

		UsernameLabel = QLabel("Username:")
		self.UsernameEdit = QLineEdit('')
		self.UsernameEdit.setEchoMode(QLineEdit.Normal)
		self.UsernameEdit.setPlaceholderText('Your username')
		self.UsernameEdit.setMinimumWidth(150)
		self.UsernameEdit.setMaximumHeight(30)

		PasswordLabel = QLabel("Password:")
		self.PasswordEdit = QLineEdit('')
		self.PasswordEdit.setEchoMode(QLineEdit.Password)
		self.PasswordEdit.setPlaceholderText('Your password')
		self.PasswordEdit.setMinimumWidth(150)
		self.PasswordEdit.setMaximumHeight(30)

		HostLabel = QLabel("HostName:")
		self.HostEdit = QLineEdit('128.146.38.1')
		self.HostEdit.setEchoMode(QLineEdit.Normal)
		self.HostEdit.setMinimumWidth(150)
		self.HostEdit.setMaximumHeight(30)

		DatabaseLabel = QLabel("Database:")
		self.DatabaseEdit = QLineEdit('phase2pixel_test')
		self.DatabaseEdit.setEchoMode(QLineEdit.Normal)
		self.DatabaseEdit.setMinimumWidth(150)
		self.DatabaseEdit.setMaximumHeight(30)

		disableCheckBox = QCheckBox("&Do not connect to DB")
		disableCheckBox.toggled.connect(self.HostEdit.setDisabled)
		disableCheckBox.toggled.connect(self.DatabaseEdit.setDisabled)

		button_login = QPushButton("&Login")
		button_login.clicked.connect(self.checkLogin)

		layout = QGridLayout()
		layout.addWidget(TitleLabel,0,1,1,2,Qt.AlignCenter)
		layout.addWidget(UsernameLabel,1,1,1,1,Qt.AlignRight)
		layout.addWidget(self.UsernameEdit,1,2,1,1)
		layout.addWidget(PasswordLabel,2,1,1,1,Qt.AlignRight)
		layout.addWidget(self.PasswordEdit,2,2,1,1)
		layout.addWidget(HostLabel,3,1,1,1,Qt.AlignRight)
		layout.addWidget(self.HostEdit,3,2,1,1)
		layout.addWidget(DatabaseLabel,4,1,1,1,Qt.AlignRight)
		layout.addWidget(self.DatabaseEdit,4,2,1,1)
		layout.addWidget(disableCheckBox,5,2,1,1,Qt.AlignLeft)
		layout.addWidget(button_login,6,1,1,2)
		layout.setRowMinimumHeight(6, 50)

		layout.setColumnStretch(0, 1)
		layout.setColumnStretch(1, 1)
		layout.setColumnStretch(2, 2)
		layout.setColumnStretch(3, 1)
		self.LoginGroupBox.setLayout(layout)
	
	def checkLogin(self):
		msg = QMessageBox()

		TryUsername = self.UsernameEdit.text()
		TryPassword = self.PasswordEdit.text()
		TryHostAddress = self.HostEdit.text()
		TryDatabase = self.DatabaseEdit.text()

		if TryUsername == '':
			msg.information(None,"Error","Please enter a valid username", QMessageBox.Ok)
			msg.exec_()
			return
		
		connection = QtStartConnection(TryUsername, TryPassword, TryHostAddress, TryDatabase)

		if connection:
			pass

		