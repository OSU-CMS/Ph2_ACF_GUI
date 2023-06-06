from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtGui import (QPixmap, QTextCursor, QColor)
from PyQt5.QtWidgets import (QAbstractItemView, QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit, QListWidget, QPlainTextEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableView, QTableWidget, QTableWidgetItem, QTabWidget, QTextEdit, QTreeWidget, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox, QSplitter)

import sys
import os
import re
import subprocess
import threading
import time
from datetime import datetime
import random
from subprocess import Popen, PIPE
import logging

from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.guiUtils import *
#from Gui.QtGUIutils.QtStartWindow import *
from Gui.QtGUIutils.QtCustomizeWindow import *
from Gui.QtGUIutils.QtTableWidget import *
from Gui.QtGUIutils.QtMatplotlibUtils import *
from Gui.QtGUIutils.QtLoginDialog import *
from Gui.python.ResultTreeWidget import *
from Gui.python.TestValidator import *
from Gui.python.ANSIColoringParser import *
from Gui.python.TestHandler import *

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class QtRunWindow(QWidget):
	resized = pyqtSignal()
	def __init__(self,master,info,firmware):
		super(QtRunWindow,self).__init__()
		self.master = master
		self.master.globalStop.connect(self.urgentStop)
		
		#self.LogoGroupBox = self.master.LogoGroupBox
		self.firmware = firmware
		self.info = info
		if "AllScan_Tuning" in self.info[1]:
			runTestList = pretuningList
			runTestList.extend(tuningList*len(defaultTargetThr))
			runTestList.extend(posttuningList)
			CompositeList.update({'AllScan_Tuning':runTestList})

		elif isCompositeTest(self.info[1]):
			runTestList = CompositeList[self.info[1]]
		else:
			runTestList = self.info[1]

		self.connection = self.master.connection
		self.firmwareName = self.firmware.getBoardName()
		self.ModuleMap = dict()
		self.ModuleType = self.firmware.getModuleByIndex(0).getModuleType()
		self.RunNumber = "-1"

		#Add TestProcedureHandler
		self.testHandler = TestHandler(self,master,info,firmware)
		self.testHandler.powerSignal.connect(self.master.HVpowersupply.TurnOffHV)
		self.testHandler.powerSignal.connect(self.master.LVpowersupply.TurnOff)

		self.GroupBoxSeg = [1, 10,  1]
		self.HorizontalSeg = [3, 5]
		self.VerticalSegCol0 = [1,3]
		self.VerticalSegCol1 = [2,2]
		self.DisplayH = self.height()*3./7
		self.DisplayW = self.width()*3./7

		self.processingFlag = False
		self.ProgressBarList = []
		self.input_dir = ''
		self.output_dir = ''
		self.config_file = '' #os.environ.get('GUI_dir')+ConfigFiles.get(self.calibration, "None")
		self.rd53_file  = {}
		self.grade = -1
		self.currentTest = ""
		self.outputFile = ""
		self.errorFile = ""

		self.backSignal = False
		self.haltSignal = False
		self.finishSingal = False
		self.proceedSignal = False

		self.runNext = threading.Event()
		self.testIndexTracker = -1
		self.listWidgetIndex = 0
		self.outputDirQueue = []
		#Fixme: QTimer to be added to update the page automatically
		self.grades = []
		self.modulestatus = []
		self.autoSave = False

		self.mainLayout = QGridLayout()
		self.setLayout(self.mainLayout)

		self.setLoginUI()
		#self.initializeRD53Dict()
		self.createHeadLine()
		self.createMain()
		self.createApp()
		self.occupied()

		self.resized.connect(self.rescaleImage)

		#added from Bowen
		print('test list should be {0}'.format(self.info[1]))
		self.j = 0
		#stepWiseGlobalValue[0]['TargetThr'] = defaultTargetThr[0]
		#if len(runTestList)>1:
		for i in range(len(runTestList)):
			if runTestList[i] == 'ThresholdAdjustment':
				self.j += 1
			if self.j == 0:
				stepWiseGlobalValue[i]['TargetThr'] = defaultTargetThr[self.j]
			else:
				stepWiseGlobalValue[i]['TargetThr'] = defaultTargetThr[self.j-1]
		 
		logger.info(stepWiseGlobalValue)


	def setLoginUI(self):
		X = self.master.dimension.width()/10
		Y = self.master.dimension.height()/10
		Width = self.master.dimension.width()*8./10
		Height = self.master.dimension.height()*8./10
		self.setGeometry(X, Y, Width, Height)  
		self.setWindowTitle('Run Control Page') 
		self.DisplayH = self.height()*3./7
		self.DisplayW = self.width()*3./7 
		self.show()

	def createHeadLine(self):
		self.HeadBox = QGroupBox()

		self.HeadLayout = QHBoxLayout()

		HeadLabel = QLabel('<font size="4"> Module: {0}  Test: {1} </font>'.format(self.info[0], self.info[1]))
		HeadLabel.setMaximumHeight(30)

		statusString, colorString = checkDBConnection(self.connection)
		StatusLabel = QLabel()
		StatusLabel.setText(statusString)
		StatusLabel.setStyleSheet(colorString)

		self.HeadLayout.addWidget(HeadLabel)
		self.HeadLayout.addStretch(1)
		self.HeadLayout.addWidget(StatusLabel)

		self.HeadBox.setLayout(self.HeadLayout)

		self.mainLayout.addWidget(self.HeadBox, 0, 0, self.GroupBoxSeg[0], 1)

	def destroyHeadLine(self):
		self.HeadBox.deleteLater()
		self.mainLayout.removeWidget(self.HeadBox)
	
	def createMain(self):
		self.testIndexTracker = 0
		self.MainBodyBox = QGroupBox()

		mainbodylayout = QHBoxLayout()

		kMinimumWidth = 120
		kMaximumWidth = 150
		kMinimumHeight = 30
		kMaximumHeight = 80

		# Splitters
		MainSplitter = QSplitter(Qt.Horizontal)
		LeftColSplitter = QSplitter(Qt.Vertical)
		RightColSplitter = QSplitter(Qt.Vertical)

		#Group Box for controller
		ControllerBox = QGroupBox()
		ControllerSP = ControllerBox.sizePolicy()
		ControllerSP.setVerticalStretch(self.VerticalSegCol0[0])
		ControllerBox.setSizePolicy(ControllerSP)

		self.ControlLayout = QGridLayout()

		self.CustomizedButton = QPushButton("&Customize...")
		self.CustomizedButton.clicked.connect(self.customizeTest)
		self.ResetButton = QPushButton("&Reset")
		self.ResetButton.clicked.connect(self.resetConfigTest)
		self.RunButton = QPushButton("&Run")
		self.RunButton.setDefault(True)
		self.RunButton.clicked.connect(self.resetConfigTest)
		self.RunButton.clicked.connect(self.initialTest)
		#self.RunButton.clicked.connect(self.runTest)
		#self.ContinueButton = QPushButton("&Continue")
		#self.ContinueButton.clicked.connect(self.sendProceedSignal)
		self.AbortButton = QPushButton("&Abort")
		self.AbortButton.clicked.connect(self.abortTest)
		#self.SaveButton = QPushButton("&Save")
		#self.SaveButton.clicked.connect(self.saveTest)
		self.saveCheckBox = QCheckBox("&auto-save to DB")
		self.saveCheckBox.setMaximumHeight(30)
		self.saveCheckBox.setChecked(self.testHandler.autoSave)
		if not isActive(self.connection):
			self.saveCheckBox.setChecked(False)
			self.testHandler.autoSave=False
			self.saveCheckBox.setDisabled(True)
		self.saveCheckBox.clicked.connect(self.setAutoSave)
##### previous layout ##########
		'''
		self.ControlLayout.addWidget(self.CustomizedButton,0,0,1,2)
		self.ControlLayout.addWidget(self.ResetButton,0,2,1,1)
		self.ControlLayout.addWidget(self.RunButton,1,0,1,1)
		self.ControlLayout.addWidget(self.AbortButton,1,1,1,1)
		self.ControlLayout.addWidget(self.saveCheckBox,1,2,1,1)
		'''
		if self.master.expertMode == True:
			self.ControlLayout.addWidget(self.RunButton,0,0,1,1)
			self.ControlLayout.addWidget(self.AbortButton,0,1,1,1)
			self.ControlLayout.addWidget(self.ResetButton,0,2,1,1)
			self.ControlLayout.addWidget(self.saveCheckBox,1,0,1,1)

		else:
			pass
		#	self.ControlLayout.addWidget(self.RunButton,0,0,1,1)
		#	self.ControlLayout.addWidget(self.AbortButton,0,1,1,1)
		#	self.saveCheckBox.setDisabled(True)
		#	self.ControlLayout.addWidget(self.saveCheckBox,0,2,1,1)

		ControllerBox.setLayout(self.ControlLayout)

		#Group Box for ternimal display
		TerminalBox = QGroupBox("&Terminal")
		TerminalSP = TerminalBox.sizePolicy()
		TerminalSP.setVerticalStretch(self.VerticalSegCol0[1])
		TerminalBox.setSizePolicy(TerminalSP)
		TerminalBox.setMinimumWidth(400)

		ConsoleLayout = QGridLayout()
		
		self.ConsoleView = QPlainTextEdit()
		self.ConsoleView.setStyleSheet("QTextEdit { background-color: rgb(10, 10, 10); color : white; }")
		#self.ConsoleView.setCenterOnScroll(True)
		self.ConsoleView.ensureCursorVisible()
		
		ConsoleLayout.addWidget(self.ConsoleView)
		TerminalBox.setLayout(ConsoleLayout)

		#Group Box for output display
		OutputBox = QGroupBox("&Result")
		OutputBoxSP = OutputBox.sizePolicy()
		OutputBoxSP.setVerticalStretch(self.VerticalSegCol1[0])
		OutputBox.setSizePolicy(OutputBoxSP)

		OutputLayout = QGridLayout()
		self.ResultWidget = ResultTreeWidget(self.info,self.DisplayW,self.DisplayH,self.master)
		OutputLayout.addWidget(self.ResultWidget,0,0,1,1)
		OutputBox.setLayout(OutputLayout)

		#Group Box for history
		self.HistoryBox = QGroupBox("&History")
		HistoryBoxSP = self.HistoryBox.sizePolicy()
		HistoryBoxSP.setVerticalStretch(self.VerticalSegCol1[1])
		self.HistoryBox.setSizePolicy(HistoryBoxSP)

		self.HistoryLayout = QGridLayout()
		#self.StatusCanvas = RunStatusCanvas(parent=self,width=5, height=4, dpi=100)
		self.StatusTable = QTableWidget()
		self.header = ["TestName"]
		for key in self.testHandler.rd53_file.keys():
			ChipName = key.split("_")
			self.header.append("Module{}_Chip{}".format(ChipName[0],ChipName[2]))
		self.StatusTable.setColumnCount(len(self.header))
		self.StatusTable.setHorizontalHeaderLabels(self.header)
		self.StatusTable.setEditTriggers(QAbstractItemView.NoEditTriggers)
		self.HistoryLayout.addWidget(self.StatusTable)
		self.HistoryBox.setLayout(self.HistoryLayout)
		
		self.TempBox = QGroupBox("&Chip Temperature")
		TempBoxSP = self.TempBox.sizePolicy()
		TempBoxSP.setVerticalStretch(self.VerticalSegCol1[1])
		self.TempBox.setSizePolicy(TempBoxSP)
		self.TempLayout = QGridLayout()
		self.Temp = QLabel("temp will go here")
		self.TempLayout.addWidget(self.Temp)
		self.TempBox.setLayout(self.TempLayout)
		#self.TempBox.setMaximumWidth(400)
		#self.TempBox.setMaximumHeight(400)
		

		
		LeftColSplitter.addWidget(ControllerBox)
		LeftColSplitter.addWidget(TerminalBox)
		LeftColSplitter.addWidget(self.TempBox)
		RightColSplitter.addWidget(OutputBox)
		RightColSplitter.addWidget(self.HistoryBox)


		LeftColSplitterSP = LeftColSplitter.sizePolicy()
		LeftColSplitterSP.setHorizontalStretch(self.HorizontalSeg[0])
		LeftColSplitter.setSizePolicy(LeftColSplitterSP)

		RightColSplitterSP = RightColSplitter.sizePolicy()
		RightColSplitterSP.setHorizontalStretch(self.HorizontalSeg[1])
		RightColSplitter.setSizePolicy(RightColSplitterSP)

		MainSplitter.addWidget(LeftColSplitter)
		MainSplitter.addWidget(RightColSplitter)

		mainbodylayout.addWidget(MainSplitter)
		#mainbodylayout.addWidget(ControllerBox, sum(self.VerticalSegCol0[:0]), sum(self.HorizontalSeg[:0]), self.VerticalSegCol0[0], self.HorizontalSeg[0])
		#mainbodylayout.addWidget(TerminalBox, sum(self.VerticalSegCol0[:1]), sum(self.HorizontalSeg[:0]), self.VerticalSegCol0[1], self.HorizontalSeg[0])
		#mainbodylayout.addWidget(OutputBox, sum(self.VerticalSegCol1[:0]), sum(self.HorizontalSeg[:1]), self.VerticalSegCol1[0], self.HorizontalSeg[1])
		#mainbodylayout.addWidget(HistoryBox, sum(self.VerticalSegCol1[:1]), sum(self.HorizontalSeg[:1]), self.VerticalSegCol1[1], self.HorizontalSeg[1])


		self.MainBodyBox.setLayout(mainbodylayout)
		self.mainLayout.addWidget(self.MainBodyBox, sum(self.GroupBoxSeg[0:1]), 0, self.GroupBoxSeg[1], 1)

	def destroyMain(self):
		self.MainBodyBox.deleteLater()
		self.mainLayout.removeWidget(self.MainBodyBox)

	def createApp(self):
		self.AppOption = QGroupBox()
		self.StartLayout = QHBoxLayout()

		self.ConnectButton = QPushButton("&Connect to DB")
		self.ConnectButton.clicked.connect(self.connectDB)

		self.BackButton = QPushButton("&Back")
		self.BackButton.clicked.connect(self.sendBackSignal)
		self.BackButton.clicked.connect(self.closeWindow)
		self.BackButton.clicked.connect(self.creatStartWindow)

		self.FinishButton = QPushButton("&Finish")
		self.FinishButton.setDefault(True)
		self.FinishButton.clicked.connect(self.closeWindow)



		self.StartLayout.addStretch(1)
		if self.master.expertMode==True:
			self.StartLayout.addWidget(self.ConnectButton)
		self.StartLayout.addWidget(self.BackButton)
		self.StartLayout.addWidget(self.FinishButton)
		self.AppOption.setLayout(self.StartLayout)

		self.LogoGroupBox = QGroupBox("")
		self.LogoGroupBox.setCheckable(False)
		self.LogoGroupBox.setMaximumHeight(100)

		self.LogoLayout = QHBoxLayout()
		OSULogoLabel = QLabel()
		OSUimage = QImage("icons/osuicon.jpg").scaled(QSize(200,60), Qt.KeepAspectRatio, Qt.SmoothTransformation)
		OSUpixmap = QPixmap.fromImage(OSUimage)
		OSULogoLabel.setPixmap(OSUpixmap)
		CMSLogoLabel = QLabel()
		CMSimage = QImage("icons/cmsicon.png").scaled(QSize(200,60), Qt.KeepAspectRatio, Qt.SmoothTransformation)
		CMSpixmap = QPixmap.fromImage(CMSimage)
		CMSLogoLabel.setPixmap(CMSpixmap)
		self.LogoLayout.addWidget(OSULogoLabel)
		self.LogoLayout.addStretch(1)
		self.LogoLayout.addWidget(CMSLogoLabel)

		self.LogoGroupBox.setLayout(self.LogoLayout)

		self.mainLayout.addWidget(self.AppOption, sum(self.GroupBoxSeg[0:2]), 0, self.GroupBoxSeg[2], 1)
		self.mainLayout.addWidget(self.LogoGroupBox, sum(self.GroupBoxSeg[0:3]), 0, self.GroupBoxSeg[2], 1)

	def destroyApp(self):
		self.AppOption.deleteLater()
		self.mainLayout.removeWidget(self.AppOption)

	def closeWindow(self):
		self.close()

	def creatStartWindow(self):
		if self.backSignal == True and self.master.expertMode == True:
			self.master.openNewTest()

	def occupied(self):
		self.master.ProcessingTest = True

	def release(self):
		self.abortTest()
		self.master.ProcessingTest = False
		if self.master.expertMode == True:
			self.master.NewTestButton.setDisabled(False)
			self.master.LogoutButton.setDisabled(False)
			self.master.ExitButton.setDisabled(False)
		else:
			self.master.SimpleMain.RunButton.setDisabled(False)
			self.master.SimpleMain.StopButton.setDisabled(True)


	def refreshHistory(self,result):
		#self.dataList = getLocalRemoteTests(self.connection, self.info[0])
		#self.proxy = QtTableWidget(self.dataList)
		#self.view.setModel(self.proxy)
		#self.view.setEditTriggers(QAbstractItemView.NoEditTriggers)
		#self.view.update()
		print('attempting to update status in history')
		self.HistoryLayout.removeWidget(self.StatusTable)
		self.StatusTable.setRowCount(0)
		for index,test in enumerate(self.modulestatus):
			row = self.StatusTable.rowCount()
			self.StatusTable.setRowCount(row + 1)
			if isCompositeTest(self.info[1]):
				self.StatusTable.setItem(row,0,QTableWidgetItem(CompositeList[self.info[1]][index]))
			else:
				self.StatusTable.setItem(row,0,QTableWidgetItem(self.info[1]))
			for moduleKey in test.keys():
				for chipKey in test[moduleKey].keys():
					ChipID = "Module{}_Chip{}".format(moduleKey,chipKey)
					status = "Pass" if test[moduleKey][chipKey] == True else "Failed"
					if  ChipID in self.header:
						columnId = self.header.index(ChipID)
						self.StatusTable.setItem(row,columnId,QTableWidgetItem(status))
						if status == "Pass":
							self.StatusTable.item(row,columnId).setBackground(QColor(Qt.green))
						elif status == "Failed":
							self.StatusTable.item(row,columnId).setBackground(QColor(Qt.red))

		self.HistoryLayout.addWidget(self.StatusTable)
		
		

	def sendBackSignal(self):
		self.backSignal = True

	def sendProceedSignal(self):
		self.testHandler.proceedSignal = True
		#self.runNext.set()

	def connectDB(self):
		if isActive(self.master.connection):
			self.connection  = self.master.connection
			self.refresh()
			self.saveCheckBox.setDisabled(False)
			return

		LoginDialog = QtLoginDialog(self.master)
		response = LoginDialog.exec_()
		if response == QDialog.Accepted:
			self.connectDB()
		else:
			return


	def customizeTest(self):
		print("Customize configuration")
		self.CustomizedButton.setDisabled(True)
		self.ResetButton.setDisabled(True)
		self.RunButton.setDisabled(True)
		self.CustomizedWindow = QtCustomizeWindow(self, self.testHandler.rd53_file)
		self.CustomizedButton.setDisabled(False)
		self.ResetButton.setDisabled(False)
		self.RunButton.setDisabled(False)
		
	def resetConfigTest(self):
		self.testHandler.resetConfigTest()

	def initialTest(self):
		isReRun = False
		if "Re" in self.RunButton.text():
			isReRun = True
			self.grades = []
			if isCompositeTest(self.info[1]):
				for index in range(len(CompositeList[self.info[1]])):
					self.ResultWidget.ProgressBar[index].setValue(0)
			else:
				self.ResultWidget.ProgressBar[0].setValue(0)
		self.ResetButton.setDisabled(True)
		self.testHandler.runTest(isReRun)

	def abortTest(self):
		self.j = 0
		self.testHandler.abortTest()
	
	def urgentStop(self):
		self.testHandler.urgentStop()

	#######################################################################
	##  For result display
	#######################################################################
	def clickedOutputItem(self, qmodelindex):
		#Fixme: Extract the info from ROOT file
		item = self.ListWidget.currentItem()
		referName = item.text().split("_")[0]
		if referName in ["GainScan","Latency","NoiseScan","PixelAlive","SCurveScan","ThresholdEqualization","GainOptimization","ThresholdMinimization","InjectionDelay"]:
			self.ReferView = QPixmap(os.environ.get('GUI_dir')+'/Gui/test_plots/{0}.png'.format(referName)).scaled(QSize(self.DisplayW,self.DisplayH), Qt.KeepAspectRatio, Qt.SmoothTransformation)
			self.ReferLabel.setPixmap(self.ReferView)

	#######################################################################
	##  For real-time terminal display
	#######################################################################

	def updateConsoleInfo(self,text):
		textCursor = self.ConsoleView.textCursor()
		self.ConsoleView.setTextCursor(textCursor)
		self.ConsoleView.appendHtml(text)
		
	def finish(self,EnableReRun):
		self.RunButton.setDisabled(True)
		self.RunButton.setText("&Continue")
		self.finishSingal = True

		if EnableReRun:
				self.RunButton.setText("&Re-run")
				self.RunButton.setDisabled(False)


	def updateResult(self,newResult):
		# self.ResultWidget.updateResult("/Users/czkaiweb/Research/data")
		if self.master.expertMode:
			self.ResultWidget.updateResult(newResult)
		else:
			step, displayDict = newResult
			self.ResultWidget.updateDisplayList(step, displayDict)

	def updateIVResult(self,newResult):
		# self.ResultWidget.updateResult("/Users/czkaiweb/Research/data")
		if self.master.expertMode:
			self.ResultWidget.updateIVResult(newResult)
		else:
			step, displayDict = newResult
			self.ResultWidget.updateDisplayList(step, displayDict)

	def updateValidation(self,grade,passmodule):
		try:
			status = True
			self.grades.append(grade)
			self.modulestatus.append(passmodule)
		
			self.ResultWidget.StatusLabel[self.testIndexTracker-1].setText("Pass")
			self.ResultWidget.StatusLabel[self.testIndexTracker-1].setStyleSheet("color: green")
			for module in passmodule.values():
				if False in module.values():
					status = False
					self.ResultWidget.StatusLabel[self.testIndexTracker-1].setText("Failed")
					self.ResultWidget.StatusLabel[self.testIndexTracker-1].setStyleSheet("color: red")
		

			time.sleep(0.5)
			return status
		#self.StatusCanvas.renew()
		#self.StatusCanvas.update()
		#self.HistoryLayout.removeWidget(self.StatusCanvas)
		#self.HistoryLayout.addWidget(self.StatusCanvas)
		except Exception as err:
			logger.error(err)


	#######################################################################
	##  For real-time terminal display
	#######################################################################

	def refresh(self):
		self.destroyHeadLine()
		self.createHeadLine()
		self.destroyApp()
		self.createApp()

	def resizeEvent(self, event):
		self.resized.emit()
		return super(QtRunWindow, self).resizeEvent(event)

	def rescaleImage(self):
		self.DisplayH = self.height()*3./7
		self.DisplayW = self.width()*3./7
		self.ResultWidget.resizeImage(self.DisplayW,self.DisplayH)


	def setAutoSave(self):
		if self.testHandler.autoSave:
			self.testHandler.autoSave =  False
		else:
			self.testHandler.autoSave = True
		self.saveCheckBox.setChecked(self.testHandler.autoSave)
	

	def closeEvent(self, event):
		if self.processingFlag == True:
			event.ignore()
		
		else:
			reply = QMessageBox.question(self, 'Window Close', 'Are you sure you want to quit the test?',
				QMessageBox.No | QMessageBox.Yes, QMessageBox.No)

			if reply == QMessageBox.Yes:
				self.release()
				self.master.HVpowersupply.TurnOffHV()
				self.master.LVpowersupply.TurnOff()
				event.accept()
			else:
				self.backSignal = False
				event.ignore()


		


		
