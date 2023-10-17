from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtGui import QPixmap
from PyQt5.QtWidgets import (QAbstractItemView, QAction, QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit, QMenuBar,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableView, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox, QSplitter)

import sys
import os
import subprocess
from subprocess import Popen, PIPE

from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.guiUtils import *
from Gui.QtGUIutils.QtDBTableWidget import *
from Gui.QtGUIutils.QtViewTableTab  import *
from Gui.QtGUIutils.QtImageInsertionTab import *
from Gui.QtGUIutils.QtImageViewerTab import *

class QtDBConsoleWindow(QMainWindow):
	def __init__(self,master):
		super(QtDBConsoleWindow,self).__init__()
		self.master = master
		self.connection = self.master.connection
		self.GroupBoxSeg = [1,10,1]
		self.processing = []
		#Fixme: QTimer to be added to update the page automatically

		self.mainWidget = QWidget()
		self.mainLayout = QGridLayout()
		self.mainWidget.setLayout(self.mainLayout)
		self.setCentralWidget(self.mainWidget)

		self.occupied()
		self.setLoginUI()
		self.createHeadLine()
		self.createMain()
		self.createApp()
		if isActive(self.connection):
			self.connectDB()

	def setLoginUI(self):
		self.setGeometry(200, 200, 600, 1200)  
		self.setWindowTitle('Database Console') 

		self.menubar =self.menuBar()
		self.menubar.setNativeMenuBar(False)

		###################################################
		##  Database Menu
		###################################################
		self.actionDatabase = self.menubar.addMenu("&Database")

		self.ViewerAction = QAction("&View Table")
		self.ViewerAction.triggered.connect(self.viewTable)
		self.actionDatabase.addAction(self.ViewerAction)

		###################################################
		##  Image Menu
		###################################################	
		self.actionImage = self.menubar.addMenu("&Image")

		self.InsertImgAction = QAction("&Insert Image")
		self.InsertImgAction.triggered.connect(self.insertImage)
		self.actionImage.addAction(self.InsertImgAction)

		self.ViewImgAction = QAction("&View Image")
		self.ViewImgAction.triggered.connect(self.viewImage)
		self.actionImage.addAction(self.ViewImgAction)

		###################################################
		##  Shipment Menu
		###################################################
		self.actionShipment = self.menubar.addMenu("&Shipment")

		self.sendPackAction = QAction("&Send Package")
		self.sendPackAction.triggered.connect(self.sendPackage)
		self.actionShipment.addAction(self.sendPackAction)

		self.receivePackAction = QAction("&Receive Package")
		self.receivePackAction.triggered.connect(self.receivePackage)
		self.actionShipment.addAction(self.receivePackAction)

		###################################################
		##  User Menu
		###################################################
		self.actionUser = self.menubar.addMenu("&User")

		self.ShowUserAction = QAction("&Show Users",self)
		self.ShowUserAction.triggered.connect(self.displayUsers)
		self.actionUser.addAction(self.ShowUserAction)

		self.AddUserAction = QAction("&Add Users",self)
		self.AddUserAction.triggered.connect(self.addUsers)
		self.actionUser.addAction(self.AddUserAction)

		self.UpdateUserAction = QAction("&Update Profile",self)
		self.UpdateUserAction.triggered.connect(self.updateProfile)
		self.actionUser.addAction(self.UpdateUserAction)

		self.ShowInstAction = QAction("&Show institute",self)
		self.ShowInstAction.triggered.connect(self.displayInstitue)
		self.actionUser.addAction(self.ShowInstAction)

		###################################################
		##  Miscellanea Menu
		###################################################
		self.actionMiscel = self.menubar.addMenu("&Miscellanea")

		self.ComplaintAction =  QAction("&Submit Complaint",self)
		self.ComplaintAction.triggered.connect(self.fileComplaint)
		self.actionMiscel.addAction(self.ComplaintAction)

		self.show()

	def activateMenuBar(self):
		self.menubar.setDisabled(False)

	def deactivateMenuBar(self):
		self.menubar.setDisabled(True)

	def enableMenuItem(self,item, enabled):
		item.setEnabled(enabled)

	##########################################################################
	##  Functions for Table Viewer
	##########################################################################

	def viewTable(self):
		viewTableTab = QtViewTableTab(self)
		self.MainTabs.addTab(viewTableTab, "Database Viewer")
		self.MainTabs.setCurrentWidget(viewTableTab)

	##########################################################################
	##  Functions for Table Viewer (END)
	##########################################################################

	##########################################################################
	##  Functions for Image
	##########################################################################

	def insertImage(self):
		insertImageTab = QtImageInsertionTab(self)
		self.InsertImgAction.setDisabled(True)
		self.MainTabs.addTab(insertImageTab, "Insert Image")
		self.MainTabs.setCurrentWidget(insertImageTab)


	def viewImage(self):
		viewImageTab = QtImageViewerTab(self)
		self.MainTabs.addTab(viewImageTab, "Image  Viewer")
		self.MainTabs.setCurrentWidget(viewImageTab)

	##########################################################################
	##  Functions for Image (END)
	##########################################################################

	##########################################################################
	##  Functions for Shipment
	##########################################################################

	def sendPackage(self):
		self.sendPackageTab = QWidget()
		
		# Add tabs
		self.MainTabs.addTab(self.sendPackageTab,"Send Package")
		self.sendPackAction.setDisabled(True)
		self.sendPackageTab.layout = QGridLayout(self)
		sendPackageLabel = QLabel('<font size="12"> Package Delivery </font>')
		sendPackageLabel.setMaximumHeight(60)

		self.SPSenderLabel = QLabel('Sender:')
		self.SPSenderEdit = QLineEdit('')
		self.SPSenderEdit.setEchoMode(QLineEdit.Normal)
		self.SPSenderEdit.setPlaceholderText('Sender Name')

		self.SPReceiverLabel = QLabel('Receiver:')
		self.SPReceiverEdit = QLineEdit('')
		self.SPReceiverEdit.setEchoMode(QLineEdit.Normal)
		self.SPReceiverEdit.setPlaceholderText('Receiver Name')
		self.SPReceiverEdit.setDisabled(True)

		siteList = getByColumnName("institute",describeInstitute(self.connection),retrieveAllInstitute(self.connection))

		self.SPOriginLabel = QLabel('Origin:')
		self.SPOriginCombo = QComboBox()
		self.SPOriginCombo.addItems(["{0}".format(x) for x in siteList])

		self.SPDestinationLabel = QLabel('Destination:')
		self.SPDestinationCombo = QComboBox()
		self.SPDestinationCombo.addItems(["{0}".format(x) for x in siteList])

		self.SPDateSentLabel = QLabel('date sent:')
		self.SPDateSentEdit = QDateTimeEdit()
		self.SPDateSentEdit.setDateTime(QDateTime.currentDateTime())

		self.SPDateReceivedLabel = QLabel('date received:')
		self.SPDateReceivedEdit = QDateTimeEdit()
		self.SPDateReceivedEdit.setDisabled(True)
		#self.SPDateReceivedEdit.setDateTime(QDateTime.currentDateTime())

		self.SPCarrierLabel = QLabel('Carrier:')
		self.SPCarrierEdit = QLineEdit('')
		self.SPCarrierEdit.setEchoMode(QLineEdit.Normal)
		self.SPCarrierEdit.setPlaceholderText('carrier name')

		self.SPTrackingLabel = QLabel('Tracking Number:')
		self.SPTrackingEdit = QLineEdit('')
		self.SPTrackingEdit.setEchoMode(QLineEdit.Normal)
		self.SPTrackingEdit.setPlaceholderText('Tracking Code')

		self.SPCommentLabel = QLabel('Comment:')
		self.SPCommentEdit = QTextEdit('')
		self.SPCommentEdit.setPlaceholderText("Your comment")

		self.SPFeedBackLabel = QLabel()

		self.SPSubmitButton = QPushButton('Submit')
		self.SPSubmitButton.clicked.connect(self.submitSPRequest)

		self.sendPackageTab.layout.addWidget(sendPackageLabel,0,0,1,4,Qt.AlignTop)
		self.sendPackageTab.layout.addWidget(self.SPSenderLabel,1,0,1,1)
		self.sendPackageTab.layout.addWidget(self.SPSenderEdit,1,1,1,1)
		self.sendPackageTab.layout.addWidget(self.SPDateSentLabel,1,2,1,1)
		self.sendPackageTab.layout.addWidget(self.SPDateSentEdit,1,3,1,1)

		self.sendPackageTab.layout.addWidget(self.SPOriginLabel,2,0,1,1)
		self.sendPackageTab.layout.addWidget(self.SPOriginCombo,2,1,1,1)
		self.sendPackageTab.layout.addWidget(self.SPDestinationLabel,2,2,1,1)
		self.sendPackageTab.layout.addWidget(self.SPDestinationCombo,2,3,1,1)

		self.sendPackageTab.layout.addWidget(self.SPCarrierLabel,3,0,1,1)
		self.sendPackageTab.layout.addWidget(self.SPCarrierEdit,3,1,1,1)
		self.sendPackageTab.layout.addWidget(self.SPTrackingLabel,3,2,1,1)
		self.sendPackageTab.layout.addWidget(self.SPTrackingEdit,3,3,1,1)

		self.sendPackageTab.layout.addWidget(self.SPCommentLabel,4,0,1,4,Qt.AlignLeft)
		self.sendPackageTab.layout.addWidget(self.SPCommentEdit,5,0,3,4)
		self.sendPackageTab.layout.addWidget(self.SPFeedBackLabel,8,0,1,2)
		self.sendPackageTab.layout.addWidget(self.SPSubmitButton,8,3,1,1)

		self.sendPackageTab.setLayout(self.sendPackageTab.layout)
		self.MainTabs.setCurrentWidget(self.sendPackageTab)

	def submitSPRequest(self):
		if self.SPTrackingEdit.text()=="":
			self.SPFeedBackLabel.setText("Please make sure Tracking Number are filled")
			return
		if self.SPSenderEdit.text() == "":
			self.SPFeedBackLabel.setText("Please make sure sender are filled")
			return
		Args = describeTable(self.connection, "shipment")
		Data = []
		SubmitArgs = []
		for arg in Args:
			if arg == "origin":
				Data.append(self.SPOriginCombo.currentText())
				SubmitArgs.append(arg)
			if arg == "destination":
				Data.append(self.SPDestinationCombo.currentText())
				SubmitArgs.append(arg)
			if arg == "sender":
				Data.append(self.SPSenderEdit.text())
				SubmitArgs.append(arg)
			if arg == "date_sent":
				Data.append(self.SPDateSentEdit.dateTime().toUTC().toString("yyyy-dd-MM hh:mm:ss.z"))
				SubmitArgs.append(arg)
			if arg == "carrier":
				Data.append(self.SPCarrierEdit.text())
				SubmitArgs.append(arg)
			if arg == "tracking_number":
				Data.append(self.SPTrackingEdit.text())
				SubmitArgs.append(arg)
			if arg == "comment":
				Data.append(self.SPCommentEdit.toPlainText())
				SubmitArgs.append(arg)
		try:
			insertGenericTable(self.connection,"shipment",SubmitArgs,Data)
		except:
			print("Failed to submit the shipment record")
			return

		self.SPSenderLabel.deleteLater()
		self.SPSenderEdit.deleteLater()
		self.SPDateSentLabel.deleteLater()
		self.SPDateSentEdit.deleteLater()

		self.SPOriginLabel.deleteLater()
		self.SPOriginCombo.deleteLater()
		self.SPDestinationLabel.deleteLater()
		self.SPDestinationCombo.deleteLater()

		self.SPCarrierLabel.deleteLater()
		self.SPCarrierEdit.deleteLater()
		self.SPTrackingLabel.deleteLater()
		self.SPTrackingEdit.deleteLater()

		self.SPCommentLabel.deleteLater()
		self.SPCommentEdit.deleteLater()
		self.SPFeedBackLabel.deleteLater()
		self.SPSubmitButton.deleteLater()

		self.sendPackageTab.layout.removeWidget(self.SPSenderLabel)
		self.sendPackageTab.layout.removeWidget(self.SPSenderEdit)
		self.sendPackageTab.layout.removeWidget(self.SPDateSentLabel)
		self.sendPackageTab.layout.removeWidget(self.SPDateSentEdit)

		self.sendPackageTab.layout.removeWidget(self.SPOriginLabel)
		self.sendPackageTab.layout.removeWidget(self.SPOriginCombo)
		#self.sendPackageTab.layout.removeWidget(self.SPDestinationLabel)
		#self.sendPackageTab.layout.removeWidget(self.SPDestinationCombo)

		self.sendPackageTab.layout.removeWidget(self.SPCarrierLabel)
		self.sendPackageTab.layout.removeWidget(self.SPCarrierEdit)
		self.sendPackageTab.layout.removeWidget(self.SPTrackingLabel)
		self.sendPackageTab.layout.removeWidget(self.SPTrackingEdit)

		self.sendPackageTab.layout.removeWidget(self.SPCommentLabel)
		self.sendPackageTab.layout.removeWidget(self.SPCommentEdit)
		self.sendPackageTab.layout.removeWidget(self.SPFeedBackLabel)
		self.sendPackageTab.layout.removeWidget(self.SPSubmitButton)

		self.SPFeedBackLabel = QLabel("Submitted")
		self.SPFeedBackLabel.setStyleSheet("color:green")
		self.SPContinueButton =  QPushButton('Continue')
		self.SPContinueButton.clicked.connect(self.recreateSP)
		self.SPCloseButton =  QPushButton('Close')
		self.SPCloseButton.clicked.connect(self.closeSP)
		self.sendPackageTab.layout.addWidget(self.SPFeedBackLabel,1,0,1,2)
		self.sendPackageTab.layout.addWidget(self.SPContinueButton,1,3,1,1)
		self.sendPackageTab.layout.addWidget(self.SPCloseButton,1,4,1,1)
		return

	def recreateSP(self):
		self.closeTab(self.MainTabs.currentIndex())
		self.sendPackage()
	
	def closeSP(self):
		self.closeTab(self.MainTabs.currentIndex())

	def receivePackage(self):
		self.receivePackageTab = QWidget()
		
		# Add tabs
		self.MainTabs.addTab(self.receivePackageTab,"Receive Package")
		self.receivePackAction.setDisabled(True)
		self.receivePackageTab.layout = QGridLayout(self)
		receivePackageLabel = QLabel('<font size="12"> Package Delivery </font>')
		receivePackageLabel.setMaximumHeight(60)

		self.RPIDLabel = QLabel('ID:')
		self.RPIDEdit = QLineEdit('')
		self.RPIDEdit.setEchoMode(QLineEdit.Normal)
		self.RPIDEdit.setPlaceholderText('Shipment ID')
		self.RPFetchButton = QPushButton("Fetch")
		self.RPFetchButton.clicked.connect(self.fillRPRequest)

		self.RPSenderLabel = QLabel('Sender:')
		self.RPSenderEdit = QLineEdit('')
		self.RPSenderEdit.setEchoMode(QLineEdit.Normal)
		self.RPSenderEdit.setPlaceholderText('Sender Name')
		self.RPSenderEdit.setDisabled(True)

		self.RPReceiverLabel = QLabel('Receiver:')
		self.RPReceiverEdit = QLineEdit('')
		self.RPReceiverEdit.setEchoMode(QLineEdit.Normal)
		self.RPReceiverEdit.setPlaceholderText('Receiver Name')

		siteList = getByColumnName("institute",describeInstitute(self.connection),retrieveAllInstitute(self.connection))

		self.RPOriginLabel = QLabel('Origin:')
		self.RPOriginCombo = QComboBox()
		self.RPOriginCombo.addItems(["{0}".format(x) for x in siteList])
		self.RPOriginCombo.setDisabled(True)

		self.RPDestinationLabel = QLabel('Destination:')
		self.RPDestinationCombo = QComboBox()
		self.RPDestinationCombo.addItems(["{0}".format(x) for x in siteList])
		self.RPDestinationCombo.setDisabled(True)

		self.RPDateSentLabel = QLabel('date sent:')
		self.RPDateSentEdit = QDateTimeEdit()
		#self.RPDateSentEdit.setDateTime(QDateTime.currentDateTime())
		self.RPDateSentEdit.setDisabled(True)

		self.RPDateReceivedLabel = QLabel('date received:')
		self.RPDateReceivedEdit = QDateTimeEdit()
		self.RPDateReceivedEdit.setDateTime(QDateTime.currentDateTime())

		self.RPCarrierLabel = QLabel('Carrier:')
		self.RPCarrierEdit = QLineEdit('')
		self.RPCarrierEdit.setEchoMode(QLineEdit.Normal)
		self.RPCarrierEdit.setPlaceholderText('carrier name')
		self.RPCarrierEdit.setDisabled(True)

		self.RPTrackingLabel = QLabel('Tracking Number:')
		self.RPTrackingEdit = QLineEdit('')
		self.RPTrackingEdit.setEchoMode(QLineEdit.Normal)
		self.RPTrackingEdit.setPlaceholderText('Tracking Code')
		self.RPTrackingEdit.setDisabled(True)

		self.RPCommentLabel = QLabel('Comment:')
		self.RPCommentEdit = QTextEdit('')
		self.RPCommentEdit.setPlaceholderText("Your comment")

		self.RPFeedBackLabel = QLabel()

		self.RPSubmitButton = QPushButton('Submit')
		self.RPSubmitButton.clicked.connect(self.submitRPRequest)

		self.receivePackageTab.layout.addWidget(receivePackageLabel,0,0,1,4,Qt.AlignTop)

		self.receivePackageTab.layout.addWidget(self.RPIDLabel,1,0,1,1)
		self.receivePackageTab.layout.addWidget(self.RPIDEdit,1,1,1,1)
		self.receivePackageTab.layout.addWidget(self.RPFetchButton,1,2,1,1)

		self.receivePackageTab.layout.addWidget(self.RPSenderLabel,2,0,1,1)
		self.receivePackageTab.layout.addWidget(self.RPSenderEdit,2,1,1,1)
		self.receivePackageTab.layout.addWidget(self.RPDateSentLabel,2,2,1,1)
		self.receivePackageTab.layout.addWidget(self.RPDateSentEdit,2,3,1,1)

		self.receivePackageTab.layout.addWidget(self.RPReceiverLabel,3,0,1,1)
		self.receivePackageTab.layout.addWidget(self.RPReceiverEdit,3,1,1,1)
		self.receivePackageTab.layout.addWidget(self.RPDateReceivedLabel,3,2,1,1)
		self.receivePackageTab.layout.addWidget(self.RPDateReceivedEdit,3,3,1,1)

		self.receivePackageTab.layout.addWidget(self.RPOriginLabel,4,0,1,1)
		self.receivePackageTab.layout.addWidget(self.RPOriginCombo,4,1,1,1)
		self.receivePackageTab.layout.addWidget(self.RPDestinationLabel,4,2,1,1)
		self.receivePackageTab.layout.addWidget(self.RPDestinationCombo,4,3,1,1)

		self.receivePackageTab.layout.addWidget(self.RPCarrierLabel,5,0,1,1)
		self.receivePackageTab.layout.addWidget(self.RPCarrierEdit,5,1,1,1)
		self.receivePackageTab.layout.addWidget(self.RPTrackingLabel,5,2,1,1)
		self.receivePackageTab.layout.addWidget(self.RPTrackingEdit,5,3,1,1)

		self.receivePackageTab.layout.addWidget(self.RPCommentLabel,6,0,1,4,Qt.AlignLeft)
		self.receivePackageTab.layout.addWidget(self.RPCommentEdit,7,0,3,4)
		self.receivePackageTab.layout.addWidget(self.RPFeedBackLabel,10,0,1,3)
		self.receivePackageTab.layout.addWidget(self.RPSubmitButton,10,3,1,1)

		self.receivePackageTab.setLayout(self.receivePackageTab.layout)
		self.MainTabs.setCurrentWidget(self.receivePackageTab)

	def fillRPRequest(self):
		try:
			header = describeTable(self.connection,"shipment",True)
			shipmentInfo = retrieveWithConstraint(self.connection,"shipment",id=self.RPIDEdit.text())
		except:
			return
		if len(shipmentInfo) > 0 and len(header) == len(shipmentInfo[0]):
			self.RPFeedBackLabel.setText("Delivery Record found")
			self.RPSenderEdit.setText(getByColumnName("sender",header,shipmentInfo)[0])
			origin_index = self.RPOriginCombo.findText(getByColumnName("origin",header,shipmentInfo)[0])
			self.RPOriginCombo.setCurrentIndex(origin_index)
			destination_index = self.RPDestinationCombo.findText(getByColumnName("destination",header,shipmentInfo)[0])
			self.RPDestinationCombo.setCurrentIndex(destination_index)
			time_string = getByColumnName("date_sent",header,shipmentInfo)[0].strftime("%m/%d/%Y, %H:%M:%S")
			print(time_string)
			self.RPDateSentEdit.setDateTime(QDateTime.fromString(time_string))
			self.RPCarrierEdit.setText(getByColumnName("carrier",header,shipmentInfo)[0])
			self.RPTrackingEdit.setText(getByColumnName("tracking_number",header,shipmentInfo)[0])
			self.RPCommentEdit.setText(getByColumnName("comment",header,shipmentInfo)[0])
		else:
			self.RPFeedBackLabel.setText("Shipment ID not found")
			self.RPSenderEdit.setText("")
			origin_index = self.RPOriginCombo.findText("")
			self.RPOriginCombo.setCurrentIndex(origin_index)
			destination_index = self.RPDestinationCombo.findText("")
			self.RPDestinationCombo.setCurrentIndex(destination_index)
			#time_string = datetime.strftime("%m/%d/%Y, %H:%M:%S")
			#self.RPDateSentEdit.setDateTime(QDateTime.fromString(time_string))
			self.RPCarrierEdit.setText("")
			self.RPTrackingEdit.setText("")
			self.RPCommentEdit.setText("")

	def submitRPRequest(self):
		if self.RPTrackingEdit.text()=="":
			self.RPFeedBackLabel.setText("Please make sure Tracking Number are filled")
			return
		if self.RPReceiverEdit.text() == "":
			self.RPFeedBackLabel.setText("Please make sure receiver are filled")
			return
		if self.RPDateReceivedEdit.text() == "":
			self.RPFeedBackLabel.setText("Please make sure received time are filled")
			return
		Args = describeTable(self.connection, "shipment")
		Data = []
		SubmitArgs = []
		for arg in Args:
			if arg == "receiver":
				Data.append(self.RPReceiverEdit.text())
				SubmitArgs.append(arg)
			if arg == "date_received":
				Data.append(self.RPDateReceivedEdit.dateTime().toUTC().toString("yyyy-dd-MM hh:mm:ss.z"))
				SubmitArgs.append(arg)
			if arg == "comment":
				Data.append(self.RPCommentEdit.toPlainText())
				SubmitArgs.append(arg)
		try:
			updateGenericTable(self.connection,"shipment",SubmitArgs,Data,id=int(self.RPIDEdit.text()))
		except:
			print("Failed to submit the shipment record")
			return

		self.RPIDLabel.deleteLater()
		self.RPIDEdit.deleteLater()
		self.RPFetchButton.deleteLater()

		self.RPSenderLabel.deleteLater()
		self.RPSenderEdit.deleteLater()
		self.RPDateSentLabel.deleteLater()
		self.RPDateSentEdit.deleteLater()

		self.RPReceiverLabel.deleteLater()
		self.RPReceiverEdit.deleteLater()
		self.RPDateReceivedLabel.deleteLater()
		self.RPDateReceivedEdit.deleteLater()

		self.RPOriginLabel.deleteLater()
		self.RPOriginCombo.deleteLater()
		self.RPDestinationLabel.deleteLater()
		self.RPDestinationCombo.deleteLater()

		self.RPCarrierLabel.deleteLater()
		self.RPCarrierEdit.deleteLater()
		self.RPTrackingLabel.deleteLater()
		self.RPTrackingEdit.deleteLater()

		self.RPCommentLabel.deleteLater()
		self.RPCommentEdit.deleteLater()
		self.RPFeedBackLabel.deleteLater()
		self.RPSubmitButton.deleteLater()

		self.receivePackageTab.layout.removeWidget(self.RPIDEdit)
		self.receivePackageTab.layout.removeWidget(self.RPIDLabel)
		self.receivePackageTab.layout.removeWidget(self.RPFetchButton)

		self.receivePackageTab.layout.removeWidget(self.RPSenderLabel)
		self.receivePackageTab.layout.removeWidget(self.RPSenderEdit)
		self.receivePackageTab.layout.removeWidget(self.RPDateSentLabel)
		self.receivePackageTab.layout.removeWidget(self.RPDateSentEdit)

		self.receivePackageTab.layout.removeWidget(self.RPReceiverLabel)
		self.receivePackageTab.layout.removeWidget(self.RPReceiverEdit)
		self.receivePackageTab.layout.removeWidget(self.RPDateReceivedLabel)
		self.receivePackageTab.layout.removeWidget(self.RPDateReceivedEdit)

		self.receivePackageTab.layout.removeWidget(self.RPOriginLabel)
		self.receivePackageTab.layout.removeWidget(self.RPOriginCombo)
		self.receivePackageTab.layout.removeWidget(self.RPDestinationLabel)
		self.receivePackageTab.layout.removeWidget(self.RPDestinationCombo)

		self.receivePackageTab.layout.removeWidget(self.RPCarrierLabel)
		self.receivePackageTab.layout.removeWidget(self.RPCarrierEdit)
		self.receivePackageTab.layout.removeWidget(self.RPTrackingLabel)
		self.receivePackageTab.layout.removeWidget(self.RPTrackingEdit)

		self.receivePackageTab.layout.removeWidget(self.RPCommentLabel)
		self.receivePackageTab.layout.removeWidget(self.RPCommentEdit)
		self.receivePackageTab.layout.removeWidget(self.RPFeedBackLabel)
		self.receivePackageTab.layout.removeWidget(self.RPSubmitButton)

		self.RPFeedBackLabel = QLabel("Submitted")
		self.RPFeedBackLabel.setStyleSheet("color:green")
		self.RPContinueButton =  QPushButton('Continue')
		self.RPContinueButton.clicked.connect(self.recreateRP)
		self.RPCloseButton =  QPushButton('Close')
		self.RPCloseButton.clicked.connect(self.closeRP)
		self.receivePackageTab.layout.addWidget(self.RPFeedBackLabel,1,0,1,2)
		self.receivePackageTab.layout.addWidget(self.RPContinueButton,1,3,1,1)
		self.receivePackageTab.layout.addWidget(self.RPCloseButton,1,4,1,1)
		return

	def recreateRP(self):
		self.closeTab(self.MainTabs.currentIndex())
		self.receivePackage()
	
	def closeRP(self):
		self.closeTab(self.MainTabs.currentIndex())

	##########################################################################
	##  Functions for Shipment  (END)
	##########################################################################

	def displayUsers(self):
		self.processing.append("UserDisplay")
		UserDisplayTab = QWidget()
		
		# Add tabs
		self.MainTabs.addTab(UserDisplayTab,"User List")

		UserDisplayTab.layout = QGridLayout()

		dataList = ([describeTable(self.connection, "people")]+retrieveGenericTable(self.connection,"people"))

		proxy = QtDBTableWidget(dataList,0)

		lineEdit       = QLineEdit()
		lineEdit.textChanged.connect(proxy.on_lineEdit_textChanged)
		view           = QTableView()
		view.setSortingEnabled(True)
		comboBox       = QComboBox()
		comboBox.addItems(["{0}".format(x) for x in dataList[0]])
		comboBox.currentIndexChanged.connect(proxy.on_comboBox_currentIndexChanged)
		label          = QLabel()
		label.setText("Regex Filter")

		view.setModel(proxy)
		view.setSelectionBehavior(QAbstractItemView.SelectRows)
		view.setSelectionMode(QAbstractItemView.MultiSelection)
		view.setEditTriggers(QAbstractItemView.NoEditTriggers)

		UserDisplayTab.layout.addWidget(lineEdit, 0, 1, 1, 1)
		UserDisplayTab.layout.addWidget(view, 1, 0, 1, 3)
		UserDisplayTab.layout.addWidget(comboBox, 0, 2, 1, 1)
		UserDisplayTab.layout.addWidget(label, 0, 0, 1, 1)

		UserDisplayTab.setLayout(UserDisplayTab.layout)
		self.MainTabs.setCurrentWidget(UserDisplayTab)

	##########################################################################
	##  Functions for Managing Users
	##########################################################################

	def addUsers(self):
		## Fixme: disable the menu when tab exists, relieaze the menu when closed.
		self.processing.append("AddUser")
		AddingUserTab = QWidget()
		
		# Add tabs
		self.MainTabs.addTab(AddingUserTab,"Adding User")
		self.AddUserAction.setDisabled(True)
		AddingUserTab.layout = QGridLayout(self)
		AddUserLabel = QLabel('<font size="12"> Creating new user: </font>')
		AddUserLabel.setMaximumHeight(60)

		self.AUNewUsernameLabel = QLabel('username')
		self.AUNewUsernameEdit = QLineEdit('')
		self.AUNewUsernameEdit.setEchoMode(QLineEdit.Normal)
		self.AUNewUsernameEdit.setPlaceholderText('new username')

		self.AUPasswordLabel = QLabel('password')
		self.AUPasswordEdit = QLineEdit('')
		self.AUPasswordEdit.setEchoMode(QLineEdit.Password)
		self.AUPasswordEdit.setPlaceholderText('new password')

		self.AURePasswordLabel = QLabel('re-password')
		self.AURePasswordEdit = QLineEdit('')
		self.AURePasswordEdit.setEchoMode(QLineEdit.Password)
		self.AURePasswordEdit.setPlaceholderText('verify password')

		self.AUNameLabel = QLabel('*name')
		self.AUNameEdit = QLineEdit('')
		self.AUNameEdit.setEchoMode(QLineEdit.Normal)
		self.AUNameEdit.setPlaceholderText('short name')

		self.AUFullNameLabel = QLabel('*full name')
		self.AUFullNameEdit = QLineEdit('')
		self.AUFullNameEdit.setEchoMode(QLineEdit.Normal)
		self.AUFullNameEdit.setPlaceholderText('full name')

		self.AUEmailLabel = QLabel('e-mail')
		self.AUEmailEdit = QLineEdit('')
		self.AUEmailEdit.setEchoMode(QLineEdit.Normal)
		self.AUEmailEdit.setPlaceholderText('your_email@host.name')

		self.AUInstLabel = QLabel('institute')
		self.AUInstComboBox       = QComboBox()
		siteList = getByColumnName("description",describeInstitute(self.connection),retrieveAllInstitute(self.connection))
		self.AUInstComboBox.addItems(["{0}".format(x) for x in siteList])

		self.AUFeedBackLabel = QLabel()

		self.AUSubmitButton = QPushButton('Submit')
		self.AUSubmitButton.clicked.connect(self.submitAURequest)

		AddingUserTab.layout.addWidget(AddUserLabel,0,0,1,4)
		AddingUserTab.layout.addWidget(self.AUNewUsernameLabel,1,0,1,1)
		AddingUserTab.layout.addWidget(self.AUNewUsernameEdit,1,1,1,1)
		AddingUserTab.layout.addWidget(self.AUPasswordLabel,2,0,1,1)
		AddingUserTab.layout.addWidget(self.AUPasswordEdit,2,1,1,1)
		AddingUserTab.layout.addWidget(self.AURePasswordLabel,3,0,1,1)
		AddingUserTab.layout.addWidget(self.AURePasswordEdit,3,1,1,1)
		AddingUserTab.layout.addWidget(self.AUNameLabel,4,0,1,1)
		AddingUserTab.layout.addWidget(self.AUNameEdit,4,1,1,1)
		AddingUserTab.layout.addWidget(self.AUFullNameLabel,4,2,1,1)
		AddingUserTab.layout.addWidget(self.AUFullNameEdit,4,3,1,1)
		AddingUserTab.layout.addWidget(self.AUEmailLabel,5,0,1,1)
		AddingUserTab.layout.addWidget(self.AUEmailEdit,5,1,1,1)
		AddingUserTab.layout.addWidget(self.AUInstLabel,5,2,1,1)
		AddingUserTab.layout.addWidget(self.AUInstComboBox,5,3,1,1)
		AddingUserTab.layout.addWidget(self.AUFeedBackLabel,6,0,1,3)
		AddingUserTab.layout.addWidget(self.AUSubmitButton,6,3,1,1)

		AddingUserTab.setLayout(AddingUserTab.layout)
		self.MainTabs.setCurrentWidget(AddingUserTab)

		#self.enableMenuItem(self.AddingUserTab, False)
	
	def submitAURequest(self):
		# check the password consistency
		if self.AUNewUsernameEdit.text() == "" or self.AUNameEdit.text() == "" or self.AUFullNameEdit.text() ==  "":
			self.AUFeedBackLabel.setText("key blank is empty")
			return
		if self.AUPasswordEdit.text() != self.AURePasswordEdit.text():
			self.AUFeedBackLabel.setText("passwords are inconsistent")
			self.AUPasswordEdit.setText("")
			self.AURePasswordEdit.setText("")
			return

		try:
			InstituteInfo = retrieveWithConstraint(self.connection, "institute", description = str(self.AUInstComboBox.currentText()))
			InstName = getByColumnName("institute",describeTable(self.connection,"institute"),InstituteInfo)
			TimeZone = getByColumnName("timezone",describeTable(self.connection,"institute"),InstituteInfo)
		except:
			self.AUFeedBackLabel.setText("Failed to extract institute info, try to reconnect to DB")
		Args =  describeTable(self.connection, "people")
		Data = []
		Data.append(self.AUNewUsernameEdit.text())
		Data.append(self.AUNameEdit.text())
		Data.append(self.AUFullNameEdit.text())
		Data.append(self.AUEmailEdit.text())
		Data.append(InstName[0])
		Data.append(self.AUPasswordEdit.text())
		Data.append(TimeZone[0])
		Data.append(0)
		try:
			createNewUser(self.connection, Args, Data)
			self.AUFeedBackLabel.setText("Query submitted")
			self.AUFeedBackLabel.setStyleSheet("color:green")
			return 
		except:
			print("submitFailed")
			return

	def updateProfile(self):
		pass

	##########################################################################
	##  Functions for Adding Users (END)
	##########################################################################
	
	##########################################################################
	##  Functions for Institutes display
	##########################################################################
	def displayInstitue(self):
		self.processing.append("InstituteDisplay")

		displayInstiTab = QWidget()
		
		# Add tabs
		self.MainTabs.addTab(displayInstiTab,"Institutes")

		displayInstiTab.layout = QGridLayout(self)

		dataList = ([describeInstitute(self.connection)]+retrieveAllInstitute(self.connection))

		proxy = QtDBTableWidget(dataList,0)

		lineEdit       = QLineEdit()
		lineEdit.textChanged.connect(proxy.on_lineEdit_textChanged)
		view           = QTableView()
		view.setSortingEnabled(True)
		comboBox       = QComboBox()
		comboBox.addItems(["{0}".format(x) for x in dataList[0]])
		comboBox.currentIndexChanged.connect(proxy.on_comboBox_currentIndexChanged)
		label          = QLabel()
		label.setText("Regex Filter")

		view.setModel(proxy)
		view.setSelectionBehavior(QAbstractItemView.SelectRows)
		view.setSelectionMode(QAbstractItemView.MultiSelection)
		view.setEditTriggers(QAbstractItemView.NoEditTriggers)

		displayInstiTab.layout.addWidget(lineEdit, 0, 1, 1, 1)
		displayInstiTab.layout.addWidget(view, 1, 0, 1, 3)
		displayInstiTab.layout.addWidget(comboBox, 0, 2, 1, 1)
		displayInstiTab.layout.addWidget(label, 0, 0, 1, 1)

		displayInstiTab.setLayout(displayInstiTab.layout)
		self.MainTabs.setCurrentWidget(displayInstiTab)

	##########################################################################
	##  Functions for Institutes display (END)
	##########################################################################

	##########################################################################
	##  Functions for Complaint submission
	##########################################################################
	def fileComplaint(self):
		self.ComplaintTab = QWidget()
		
		# Add tabs
		self.MainTabs.addTab(self.ComplaintTab,"Complaint")
		self.ComplaintAction.setDisabled(True)
		self.ComplaintTab.layout = QGridLayout(self)
		ComplaintLabel = QLabel('<font size="12"> Submit the complaint </font>')
		ComplaintLabel.setMaximumHeight(60)

		self.FCNameLabel = QLabel('Name:')
		self.FCNameEdit = QLineEdit('')
		self.FCNameEdit.setEchoMode(QLineEdit.Normal)
		TableInfo =  describeTable(self.connection, "people")
		UserInfo = retrieveWithConstraint(self.connection, "people", username=self.TryUsername)
		ShortName = getByColumnName("name",TableInfo,UserInfo)
		if len(ShortName)>0:
			self.FCNameEdit.setText(str(ShortName[0]))

		self.FCDateTimeLabel = QLabel('Date(local)')
		self.FCDateTimeEdit = QDateTimeEdit()
		self.FCDateTimeEdit.setDateTime(QDateTime.currentDateTime())

		self.FCCommentLabel = QLabel('Comment:')
		self.FCCommentEdit = QTextEdit('')
		self.FCCommentEdit.setPlaceholderText("Your comment")

		self.FCFeedBackLabel = QLabel()

		self.FCSubmitButton = QPushButton('Submit')
		self.FCSubmitButton.clicked.connect(self.submitFCRequest)

		self.ComplaintTab.layout.addWidget(ComplaintLabel,0,0,1,4,Qt.AlignTop)
		self.ComplaintTab.layout.addWidget(self.FCNameLabel,1,0,1,1)
		self.ComplaintTab.layout.addWidget(self.FCNameEdit,1,1,1,1)
		self.ComplaintTab.layout.addWidget(self.FCDateTimeLabel,1,2,1,1)
		self.ComplaintTab.layout.addWidget(self.FCDateTimeEdit,1,3,1,1)
		self.ComplaintTab.layout.addWidget(self.FCCommentLabel,2,0,1,4,Qt.AlignLeft)
		self.ComplaintTab.layout.addWidget(self.FCCommentEdit,3,0,6,4)
		self.ComplaintTab.layout.addWidget(self.FCFeedBackLabel,9,0,1,2)
		self.ComplaintTab.layout.addWidget(self.FCSubmitButton,9,3,1,1)

		self.ComplaintTab.setLayout(self.ComplaintTab.layout)
		self.MainTabs.setCurrentWidget(self.ComplaintTab)
	
	def submitFCRequest(self):
		if self.FCNameEdit.text() == "" or self.FCCommentEdit.toPlainText()=="":
			self.FCFeedBackLabel.setText("Please make sure the name/comment are filled")
			return
		Args = describeTable(self.connection, "complaint")
		Data = []
		Data.append(self.FCDateTimeEdit.dateTime().toUTC().toString("yyyy-dd-MM hh:mm:ss.z"))
		Data.append(self.FCCommentEdit.toPlainText())
		Data.append(self.FCNameEdit.text())
		#print(Data)
		try:
			insertGenericTable(self.connection,"complaint",Args,Data)
		except:
			print("Failed to submit the text")
			return
		self.FCNameLabel.deleteLater()
		self.FCNameEdit.deleteLater()
		self.FCDateTimeLabel.deleteLater()
		self.FCDateTimeEdit.deleteLater()
		self.FCCommentLabel.deleteLater()
		self.FCCommentEdit.deleteLater()
		self.FCFeedBackLabel.deleteLater()
		self.FCSubmitButton.deleteLater()

		self.ComplaintTab.layout.removeWidget(self.FCNameLabel)
		self.ComplaintTab.layout.removeWidget(self.FCNameEdit)
		self.ComplaintTab.layout.removeWidget(self.FCDateTimeLabel)
		self.ComplaintTab.layout.removeWidget(self.FCDateTimeEdit)
		self.ComplaintTab.layout.removeWidget(self.FCCommentLabel)
		self.ComplaintTab.layout.removeWidget(self.FCCommentEdit)
		self.ComplaintTab.layout.removeWidget(self.FCFeedBackLabel)
		self.ComplaintTab.layout.removeWidget(self.FCSubmitButton)

		self.FCFeedBackLabel = QLabel("Submitted")
		self.FCFeedBackLabel.setStyleSheet("color:green")
		self.FCContinueButton =  QPushButton('Continue')
		self.FCContinueButton.clicked.connect(self.recreateFC)
		self.ComplaintTab.layout.addWidget(self.FCFeedBackLabel,1,0,1,2)
		self.ComplaintTab.layout.addWidget(self.FCContinueButton,1,3,1,1)
		return

	def recreateFC(self):
		self.closeTab(self.MainTabs.currentIndex())
		self.fileComplaint()

	##########################################################################
	##  Functions for Complaint submission (END)
	##########################################################################

	def createHeadLine(self):
		self.deactivateMenuBar()
		self.HeadBox = QGroupBox()

		self.HeadLayout = QHBoxLayout()

		UsernameLabel = QLabel("Username:")
		self.UsernameEdit = QLineEdit('')
		self.UsernameEdit.setEchoMode(QLineEdit.Normal)
		self.UsernameEdit.setPlaceholderText('Your username')
		self.UsernameEdit.setMinimumWidth(150)
		self.UsernameEdit.setMaximumHeight(20)

		PasswordLabel = QLabel("Password:")
		self.PasswordEdit = QLineEdit('')
		self.PasswordEdit.setEchoMode(QLineEdit.Password)
		self.PasswordEdit.setPlaceholderText('Your password')
		self.PasswordEdit.setMinimumWidth(150)
		self.PasswordEdit.setMaximumHeight(20)

		HostLabel = QLabel("Host IP:")
		self.HostEdit = QLineEdit('128.146.38.1')
		self.HostEdit.setEchoMode(QLineEdit.Normal)
		self.HostEdit.setMinimumWidth(150)
		self.HostEdit.setMaximumHeight(30)

		DatabaseLabel = QLabel("Database:")
		self.DatabaseEdit = QLineEdit('SampleDB')
		self.DatabaseEdit.setEchoMode(QLineEdit.Normal)
		self.DatabaseEdit.setMinimumWidth(150)
		self.DatabaseEdit.setMaximumHeight(30)
		
		self.ConnectButton = QPushButton("&Connect to DB")
		self.ConnectButton.clicked.connect(self.connectDB)

		self.HeadLayout.addWidget(UsernameLabel)
		self.HeadLayout.addWidget(self.UsernameEdit)
		self.HeadLayout.addWidget(PasswordLabel)
		self.HeadLayout.addWidget(self.PasswordEdit)
		self.HeadLayout.addWidget(HostLabel)
		self.HeadLayout.addWidget(self.HostEdit)
		self.HeadLayout.addWidget(DatabaseLabel)
		self.HeadLayout.addWidget(self.DatabaseEdit)
		self.HeadLayout.addWidget(self.ConnectButton)

		self.HeadBox.setLayout(self.HeadLayout)

		self.mainLayout.addWidget(self.HeadBox, 0, 0, self.GroupBoxSeg[0], 1)

	def destroyHeadLine(self):
		self.HeadBox.deleteLater()
		self.mainLayout.removeWidget(self.HeadBox)

	def clearLogInfo(self):
		self.connection = "Offline"
	
	def connectedHeadLine(self):
		self.activateMenuBar()
		self.HeadBox.deleteLater()
		self.mainLayout.removeWidget(self.HeadBox)

		self.HeadBox = QGroupBox()
		self.HeadLayout = QHBoxLayout()

		WelcomeLabel = QLabel("Hello,{}!".format(self.TryUsername))

		statusString, colorString = checkDBConnection(self.connection)		
		DBStatusLabel = QLabel()
		DBStatusLabel.setText("Database:{}/{}".format(self.TryHostAddress,self.TryDatabase))
		DBStatusValue = QLabel()
		DBStatusValue.setText(statusString)
		DBStatusValue.setStyleSheet(colorString)

		self.SwitchButton = QPushButton("&Switch DB")
		self.SwitchButton.clicked.connect(self.destroyHeadLine)
		self.SwitchButton.clicked.connect(self.clearLogInfo)
		self.SwitchButton.clicked.connect(self.createHeadLine)

		self.HeadLayout.addWidget(WelcomeLabel)
		self.HeadLayout.addStretch(1)
		self.HeadLayout.addWidget(DBStatusLabel)
		self.HeadLayout.addWidget(DBStatusValue)
		self.HeadLayout.addWidget(self.SwitchButton)

		self.HeadBox.setLayout(self.HeadLayout)

		self.HeadBox.setLayout(self.HeadLayout)
		self.mainLayout.addWidget(self.HeadBox, 0, 0, self.GroupBoxSeg[0], 1)

	def createMain(self):
		self.MainBodyBox = QGroupBox()

		self.mainbodylayout = QHBoxLayout()

		self.MainTabs = QTabWidget()
		self.MainTabs.setTabsClosable(True)
		self.MainTabs.tabCloseRequested.connect(lambda index: self.closeTab(index))
		
		self.mainbodylayout.addWidget(self.MainTabs)

		self.MainBodyBox.setLayout(self.mainbodylayout)
		self.mainLayout.addWidget(self.MainBodyBox, sum(self.GroupBoxSeg[0:1]), 0, self.GroupBoxSeg[1], 1)

	def destroyMain(self):
		self.MainBodyBox.deleteLater()
		self.mainLayout.removeWidget(self.MainBodyBox)

	def createApp(self):
		self.AppOption = QGroupBox()
		self.StartLayout = QHBoxLayout()

		self.ResetButton = QPushButton("&Refresh")

		self.FinishButton = QPushButton("&Finish")
		self.FinishButton.setDefault(True)
		self.FinishButton.clicked.connect(self.closeWindow)

		self.StartLayout.addWidget(self.ResetButton)
		self.StartLayout.addWidget(self.FinishButton)
		self.StartLayout.addStretch(1)
		self.AppOption.setLayout(self.StartLayout)

		self.mainLayout.addWidget(self.AppOption, sum(self.GroupBoxSeg[0:2]), 0, self.GroupBoxSeg[2], 1)

	def destroyApp(self):
		self.AppOption.deleteLater()
		self.mainLayout.removeWidget(self.AppOption)

	def sendBackSignal(self):
		self.backSignal = True

	def connectDB(self):
		try:
			if isActive(self.connection):
				self.TryUsername = self.master.TryUsername
				self.TryHostAddress = self.master.TryHostAddress
				self.TryDatabase = self.master.TryDatabase
				self.connectedHeadLine()
				return
			self.TryUsername = self.UsernameEdit.text()
			self.TryPassword = self.PasswordEdit.text()
			self.TryHostAddress = self.HostEdit.text()
			self.TryDatabase = self.DatabaseEdit.text()

			if self.TryUsername == '':
				msg.information(None,"Error","Please enter a valid username", QMessageBox.Ok)
				return

			self.connection = QtStartConnection(self.TryUsername, self.TryPassword, self.TryHostAddress, self.TryDatabase)

			if isActive(self.connection):
				self.connectedHeadLine()
				GetTrimClass(self.connection)
		except Exception as err:
			print("Error in connecting to database, {}".format(repr(err)))

	def syncDB(self):
		pass

	def changeDBList(self):
		self.DBNames = DBNames[str(self.HostName.currentText())]
		self.DatabaseCombo.clear()
		self.DatabaseCombo.addItems(self.DBNames)
		self.DatabaseCombo.setCurrentIndex(0)

	def closeTab(self, index):
		self.releaseAction( self.MainTabs.tabText(index) )
		self.MainTabs.removeTab(index)

	def releaseAction(self, text):
		if text == "Adding User":
			self.AddUserAction.setDisabled(False)
			return
		if text == "Complaint":
			self.ComplaintAction.setDisabled(False)
			return
		if text == "Send Package":
			self.sendPackAction.setDisabled(False)
			return
		if text == "Receive Package":
			self.receivePackAction.setDisabled(False)
			return
		if text == "Insert Image":
			self.InsertImgAction.setDisabled(False)
			return

	def closeWindow(self):
		self.close()
		self.release()

	def occupied(self):
		self.master.DBConsoleButton.setDisabled(True)

	def release(self):
		self.master.DBConsoleButton.setDisabled(False)

	def closeEvent(self, event):
		#Fixme: strict criterias for process checking  should be added here:
		#if self.ProcessingTest == True:
		#	QMessageBox.critical(self, 'Critical Message', 'There is running process, can not close!')
		#	event.ignore()

		reply = QMessageBox.question(self, 'Window Close', 'Are you sure you want to exit, all unsubmitted content will be lost?',
			QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

		if reply == QMessageBox.Yes:
			print('DB console terminated')
			self.release()
			event.accept()
		else:
			event.ignore()

