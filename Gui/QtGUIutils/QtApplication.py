from PyQt5.QtCore import *
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import sys
import os

from Gui.GUIutils.DBConnection import *
from Gui.QtGUIutils.QtStartWindow import *

class QtApplication(QWidget):
	def __init__(self):
		super(QtApplication,self).__init__()
		self.mainLayout = QGridLayout()
		self.setLayout(self.mainLayout)
		self.ProcessingTest = False

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

		self.SummaryButton = QPushButton("&Status summary")
		self.SummaryButton.setDefault(True)
		self.SummaryButton.setMinimumWidth(kMinimumWidth)
		self.SummaryButton.setMaximumWidth(kMaximumWidth)
		self.SummaryButton.setMinimumHeight(kMinimumHeight)
		self.SummaryButton.setMaximumHeight(kMaximumHeight)
		SummaryLabel = QLabel("Statistics of test status")

		self.NewTestButton = QPushButton("&New")
		self.NewTestButton.setMinimumWidth(kMinimumWidth)
		self.NewTestButton.setMaximumWidth(kMaximumWidth)
		self.NewTestButton.setMinimumHeight(kMinimumHeight)
		self.NewTestButton.setMaximumHeight(kMaximumHeight)
		self.NewTestButton.clicked.connect(self.openNewTest)
		if self.ProcessingTest == True:
			self.NewTestButton.setDisabled(True)
		NewTestLabel = QLabel("Open new test")

		self.ReviewButton = QPushButton("&Review")
		self.ReviewButton.setMinimumWidth(kMinimumWidth)
		self.ReviewButton.setMaximumWidth(kMaximumWidth)
		self.ReviewButton.setMinimumHeight(kMinimumHeight)
		self.ReviewButton.setMaximumHeight(kMaximumHeight)
		ReviewLabel = QLabel("Review all results")

		self.ReviewModuleButton = QPushButton("&Show Module")
		self.ReviewModuleButton.setMinimumWidth(kMinimumWidth)
		self.ReviewModuleButton.setMaximumWidth(kMaximumWidth)
		self.ReviewModuleButton.setMinimumHeight(kMinimumHeight)
		self.ReviewModuleButton.setMaximumHeight(kMaximumHeight)
		ReviewModuleEdit = QLineEdit('')
		ReviewModuleEdit.setEchoMode(QLineEdit.Normal)
		ReviewModuleEdit.setPlaceholderText('Enter Module ID')

		layout = QGridLayout()
		layout.addWidget(StatusLabel,  0, 0, 1, 3)
		layout.addWidget(self.SummaryButton,1, 0, 1, 1)
		layout.addWidget(SummaryLabel, 1, 1, 1, 2)
		layout.addWidget(self.NewTestButton,2, 0, 1, 1)
		layout.addWidget(NewTestLabel, 2, 1, 1, 2)
		layout.addWidget(self.ReviewButton, 3, 0, 1, 1)
		layout.addWidget(ReviewLabel,  3, 1, 1, 2)
		layout.addWidget(self.ReviewModuleButton,4, 0, 1, 1)
		layout.addWidget(ReviewModuleEdit,  4, 1, 1, 2)

		self.MainOption.setLayout(layout)

		self.AppOption = QGroupBox()
		self.AppLayout = QHBoxLayout()

		self.RefreshButton = QPushButton("&Refresh")
		self.RefreshButton.clicked.connect(self.destroyMain)
		self.RefreshButton.clicked.connect(self.createMain)

		self.LogoutButton = QPushButton("&Logout")
		# Fixme: more conditions to be added
		if self.ProcessingTest:
			self.LogoutButton.setDisabled(True)
		self.LogoutButton.clicked.connect(self.destroyMain)
		self.LogoutButton.clicked.connect(self.createLogin)

		self.ExitButton = QPushButton("&Exit")
		# Fixme: more conditions to be added
		if self.ProcessingTest:
			self.ExitButton.setDisabled(True)
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

	def openNewTest(self):
		self.StartNewTest = QtStartWindow(self)
		self.NewTestButton.setDisabled(True)
		self.LogoutButton.setDisabled(True)
		self.ExitButton.setDisabled(True)
		pass

	def openSummaryWindow(self):
		pass

	def openReviewWindow(self):
		pass

	def openModuleReviewWindow(self):
		pass

	def closeEvent(self, event):
		#Fixme: strict criterias for process checking  should be added here:
		#if self.ProcessingTest == True:
		#	QMessageBox.critical(self, 'Critical Message', 'There is running process, can not close!')
		#	event.ignore()

		reply = QMessageBox.question(self, 'Window Close', 'Are you sure you want to exit?',
			QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

		if reply == QMessageBox.Yes:
			event.accept()
			self.release()
			print('Window closed')
		else:
			event.ignore()
		


		