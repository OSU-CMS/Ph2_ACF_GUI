from PyQt5.QtCore import *
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import sys
import os

from Gui.GUIutils.DBConnection import *

class QtApplication(QWidget):
	def __init__(self):
		super(QtApplication,self).__init__()
		self.mainLayout = QGridLayout()
		self.setLayout(self.mainLayout)

		self.setLoginUI()
		self.createLogin()

	def setLoginUI(self):
		self.setGeometry(300, 300, 400, 400)  
		self.setWindowTitle('Phase2 Pixel Module Test GUI')  
		QApplication.setStyle(QStyleFactory.create('Fusion'))
		QApplication.setPalette(QApplication.style().standardPalette())
		#QApplication.setPalette(QApplication.palette())
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

		self.disableCheckBox = QCheckBox("&Do not connect to DB")
		self.disableCheckBox.toggled.connect(self.HostEdit.setDisabled)
		self.disableCheckBox.toggled.connect(self.DatabaseEdit.setDisabled)

		button_login = QPushButton("&Login")
		button_login.setDefault(True)
		button_login.clicked.connect(self.checkLogin)

		layout = QGridLayout()
		layout.setSpacing(20)
		layout.addWidget(TitleLabel,0,1,1,2,Qt.AlignCenter)
		layout.addWidget(UsernameLabel,1,1,1,1,Qt.AlignRight)
		layout.addWidget(self.UsernameEdit,1,2,1,1)
		layout.addWidget(PasswordLabel,2,1,1,1,Qt.AlignRight)
		layout.addWidget(self.PasswordEdit,2,2,1,1)
		layout.addWidget(HostLabel,3,1,1,1,Qt.AlignRight)
		layout.addWidget(self.HostEdit,3,2,1,1)
		layout.addWidget(DatabaseLabel,4,1,1,1,Qt.AlignRight)
		layout.addWidget(self.DatabaseEdit,4,2,1,1)
		layout.addWidget(self.disableCheckBox,5,2,1,1,Qt.AlignLeft)
		layout.addWidget(button_login,6,1,1,2)
		layout.setRowMinimumHeight(6, 50)

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
		msg = QMessageBox()

		self.TryUsername = self.UsernameEdit.text()
		self.TryPassword = self.PasswordEdit.text()
		self.TryHostAddress = self.HostEdit.text()
		self.TryDatabase = self.DatabaseEdit.text()

		if self.TryUsername == '':
			msg.information(None,"Error","Please enter a valid username", QMessageBox.Ok)
			return
		if self.disableCheckBox.isChecked() == False:
			print("Connect to database...")
			self.connection = QtStartConnection(self.TryUsername, self.TryPassword, self.TryHostAddress, self.TryDatabase)

			if self.connection:
				self.destroyLogin()
				self.createMain()
		else:
			self.connection = "Offline"
			self.destroyLogin()
			self.createMain()
	
	def createMain(self):
		statusString, colorString = checkDBConnection(self.connection)
		self.MainOption = QGroupBox("Hello,{}!".format(self.TryUsername))

		kMinimumWidth = 120
		kMaximumWidth = 150
		kMinimumHeight = 30
		kMaximumHeight = 80

		StatusLabel = QLabel()
		StatusLabel.setText(statusString)
		StatusLabel.setStyleSheet(colorString)

		SummaryButton = QPushButton("&Status summary")
		SummaryButton.setDefault(True)
		SummaryButton.setMinimumWidth(kMinimumWidth)
		SummaryButton.setMaximumWidth(kMaximumWidth)
		SummaryButton.setMinimumHeight(kMinimumHeight)
		SummaryButton.setMaximumHeight(kMaximumHeight)
		SummaryLabel = QLabel("Statistics of test status")

		NewTestButton = QPushButton("&New")
		NewTestButton.setMinimumWidth(kMinimumWidth)
		NewTestButton.setMaximumWidth(kMaximumWidth)
		NewTestButton.setMinimumHeight(kMinimumHeight)
		NewTestButton.setMaximumHeight(kMaximumHeight)
		NewTestLabel = QLabel("Open new test")

		ReviewButton = QPushButton("&Review")
		ReviewButton.setMinimumWidth(kMinimumWidth)
		ReviewButton.setMaximumWidth(kMaximumWidth)
		ReviewButton.setMinimumHeight(kMinimumHeight)
		ReviewButton.setMaximumHeight(kMaximumHeight)
		ReviewLabel = QLabel("Review all results")

		ReviewModuleButton = QPushButton("&Show Module")
		ReviewModuleButton.setMinimumWidth(kMinimumWidth)
		ReviewModuleButton.setMaximumWidth(kMaximumWidth)
		ReviewModuleButton.setMinimumHeight(kMinimumHeight)
		ReviewModuleButton.setMaximumHeight(kMaximumHeight)
		ReviewModuleEdit = QLineEdit('')
		ReviewModuleEdit.setEchoMode(QLineEdit.Normal)
		ReviewModuleEdit.setPlaceholderText('Enter Module ID')

		layout = QGridLayout()
		layout.addWidget(StatusLabel,  0, 0, 1, 3)
		layout.addWidget(SummaryButton,1, 0, 1, 1)
		layout.addWidget(SummaryLabel, 1, 1, 1, 2)
		layout.addWidget(NewTestButton,2, 0, 1, 1)
		layout.addWidget(NewTestLabel, 2, 1, 1, 2)
		layout.addWidget(ReviewButton, 3, 0, 1, 1)
		layout.addWidget(ReviewLabel,  3, 1, 1, 2)
		layout.addWidget(ReviewModuleButton,4, 0, 1, 1)
		layout.addWidget(ReviewModuleEdit,  4, 1, 1, 2)

		self.MainOption.setLayout(layout)

		self.AppOption = QGroupBox()
		self.AppLayout = QHBoxLayout()

		self.RefreshButton = QPushButton("&Refresh")
		self.RefreshButton.clicked.connect(self.destroyMain)
		self.RefreshButton.clicked.connect(self.createMain)

		self.LogoutButton = QPushButton("&Logout")
		self.LogoutButton.clicked.connect(self.destroyMain)
		self.LogoutButton.clicked.connect(self.createLogin)

		self.ExitButton = QPushButton("&Exit")
		self.ExitButton.clicked.connect(QApplication.quit)
		self.AppLayout.addStretch(1)
		self.AppLayout.addWidget(self.RefreshButton)
		self.AppLayout.addWidget(self.LogoutButton)
		self.AppLayout.addWidget(self.ExitButton)
		self.AppOption.setLayout(self.AppLayout)

		self.mainLayout.addWidget(self.MainOption)
		self.mainLayout.addWidget(self.AppOption)

	def destroyMain(self):
		self.MainOption.deleteLater()
		self.AppOption.deleteLater()
		self.mainLayout.removeWidget(self.MainOption)
		self.mainLayout.removeWidget(self.AppOption)


		


		