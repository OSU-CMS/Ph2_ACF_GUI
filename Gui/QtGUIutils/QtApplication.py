from PyQt5.QtCore import *
from PyQt5.QtGui import QFont, QPixmap 
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import sys
import os

from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.settings import *
from Gui.GUIutils.FirmwareUtil import *
from Gui.QtGUIutils.QtSummaryWindow import *
from Gui.QtGUIutils.QtStartWindow import *
from Gui.QtGUIutils.QtModuleReviewWindow import *

class QtApplication(QWidget):
	def __init__(self):
		super(QtApplication,self).__init__()
		self.mainLayout = QGridLayout()
		self.setLayout(self.mainLayout)
		self.ProcessingTest = False
		self.expertMode = False
		self.FwUnderUsed = ''

		self.setLoginUI()
		self.createLogin()

	def setLoginUI(self):
		self.setGeometry(300, 300, 400, 500)  
		self.setWindowTitle('Phase2 Pixel Module Test GUI')  
		QApplication.setStyle(QStyleFactory.create('macintosh'))
		QApplication.setPalette(QApplication.style().standardPalette())
		#QApplication.setPalette(QApplication.palette())
		self.show()


	###############################################################
	##  Login page and related functions
	###############################################################
	def createLogin(self):
		self.LoginGroupBox = QGroupBox("")
		self.LoginGroupBox.setCheckable(False)

		TitleLabel = QLabel('<font size="12"> Phase2 Pixel Module Test </font>')
		TitleLabel.setFont(QFont("Courier"))
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

		if self.expertMode == False:
			self.HostName = QComboBox()
			self.HostName.addItems(DBServerIP.keys())
			self.HostName.currentIndexChanged.connect(self.changeDBList)
			HostLabel.setBuddy(self.HostName)
		else:
			self.HostEdit = QLineEdit('128.146.38.1')
			self.HostEdit.setEchoMode(QLineEdit.Normal)
			self.HostEdit.setMinimumWidth(150)
			self.HostEdit.setMaximumHeight(30)

		DatabaseLabel = QLabel("Database:")
		if self.expertMode == False:	
			self.DatabaseCombo = QComboBox()
			self.DBNames = DBNames['All']
			self.DatabaseCombo.addItems(self.DBNames)
			self.DatabaseCombo.setCurrentIndex(0)
		else:
			self.DatabaseEdit = QLineEdit('phase2pixel_test')
			self.DatabaseEdit.setEchoMode(QLineEdit.Normal)
			self.DatabaseEdit.setMinimumWidth(150)
			self.DatabaseEdit.setMaximumHeight(30)

		self.disableCheckBox = QCheckBox("&Do not connect to DB")
		self.disableCheckBox.setMaximumHeight(30)
		if self.expertMode:
			self.disableCheckBox.toggled.connect(self.HostEdit.setDisabled)
			self.disableCheckBox.toggled.connect(self.DatabaseEdit.setDisabled)
		else:
			self.disableCheckBox.toggled.connect(self.HostName.setDisabled)
			self.disableCheckBox.toggled.connect(self.DatabaseCombo.setDisabled)

		self.expertCheckBox = QCheckBox("&Expert Mode")
		self.expertCheckBox.setMaximumHeight(30)
		self.expertCheckBox.setChecked(self.expertMode)
		self.expertCheckBox.clicked.connect(self.switchMode)

		button_login = QPushButton("&Login")
		button_login.setDefault(True)
		button_login.clicked.connect(self.checkLogin)

		layout = QGridLayout()
		layout.setSpacing(20)
		layout.addWidget(TitleLabel,0,1,1,3,Qt.AlignCenter)
		layout.addWidget(UsernameLabel,1,1,1,1,Qt.AlignRight)
		layout.addWidget(self.UsernameEdit,1,2,1,2)
		layout.addWidget(PasswordLabel,2,1,1,1,Qt.AlignRight)
		layout.addWidget(self.PasswordEdit,2,2,1,2)
		layout.addWidget(HostLabel,3,1,1,1,Qt.AlignRight)
		if self.expertMode:
			layout.addWidget(self.HostEdit,3,2,1,2)
		else:
			layout.addWidget(self.HostName,3,2,1,2)
		layout.addWidget(DatabaseLabel,4,1,1,1,Qt.AlignRight)
		if self.expertMode:
			layout.addWidget(self.DatabaseEdit,4,2,1,2)
		else:
			layout.addWidget(self.DatabaseCombo,4,2,1,2)
		layout.addWidget(self.disableCheckBox,5,2,1,1,Qt.AlignLeft)
		layout.addWidget(self.expertCheckBox,5,3,1,1,Qt.AlignRight)
		layout.addWidget(button_login,6,1,1,3)
		layout.setRowMinimumHeight(6, 50)

		layout.setColumnStretch(0, 1)
		layout.setColumnStretch(1, 1)
		layout.setColumnStretch(2, 2)
		layout.setColumnStretch(3, 1)
		self.LoginGroupBox.setLayout(layout)


		self.LogoGroupBox = QGroupBox("")
		self.LogoGroupBox.setCheckable(False)
		self.LogoGroupBox.setMaximumHeight(100)

		self.LogoLayout = QHBoxLayout()
		OSULogoLabel = QLabel()
		OSUpixmap = QPixmap("icons/osuicon.png").scaled(QSize(200,60), Qt.KeepAspectRatio, Qt.SmoothTransformation)
		OSULogoLabel.setPixmap(OSUpixmap)
		CMSLogoLabel = QLabel()
		CMSpixmap = QPixmap("icons/cmsicon.png").scaled(QSize(200,60), Qt.KeepAspectRatio, Qt.SmoothTransformation)
		CMSLogoLabel.setPixmap(CMSpixmap)
		self.LogoLayout.addWidget(OSULogoLabel)
		self.LogoLayout.addStretch(1)
		self.LogoLayout.addWidget(CMSLogoLabel)

		self.LogoGroupBox.setLayout(self.LogoLayout)

		self.mainLayout.addWidget(self.LoginGroupBox, 0, 0)
		self.mainLayout.addWidget(self.LogoGroupBox, 1, 0)

	def changeDBList(self):
		self.DBNames = DBNames[str(self.HostName.currentText())]
		self.DatabaseCombo.clear()
		self.DatabaseCombo.addItems(self.DBNames)
		self.DatabaseCombo.setCurrentIndex(0)

	def switchMode(self):
		if self.expertMode:
			self.expertMode =  False
		else:
			self.expertMode = True
		self.destroyLogin()
		self.createLogin()

	def destroyLogin(self):
		self.LoginGroupBox.deleteLater()
		self.LogoGroupBox.deleteLater()
		self.mainLayout.removeWidget(self.LoginGroupBox)
	
	def checkLogin(self):
		msg = QMessageBox()

		if self.expertMode == True:
			self.TryUsername = self.UsernameEdit.text()
			self.TryPassword = self.PasswordEdit.text()
			self.TryHostAddress = self.HostEdit.text()
			self.TryDatabase = self.DatabaseEdit.text()
		else:
			self.TryUsername = self.UsernameEdit.text()
			self.TryPassword = self.PasswordEdit.text()
			self.TryHostAddress = DBServerIP[str(self.HostName.currentText())]
			self.TryDatabase = str(self.DatabaseCombo.currentText())


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

	###############################################################
	##  Login page and related functions  (END)
	###############################################################

	###############################################################
	##  Main page and related functions
	###############################################################
	
	def createMain(self):
		statusString, colorString = checkDBConnection(self.connection)
		self.FirmwareStatus = QGroupBox("Hello,{}!".format(self.TryUsername))
		self.FirmwareStatus.setDisabled(True)
		
		DBStatusLabel = QLabel()
		DBStatusLabel.setText("Database connection:")
		DBStatusValue = QLabel()
		DBStatusValue.setText(statusString)
		DBStatusValue.setStyleSheet(colorString)

		self.StatusList = []
		self.StatusList.append([DBStatusLabel, DBStatusValue])

		for index, (firmwareName, fwAddress) in enumerate(FirmwareList.items()):
			FwNameLabel = QLabel()
			FwNameLabel.setText(firmwareName)
			FwStatusValue = QLabel()
			FwStatusComment, FwStatusColor = self.getFwComment(firmwareName,fwAddress)
			FwStatusValue.setText(FwStatusComment)
			FwStatusValue.setStyleSheet(FwStatusColor)
			self.StatusList.append([FwNameLabel,FwStatusValue])

		self.UseButton = []

		StatusLayout = QGridLayout()

		for index, items in enumerate(self.StatusList):
			if index == 0:
				StatusLayout.addWidget(items[0], index, 1,  1, 1)
				StatusLayout.addWidget(items[1], index, 2,  1, 2)
			else:
				UseButton = QPushButton("&Use")
				UseButton.toggle()
				UseButton.clicked.connect(lambda state, x="{0}".format(index-1) : self.switchFw(x))
				UseButton.setCheckable(True)
				self.UseButton.append(UseButton)
				StatusLayout.addWidget(UseButton, index, 0, 1, 1)
				StatusLayout.addWidget(items[0], index, 1,  1, 1)
				StatusLayout.addWidget(items[1], index, 2,  1, 2)
				DetailButton = QPushButton("&Details")
				DetailButton.clicked.connect(lambda state, x="{0}".format(index-1) : self.showCommentFw(x))
				StatusLayout.addWidget(DetailButton, index, 4, 1, 1)

		if self.FwUnderUsed != '':
			index = self.getIndex(self.FwUnderUsed,self.StatusList)
			print(index)
			self.occupyFw("{0}".format(index))


		self.FirmwareStatus.setLayout(StatusLayout)
		self.FirmwareStatus.setDisabled(False)
				
		self.MainOption = QGroupBox("Main")

		kMinimumWidth = 120
		kMaximumWidth = 150
		kMinimumHeight = 30
		kMaximumHeight = 100

		self.SummaryButton = QPushButton("&Status summary")
		self.SummaryButton.setDefault(True)
		self.SummaryButton.setMinimumWidth(kMinimumWidth)
		self.SummaryButton.setMaximumWidth(kMaximumWidth)
		self.SummaryButton.setMinimumHeight(kMinimumHeight)
		self.SummaryButton.setMaximumHeight(kMaximumHeight)
		self.SummaryButton.clicked.connect(self.openSummaryWindow)
		SummaryLabel = QLabel("Statistics of test status")

		self.NewTestButton = QPushButton("&New")
		self.NewTestButton.setMinimumWidth(kMinimumWidth)
		self.NewTestButton.setMaximumWidth(kMaximumWidth)
		self.NewTestButton.setMinimumHeight(kMinimumHeight)
		self.NewTestButton.setMaximumHeight(kMaximumHeight)
		self.NewTestButton.clicked.connect(self.openNewTest)
		self.NewTestButton.setDisabled(True)
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
		self.ReviewModuleButton.clicked.connect(self.openModuleReviewWindow)
		self.ReviewModuleEdit = QLineEdit('')
		self.ReviewModuleEdit.setEchoMode(QLineEdit.Normal)
		self.ReviewModuleEdit.setPlaceholderText('Enter Module ID')

		layout = QGridLayout()
		layout.addWidget(self.SummaryButton,0, 0, 1, 1)
		layout.addWidget(SummaryLabel, 0, 1, 1, 2)
		layout.addWidget(self.NewTestButton,1, 0, 1, 1)
		layout.addWidget(NewTestLabel, 1, 1, 1, 2)
		layout.addWidget(self.ReviewButton, 2, 0, 1, 1)
		layout.addWidget(ReviewLabel,  2, 1, 1, 2)
		layout.addWidget(self.ReviewModuleButton,3, 0, 1, 1)
		layout.addWidget(self.ReviewModuleEdit,  3, 1, 1, 2)

		self.MainOption.setLayout(layout)

		self.AppOption = QGroupBox()
		self.AppLayout = QHBoxLayout()

		self.RefreshButton = QPushButton("&Refresh")
		self.RefreshButton.clicked.connect(self.disableBoxs)
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
		#self.ExitButton.clicked.connect(QApplication.quit)
		self.ExitButton.clicked.connect(self.close)
		self.AppLayout.addStretch(1)
		self.AppLayout.addWidget(self.RefreshButton)
		self.AppLayout.addWidget(self.LogoutButton)
		self.AppLayout.addWidget(self.ExitButton)
		self.AppOption.setLayout(self.AppLayout)

		self.mainLayout.addWidget(self.FirmwareStatus)
		self.mainLayout.addWidget(self.MainOption)
		self.mainLayout.addWidget(self.AppOption)

	def disableBoxs(self):
		self.FirmwareStatus.setDisabled(True)
		self.MainOption.setDisabled(True)

	def destroyMain(self):
		self.FirmwareStatus.deleteLater()
		self.MainOption.deleteLater()
		self.AppOption.deleteLater()
		self.mainLayout.removeWidget(self.FirmwareStatus)
		self.mainLayout.removeWidget(self.MainOption)
		self.mainLayout.removeWidget(self.AppOption)

	def openNewTest(self):
		self.StartNewTest = QtStartWindow(self)
		self.NewTestButton.setDisabled(True)
		self.LogoutButton.setDisabled(True)
		self.ExitButton.setDisabled(True)
		pass

	def openSummaryWindow(self):
		self.SummaryViewer = QtSummaryWindow(self)
		self.SummaryButton.setDisabled(True)

	def openReviewWindow(self):
		pass

	def openModuleReviewWindow(self):
		Module_ID = self.ReviewModuleEdit.text()
		if Module_ID != "":
			self.ModuleReviewWindow =  QtModuleReviewWindow(self, Module_ID)
		else:
			QMessageBox.information(None, "Error", "Please enter a valid module ID", QMessageBox.Ok)

	def getFwComment(self,firmwareName,fwAddress):
		comment,color = fwStatusParser(firmwareName,fwAddress)
		return comment,color

	def getIndex(self, element, List2D):
		print(element)
		for index, item in enumerate(List2D):
			if item[0].text() == element:
				return index - 1
		return -1

	def switchFw(self, index):
		if self.UseButton[int(index)].isChecked():
			self.occupyFw(index)
		else:
			self.releaseFw(index)

	def occupyFw(self, index):
		if self.StatusList[int(index)+1][1].text() == "Connected":
			self.NewTestButton.setDisabled(False)
		for i ,button in enumerate(self.UseButton):
			if i == int(index):
				button.setChecked(True)
				button.setText("&In use")
				self.FwUnderUsed = self.StatusList[i+1][0].text()
			else:
				button.setDisabled(True)

	def releaseFw(self, index):
		for i ,button in enumerate(self.UseButton):
			if i == int(index):
				self.FwUnderUsed = ''
				button.setText("&Use")
				button.setDown(False)
				button.setDisabled(False)
			else:
				button.setDisabled(False)

	def showCommentFw(self, index):
		comment = self.StatusList[int(index)+1][1].text()
		solution = FwStatusCheck[comment]
		QMessageBox.information(None, "Info", "{}".format(solution), QMessageBox.Ok)
			
	###############################################################
	##  Main page and related functions  (END)
	###############################################################

	def closeEvent(self, event):
		#Fixme: strict criterias for process checking  should be added here:
		#if self.ProcessingTest == True:
		#	QMessageBox.critical(self, 'Critical Message', 'There is running process, can not close!')
		#	event.ignore()

		reply = QMessageBox.question(self, 'Window Close', 'Are you sure you want to exit?',
			QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

		if reply == QMessageBox.Yes:
			print('Application terminated')
			event.accept()
		else:
			event.ignore()


		