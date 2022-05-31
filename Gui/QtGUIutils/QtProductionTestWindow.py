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

from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.guiUtils import *
from Gui.QtGUIutils.QtMatplotlibUtils import *
from Gui.QtGUIutils.QtLoginDialog import *
from Gui.python.ResultTreeWidget import *
from Gui.python.TestValidator import *
from Gui.python.IVCurveHandler import *
from Gui.python.SLDOScanHandler import *

class QtProductionTestWindow(QWidget):
	resized = pyqtSignal()
	close = pyqtSignal()
	def __init__(self,master,HV = None, LV= None):
		super(QtProductionTestWindow,self).__init__()
		self.master = master
		self.hvpowersupply = HV
		self.lvpowersupply = LV
		self.master.globalStop.connect(self.urgentStop)
		self.connection = self.master.connection

		# Handlers
		self.IVCurveHandler = None
		self.IVCurveResult = None

		self.SLDOScanHandler = None
		self.SLDOScanResult = None

		self.mainLayout = QGridLayout() # 10*10 segmentation
		self.setLayout(self.mainLayout)

		self.setUI()
		self.createHeadLine()
		self.createTestList()
		self.createMain()

	def setUI(self):
		X = self.master.dimension.width()/10
		Y = self.master.dimension.height()/10
		Width = self.master.dimension.width()*8./10
		Height = self.master.dimension.height()*8./10
		self.setGeometry(X, Y, Width, Height)  
		self.setWindowTitle('Production Tests Window') 
		self.show()

	def createHeadLine(self):
		self.HeadBox = QGroupBox()
		self.HeadLayout = QHBoxLayout()

		HeadLabel = QLabel('<font size="5"> Tests for production: </font>')
		HeadFont=QFont()
		HeadFont.setBold(True)
		HeadLabel.setMaximumHeight(30)
		HeadLabel.setFont(HeadFont)

		self.HeadLayout.addWidget(HeadLabel)
		self.HeadBox.setLayout(self.HeadLayout)
		self.mainLayout.addWidget(self.HeadBox, 0, 0, 1, 10) 

	def createTestList(self):
		self.TestsBox = QGroupBox()
		self.TestsBox.setMaximumWidth(300)
		self.TestsLayout = QVBoxLayout()

		# Add IVCurve
		self.IVCurveGroupBox = QGroupBox()
		self.IVCurveBox = QHBoxLayout()
		self.IVCurveButtom = QPushButton("&I-V Curve")
		self.IVCurveButtom.clicked.connect(self.startIVCurve)
		self.IVCurveAbortButtom = QPushButton("&Abort")
		self.IVCurveAbortButtom.clicked.connect(self.abortIVCurve)
		self.IVCurveAbortButtom.setDisabled(True)
		self.IVCurveBox.addWidget(self.IVCurveButtom)
		self.IVCurveBox.addWidget(self.IVCurveAbortButtom)
		self.IVCurveGroupBox.setLayout(self.IVCurveBox)
		self.TestsLayout.addWidget(self.IVCurveGroupBox)

		# Add SLDOScan
		self.SLDOScanGroupBox = QGroupBox()
		self.SLDOScanBox = QHBoxLayout()
		self.SLDOScanButtom = QPushButton("&SLDO scan")
		self.SLDOScanButtom.clicked.connect(self.startSLDOScan)
		self.SLDOScanAbortButtom = QPushButton("&Abort")
		self.SLDOScanAbortButtom.clicked.connect(self.abortSLDOScan)
		self.SLDOScanAbortButtom.setDisabled(True)
		self.SLDOScanBox.addWidget(self.SLDOScanButtom)
		self.SLDOScanBox.addWidget(self.SLDOScanAbortButtom)
		self.SLDOScanGroupBox.setLayout(self.SLDOScanBox)
		self.TestsLayout.addWidget(self.SLDOScanGroupBox)

		self.TestsLayout.addStretch(1)

		self.TestsBox.setLayout(self.TestsLayout)
		self.mainLayout.addWidget(self.TestsBox,1,0,9,3) 

	def createMain(self):
		self.MainBodyBox = QGroupBox()

		self.mainbodylayout = QHBoxLayout()

		self.MainTabs = QTabWidget()
		self.MainTabs.setTabsClosable(True)
		self.MainTabs.tabCloseRequested.connect(lambda index: self.closeTab(index))
		self.TabIndex = 0
		
		self.mainbodylayout.addWidget(self.MainTabs)

		self.MainBodyBox.setLayout(self.mainbodylayout)
		self.mainLayout.addWidget(self.MainBodyBox, 1, 3, 9, 7)

	def destroyMain(self):
		self.MainBodyBox.deleteLater()
		self.mainLayout.removeWidget(self.MainBodyBox)

	def closeWindow(self):
		self.close()

	def startIVCurve(self):
		self.IVCurveButtom.setDisabled(True)
		self.IVCurveAbortButtom.setDisabled(False)
		# Clean existing tab
		if self.IVCurveResult:
			index = self.MainTabs.indexOf(self.IVCurveResult)
			if index >= 0:
				self.MainTabs.removeTab(index)

		# Create tab and display result
		self.IVCurveData = []
		self.IVCurveResult = ScanCanvas(self, xlabel = "Voltage (V)", ylabel = "I (A)")
		self.MainTabs.addTab(self.IVCurveResult,"I-V Curve")
		self.IVCurveHandler = IVCurveHandler(self,self.hvpowersupply)
		self.IVCurveHandler.finished.connect(self.IVCurveFinished)
		self.IVCurveHandler.IVCurve()

	def abortIVCurve(self):
		if self.IVCurveHandler:
			self.IVCurveHandler.stop()
		self.IVCurveButtom.setDisabled(False)
		self.IVCurveAbortButtom.setDisabled(True)
	
	def IVCurveFinished(self):
		self.IVCurveButtom.setDisabled(False)
		self.IVCurveAbortButtom.setDisabled(True)

	def startSLDOScan(self):
		self.SLDOScanButtom.setDisabled(True)
		self.SLDOScanAbortButtom.setDisabled(False)
		# Clean existing tab
		if self.SLDOScanResult:
			index = self.MainTabs.indexOf(self.SLDOScanResult)
			if index >= 0:
				self.MainTabs.removeTab(index)

		# Create tab and display result
		self.SLDOScanData = []
		self.SLDOScanResult = ScanCanvas(self, xlabel = "Voltage (V)", ylabel = "I (A)", sorted = False)
		self.MainTabs.addTab(self.SLDOScanResult,"SLDO scan")
		self.SLDOScanHandler = SLDOScanHandler(self,self.lvpowersupply)
		self.SLDOScanHandler.finished.connect(self.SLDOScanFinished)
		self.SLDOScanHandler.SLDOScan()

	def abortSLDOScan(self):
		if self.SLDOScanHandler:
			self.SLDOScanHandler.stop()
		self.SLDOScanButtom.setDisabled(False)
		self.SLDOScanAbortButtom.setDisabled(True)
	
	def SLDOScanFinished(self):
		self.SLDOScanButtom.setDisabled(False)
		self.SLDOScanAbortButtom.setDisabled(True)

	def updateMeasurement(self, measureType, measure):
		print(measureType,measure)
		if measureType == "IVCurve":
			Voltage = measure["voltage"]
			Current = measure["current"]
			self.IVCurveData.append([Voltage,Current])
			self.IVCurveResult.updatePlots(self.IVCurveData)
			self.IVCurveResult.update()
			index = self.MainTabs.indexOf(self.IVCurveResult)
			self.MainTabs.clear()
			self.MainTabs.addTab(self.IVCurveResult,"I-V Curve")
			self.MainTabs.setCurrentWidget(self.IVCurveResult)

		if measureType == "SLDOScan":
			Voltage = measure["voltage"]
			Current = measure["current"]
			self.SLDOScanData.append([Voltage,Current])
			self.SLDOScanResult.updatePlots(self.SLDOScanData)
			self.SLDOScanResult.update()
			index = self.MainTabs.indexOf(self.SLDOScanResult)
			self.MainTabs.clear()
			self.MainTabs.addTab(self.SLDOScanResult,"SLDO scan")
			self.MainTabs.setCurrentWidget(self.SLDOScanResult)	

	def closeTab(self, index):
		self.MainTabs.removeTab(index)

	def urgentStop(self):
		if self.IVCurveHandler:
			self.IVCurveHandler.stop()
		
	def closeEvent(self, event):
		reply = QMessageBox.question(self, 'Window Close', 'Are you sure you want to close the window?',
				QMessageBox.No | QMessageBox.Yes, QMessageBox.No)

		if reply == QMessageBox.Yes:
			self.close.emit()
			event.accept()
		else:
			event.ignore()


		


		
