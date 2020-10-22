from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox, QSplitter)

import sys
import os
from subprocess import Popen, PIPE

from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.guiUtils import *

class QtRunWindow(QWidget):
	def __init__(self,master, info):
		super(QtRunWindow,self).__init__()
		self.master = master
		self.info = info
		self.connection = self.master.connection
		self.GroupBoxSeg = [1, 10,  1]
		self.HorizontalSeg = [3, 5]
		self.VerticalSegCol0 = [1,3]
		self.VerticalSegCol1 = [3,2]
		self.processingFlag = False
		#Fixme: QTimer to be added to update the page automatically

		self.mainLayout = QGridLayout()
		self.setLayout(self.mainLayout)

		self.setLoginUI()
		self.createHeadLine()
		self.createMain()
		self.createApp()
		self.occupied()

		self.run_process = QProcess(self)
		self.run_process.readyReadStandardOutput.connect(self.on_readyReadStandardOutput)
		self.run_process.finished.connect(self.on_finish)

	def setLoginUI(self):
		self.setGeometry(200, 200, 1000, 1000)  
		self.setWindowTitle('Run Control Page')  
		self.show()

	def createHeadLine(self):
		self.HeadBox = QGroupBox()

		self.HeadLayout = QHBoxLayout()

		HeadLabel = QLabel('<font size="4"> Module: {0}  Calibration: {1} </font>'.format(self.info[0], self.info[1]))
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

		ControlLayout = QGridLayout()

		self.ConfigureButton = QPushButton("&Configure")
		self.ConfigureButton.setDefault(True)
		self.CustomizedButton = QPushButton("&Customize...")
		self.RunButton = QPushButton("&Run")
		self.RunButton.clicked.connect(self.runTest)
		self.AbortButton = QPushButton("&Abort")
		self.AbortButton.clicked.connect(self.abortTest)
		self.SaveButton = QPushButton("&Save")

		ControlLayout.addWidget(self.ConfigureButton,0,0,1,1)
		ControlLayout.addWidget(self.CustomizedButton,0,1,1,2)
		ControlLayout.addWidget(self.RunButton,1,0,1,1)
		ControlLayout.addWidget(self.AbortButton,1,1,1,1)
		ControlLayout.addWidget(self.SaveButton,1,2,1,1)

		ControllerBox.setLayout(ControlLayout)

		#Group Box for ternimal display
		TerminalBox = QGroupBox("&Terminal")
		TerminalSP = TerminalBox.sizePolicy()
		TerminalSP.setVerticalStretch(self.VerticalSegCol0[1])
		TerminalBox.setSizePolicy(TerminalSP)

		ConsoleLayout = QGridLayout()
		
		self.ConsoleView = QTextEdit()
		self.ConsoleView.setStyleSheet("QTextEdit { background-color: rgb(10, 10, 10); color : white; }")

		ConsoleLayout.addWidget(self.ConsoleView)
		TerminalBox.setLayout(ConsoleLayout)

			

		#Group Box for output display
		OutputBox = QGroupBox()
		OutputBoxSP = OutputBox.sizePolicy()
		OutputBoxSP.setVerticalStretch(self.VerticalSegCol1[0])
		OutputBox.setSizePolicy(OutputBoxSP)
		

		#Group Box for history
		HistoryBox = QGroupBox()
		HistoryBoxSP = HistoryBox.sizePolicy()
		HistoryBoxSP.setVerticalStretch(self.VerticalSegCol1[1])
		HistoryBox.setSizePolicy(HistoryBoxSP)
		

		LeftColSplitter.addWidget(ControllerBox)
		LeftColSplitter.addWidget(TerminalBox)
		RightColSplitter.addWidget(OutputBox)
		RightColSplitter.addWidget(HistoryBox)



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
		self.BackButton.clicked.connect(self.destroyMain)
		self.BackButton.clicked.connect(self.createMain)

		self.FinishButton = QPushButton("&Finish")
		self.FinishButton.setDefault(True)
		self.FinishButton.clicked.connect(self.closeWindow)

		self.StartLayout.addStretch(1)
		self.StartLayout.addWidget(self.ConnectButton)
		self.StartLayout.addWidget(self.BackButton)
		self.StartLayout.addWidget(self.FinishButton)
		self.AppOption.setLayout(self.StartLayout)

		self.mainLayout.addWidget(self.AppOption, sum(self.GroupBoxSeg[0:2]), 0, self.GroupBoxSeg[2], 1)

	def destroyApp(self):
		self.AppOption.deleteLater()
		self.mainLayout.removeWidget(self.AppOption)

	def closeWindow(self):
		self.close()

	def occupied(self):
		self.master.ProcessingTest = True

	def release(self):
		self.master.ProcessingTest = False
		self.master.NewTestButton.setDisabled(False)
		self.master.LogoutButton.setDisabled(False)
		self.master.ExitButton.setDisabled(False)

	def connectDB(self):
		pass

	def runTest(self):
		calibrationName = self.info[1]
		if isCompositeTest(calibrationName):
			self.runCompositeTest(calibrationName)
		elif isSingleTest(calibrationName):
			self.runSingleTest(calibrationName)
		else:
			QMessageBox.information(None, "Warning", "Not a valid calibration", QMessageBox.Ok)
			return

	def runCompositeTest(self,calibrationName):
		pass

	def runSingleTest(self,calibrationName):
		#self.run_process = Popen('python CMSITminiDAQ.py -f {0} -c {1}'.format("CMSIT.xml",self.calibration), shell=True, stdout=PIPE, stderr=PIPE)
		#self.run_process.setProgram()
		self.run_process.setProcessChannelMode(QtCore.QProcess.MergedChannels)
		self.run_process.start("ping",["www.google.com"])
		self.RunButton.setDisabled(True)

	def abortTest(self):
		reply = QMessageBox.question(None, "Abort", "Are you sure to abort?", QMessageBox.No | QMessageBox.Yes, QMessageBox.No)

		if reply == QMessageBox.Yes:
			self.run_process.kill()
		else:
			return

	@QtCore.pyqtSlot()
	def on_readyReadStandardOutput(self):
		text = self.run_process.readAllStandardOutput().data().decode()
		self.ConsoleView.append(text)

	@QtCore.pyqtSlot()
	def on_finish(self):
		self.RunButton.setDisabled(False)

	def closeEvent(self, event):
		if self.processingFlag == True:
			event.ignore()
		
		else:
			reply = QMessageBox.question(self, 'Window Close', 'Are you sure you want to quit the test?',
				QMessageBox.No | QMessageBox.Yes, QMessageBox.No)

			if reply == QMessageBox.Yes:
				event.accept()
				self.release()
			else:
				event.ignore()


		


		