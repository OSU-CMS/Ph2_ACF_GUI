from PyQt5.QtCore import *
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import sys
import os
import math

from Gui.QtGUIutils.QtRunWindow import *
from Gui.QtGUIutils.QtFwCheckDetails import *
from Gui.python.CustomizedWidget import *
from Gui.python.Firmware import *
from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.FirmwareUtil import *
from Gui.GUIutils.settings import *

class SummaryBox(QWidget):
	def __init__(self,module, index=0):
		super(SummaryBox,self).__init__()
		self.module = module
		self.index = index
		self.result = False
		self.verboseResult = {}
		self.chipSwitches = {}

		self.mainLayout = QGridLayout()
		self.initResult()
		self.createBody()
		self.measureFwPar()
		self.checkFwPar()
		self.setLayout(self.mainLayout)
	
	def initResult(self):
		for i in range(BoxSize[self.module.getType()]):
			self.verboseResult[i] = {}
			self.chipSwitches[i] = True

	def createBody(self):
		FEIDLabel = QLabel("ID: {}".format(self.module.getID()))
		FEIDLabel.setStyleSheet("font-weight:bold")
		PowerModeLabel = QLabel()
		PowerModeLabel.setText("FE Power Mode:")
		self.PowerModeCombo = QComboBox()
		self.PowerModeCombo.addItems(FEPowerUpVD.keys())
		self.PowerModeCombo.currentTextChanged.connect(self.checkFwPar)

		self.CheckLabel = QLabel()
		self.DetailsButton = QPushButton("Details")
		self.DetailsButton.clicked.connect(self.showDetails)

		self.mainLayout.addWidget(FEIDLabel,0,0,1,1)
		self.mainLayout.addWidget(PowerModeLabel,1,0,1,1)
		self.mainLayout.addWidget(self.PowerModeCombo,1,1,1,1)
		self.mainLayout.addWidget(self.CheckLabel,2,0,1,1)
		self.mainLayout.addWidget(self.DetailsButton,2,1,1,1)

	def measureFwPar(self):
		for index, (key, value) in enumerate(self.verboseResult.items()):
			value["Power-up Mode"] = self.PowerModeCombo.currentText() 
			# Fixme
			measureList = ["Analog Volts(V)","Digital Volts(V)","Analog I(A)","Digital I(A)"]
			for item in measureList:
				value[item] = 1.0
			self.verboseResult[key] = value

	def checkFwPar(self):
		self.result = True
		self.CheckLabel.setText("OK")
		self.CheckLabel.setStyleSheet("color:green")

	def getDetails(self):
		pass

	def showDetails(self):
		self.measureFwPar()
		self.occupied()
		self.DetailPage = QtFwCheckDetails(self)
		self.DetailPage.closedSignal.connect(self.release)

	def getResult(self):
		return self.result

	def occupied(self):
		self.DetailsButton.setDisabled(True)

	def release(self):
		self.DetailsButton.setDisabled(False)

class QtStartWindow(QWidget):
	def __init__(self,master, firmware):
		super(QtStartWindow,self).__init__()
		self.master = master
		#self.master.HVpowersupply.TurnOn()
		#self.master.LVpowersupply.TurnOn()
		self.firmware = firmware
		self.firmwareName = firmware.getBoardName()
		self.connection = self.master.connection
		self.mainLayout = QGridLayout()
		self.setLayout(self.mainLayout)
		self.runFlag = False

		self.setLoginUI()
		self.createHead()
		self.createMain()
		self.createApp()
		self.occupied()

	def setLoginUI(self):
		self.setGeometry(400, 400, 400, 400)  
		self.setWindowTitle('Start a new test')  
		#QApplication.setStyle(QStyleFactory.create('macintosh'))
		#QApplication.setPalette(QApplication.style().standardPalette())
		#QApplication.setPalette(QApplication.palette())
		self.show()

	def createHead(self):
		self.TestBox = QGroupBox()
		testlayout  = QGridLayout()
		TestLabel = QLabel("Test:")
		self.TestCombo = QComboBox()
		self.TestList = getAllTests(self.master.connection)
		self.TestCombo.addItems(self.TestList)
		TestLabel.setBuddy(self.TestCombo)

		testlayout.addWidget(TestLabel,0,0,1,1)
		testlayout.addWidget(self.TestCombo,0,1,1,1)
		self.TestBox.setLayout(testlayout)

		self.firmware.removeAllModule()
		self.BeBoardWidget = BeBoardBox(self.firmware)   #FLAG
		self.BeBoardWidget.changed.connect(self.destroyMain)
		self.BeBoardWidget.changed.connect(self.createMain)

		self.mainLayout.addWidget(self.TestBox,0,0)
		self.mainLayout.addWidget(self.BeBoardWidget,1,0)

	
	def createMain(self):
		self.firmwareCheckBox = QGroupBox()
		firmwarePar  = QGridLayout()
		BoxSize = {
			"SingleSCC" : 1,
			"DualSCC"	: 2,
			"QuadSCC"	: 4
		}
        ## To be finished
		self.ModuleList = []
		for  i, module  in enumerate(self.BeBoardWidget.ModuleList):
			ModuleSummaryBox = SummaryBox(module)
			self.ModuleList.append(ModuleSummaryBox)
			firmwarePar.addWidget(ModuleSummaryBox, math.floor(i/2), math.ceil( i%2 /2), 1, 1)

		self.firmwareCheckBox.setLayout(firmwarePar)

		self.mainLayout.addWidget(self.firmwareCheckBox,2,0)
		
	def destroyMain(self):
		self.firmwareCheckBox.deleteLater()
		self.mainLayout.removeWidget(self.firmwareCheckBox)

	def createApp(self):
		self.AppOption = QGroupBox()
		self.StartLayout = QHBoxLayout()

		self.CancelButton = QPushButton("&Cancel")
		self.CancelButton.clicked.connect(self.release)
		self.CancelButton.clicked.connect(self.closeWindow)

		self.ResetButton = QPushButton("&Reset")
		self.ResetButton.clicked.connect(self.destroyMain)
		self.ResetButton.clicked.connect(self.createMain)

		self.NextButton = QPushButton("&Next")
		self.NextButton.setDefault(True)
		#self.NextButton.setDisabled(True)
		self.NextButton.clicked.connect(self.openRunWindow)

		self.StartLayout.addStretch(1)
		self.StartLayout.addWidget(self.CancelButton)
		self.StartLayout.addWidget(self.ResetButton)
		self.StartLayout.addWidget(self.NextButton)
		self.AppOption.setLayout(self.StartLayout)

		self.mainLayout.addWidget(self.AppOption,3,0)

	def closeWindow(self):
		self.close()

	def occupied(self):
		self.master.ProcessingTest = True

	def release(self):
		self.master.ProcessingTest = False
		self.master.NewTestButton.setDisabled(False)
		self.master.LogoutButton.setDisabled(False)
		self.master.ExitButton.setDisabled(False)

	def setTestList(self):
		self.TestCombo.setDisabled(True)
		currentModule = self.ModuleIDEdit.text()
		localTests = getLocalTests(currentModule)
		if localTests == []:
			self.TestCombo.clear()
			self.TestCombo.addItems(firstTimeList) 
		else:
			self.TestCombo.clear()
			self.TestCombo.addItems(self.TestList)
		self.TestCombo.setDisabled(False)

	def checkFwPar(self):
		GlobalCheck = True
		for item in self.ModuleList:
			GlobalCheck = GlobalCheck and item.getResult()
		return GlobalCheck

	def setupBeBoard(self):
		# Setup the BeBoard
		pass


	def openRunWindow(self):
		for module in self.BeBoardWidget.getModules():
			if module.getID() == "":
				QMessageBox.information(None,"Error","No valid modlue ID!", QMessageBox.Ok)
				return

		if self.checkFwPar() == False:
			QMessageBox().information(None, "Error", "Front-End parameter check failed", QMessageBox.Ok)
			return

		self.firmwareDescription = self.BeBoardWidget.getFirmwareDescription()
		#self.info = [self.firmware.getModuleByIndex(0).getModuleID(), str(self.TestCombo.currentText())]
		self.info = [self.firmware.getModuleByIndex(0).getOpticalGroupID(), str(self.TestCombo.currentText())]
		self.runFlag = True
		self.master.RunNewTest = QtRunWindow(self.master, self.info, self.firmwareDescription)
		self.close()

	def closeEvent(self, event):
		if self.runFlag == True:
			event.accept()
		
		else:
			reply = QMessageBox.question(self, 'Window Close', 'Are you sure you want to quit the test?',
				QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

			if reply == QMessageBox.Yes:
				event.accept()
				self.release()
				#self.master.powersupply.TurnOff()
				print('Window closed')
			else:
				event.ignore()


		


		