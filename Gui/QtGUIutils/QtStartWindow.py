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
from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.FirmwareUtil import *

class StatusBox(QWidget):
	def __init__(self,firmwareName, index=0):
		super(StatusBox,self).__init__()
		self.firmwareName = firmwareName
		self.index = index #For parameter fetching
		self.mainLayout = QGridLayout()
		self.createBody()
		self.measureFwPar()
		self.checkFwPar()
		self.setLayout(self.mainLayout)

	
	def createBody(self):
		FEIDLabel = QLabel("ID: {}".format(self.index))
		FEIDLabel.setStyleSheet("font-weight:bold")
		PowerModeLabel = QLabel()
		PowerModeLabel.setText("FE Power Mode:")
		self.PowerModeCombo = QComboBox()
		self.PowerModeCombo.addItems(FEPowerUpVD.keys())
		self.PowerModeCombo.currentTextChanged.connect(self.checkFwPar)

		ANLVoltLabel = QLabel("Analog Volts(V):")
		self.ANLVoltEdit = QLineEdit()
		self.ANLVoltEdit.setEchoMode(QLineEdit.Normal)
		self.ANLVoltEdit.textChanged.connect(self.checkFwPar)
		DIGVoltLabel = QLabel("Digital Volts(V):")
		self.DIGVoltEdit = QLineEdit()
		self.DIGVoltEdit.setEchoMode(QLineEdit.Normal)
		self.DIGVoltEdit.textChanged.connect(self.checkFwPar)
		ANLAmpLabel = QLabel("Analog I(A):")
		self.ANLAmpEdit = QLineEdit()
		self.ANLAmpEdit.setEchoMode(QLineEdit.Normal)
		self.ANLAmpEdit.textChanged.connect(self.checkFwPar)
		DIGAmpLabel = QLabel("Digital I(A):")
		self.DIGAmpEdit = QLineEdit()
		self.DIGAmpEdit.setEchoMode(QLineEdit.Normal)
		self.DIGAmpEdit.textChanged.connect(self.checkFwPar)
		#AmpLabel = QLabel("Sourcemeter I(A):")
		#self.AmpEdit = QLineEdit()
		#self.AmpEdit.setEchoMode(QLineEdit.Normal)

		self.CheckLabel = QLabel()

		self.mainLayout.addWidget(FEIDLabel,0,0,1,1)
		self.mainLayout.addWidget(PowerModeLabel,1,0,1,1)
		self.mainLayout.addWidget(self.PowerModeCombo,1,1,1,1)
		self.mainLayout.addWidget(ANLVoltLabel,2,0,1,1)
		self.mainLayout.addWidget(self.ANLVoltEdit,2,1,1,1)
		self.mainLayout.addWidget(DIGVoltLabel,3,0,1,1)
		self.mainLayout.addWidget(self.DIGVoltEdit,3,1,1,1)
		#self.mainLayout.addWidget(AmpLabel,4,0,1,1)
		#self.mainLayout.addWidget(self.AmpEdit,4,1,1,1)
		self.mainLayout.addWidget(ANLAmpLabel,4,0,1,1)
		self.mainLayout.addWidget(self.ANLAmpEdit,4,1,1,1)
		self.mainLayout.addWidget(DIGAmpLabel,5,0,1,1)
		self.mainLayout.addWidget(self.DIGAmpEdit,5,1,1,1)
		self.mainLayout.addWidget(self.CheckLabel,6,0,1,3)

	def measureFwPar(self):
		ANL_Voltage = 1.8
		DIG_Voltage = 1.8
		ANL_Amp  = 0.6
		DIG_Amp  = 0.6
		self.ANLVoltEdit.setText("{}".format(ANL_Voltage))
		self.DIGVoltEdit.setText("{}".format(DIG_Voltage))
		self.ANLAmpEdit.setText("{}".format(ANL_Amp))
		self.DIGAmpEdit.setText("{}".format(DIG_Amp))

	def checkFwPar(self):
		self.CheckLabel.setStyleSheet("color:red")
		PowerMode = str(self.PowerModeCombo.currentText())
		if not str(self.ANLVoltEdit.text()) or not str(self.DIGVoltEdit.text()) or not str(self.ANLAmpEdit.text()) or not str(self.DIGAmpEdit.text()):
			self.CheckLabel.setText("V/I measure is missing")
			return False

		comment = ''
		try:		
			if float(str(self.ANLVoltEdit.text())) < FEPowerUpVA[PowerMode][0] or float(str(self.ANLVoltEdit.text())) > FEPowerUpVA[PowerMode][1]:
				comment +=  "Analog Voltage range: {}".format(FEPowerUpVA[PowerMode])
			if float(str(self.DIGVoltEdit.text())) < FEPowerUpVD[PowerMode][0] or float(str(self.DIGVoltEdit.text())) > FEPowerUpVD[PowerMode][1]:
				comment +=  "Digital Voltage range: {}".format(FEPowerUpVA[PowerMode])
			#if math.fabs( float(str(self.AmpEdit.text())) - float(str(self.ANLAmpEdit.text())) - float(str(self.DIGAmpEdit.text())) ) > 0.1:
			#	comment += "Current from Source deviated from measured current in module"
			ChipAmp = float(str(self.ANLAmpEdit.text())) + float(str(self.DIGAmpEdit.text()))
			if ChipAmp  < FEPowerUpAmp[PowerMode][0] or ChipAmp > FEPowerUpAmp[PowerMode][1]:
				comment +=  "Amp range: {}".format(FEPowerUpAmp[PowerMode])		
		except ValueError:
			comment = "Not valid input"

		if comment == '':
			comment = "Ok"
			self.CheckLabel.setText(comment)
			self.CheckLabel.setStyleSheet("color:green")
			return True
		else:
			self.CheckLabel.setText(comment)
			return False



class QtStartWindow(QWidget):
	def __init__(self,master, firmwareName):
		super(QtStartWindow,self).__init__()
		self.master = master
		self.firmwareName = firmwareName
		self.FEStatusDict = {}
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
		self.HeadBox = QGroupBox()

		kMinimumWidth = 80
		kMaximumWidth = 150
		kMinimumHeight = 30
		kMaximumHeight = 80

		ModuleIDLabel = QLabel("Module ID:")
		ModuleIDLabel.setMinimumWidth(kMinimumWidth)
		ModuleIDLabel.setMaximumWidth(kMaximumWidth)
		ModuleIDLabel.setMinimumHeight(kMinimumHeight)
		ModuleIDLabel.setMaximumHeight(kMaximumHeight)
		self.ModuleIDEdit = QLineEdit('')
		self.ModuleIDEdit.setEchoMode(QLineEdit.Normal)
		self.ModuleIDEdit.setPlaceholderText('Enter Module ID')
		self.ModuleIDEdit.textChanged.connect(self.setTestList)

		TypeLabel = QLabel("Type:")
		TypeLabel.setMinimumWidth(kMinimumWidth)
		TypeLabel.setMaximumWidth(kMaximumWidth)
		TypeLabel.setMinimumHeight(kMinimumHeight)
		TypeLabel.setMaximumHeight(kMaximumHeight)

		self.TypeCombo = QComboBox()
		self.TypeList = ["SingleSCC", "DualSCC", "QuadSCC"]
		self.TypeCombo.addItems(self.TypeList)
		TypeLabel.setBuddy(self.TypeCombo)
		self.TypeCombo.currentIndexChanged.connect(self.destroyMain)
		self.TypeCombo.currentIndexChanged.connect(lambda state, x="{0}".format(self.TypeCombo.currentText()) : self.createMain())

		TestLabel = QLabel("Test:")
		TestLabel.setMinimumWidth(kMinimumWidth)
		TestLabel.setMaximumWidth(kMaximumWidth)
		TestLabel.setMinimumHeight(kMinimumHeight)
		TestLabel.setMaximumHeight(kMaximumHeight)

		self.TestCombo = QComboBox()
		self.TestList = getAllTests(self.master.connection)
		self.TestCombo.addItems(self.TestList)
		TestLabel.setBuddy(self.TestCombo)

		layout = QGridLayout()
		layout.addWidget(ModuleIDLabel,0, 0, 1, 1, Qt.AlignRight)
		layout.addWidget(self.ModuleIDEdit, 0, 1, 1, 2, Qt.AlignLeft)
		layout.addWidget(TypeLabel,1, 0, 1, 1, Qt.AlignRight)
		layout.addWidget(self.TypeCombo,1, 1, 1, 2, Qt.AlignLeft)
		layout.addWidget(TestLabel,2, 0, 1, 1, Qt.AlignRight)
		layout.addWidget(self.TestCombo,2, 1, 1, 2, Qt.AlignLeft)

		self.HeadBox.setLayout(layout)
		self.mainLayout.addWidget(self.HeadBox,0,0)
	
	def createMain(self):

		self.firmwareCheckBox = QGroupBox()

		firmwarePar  = QGridLayout()

		#PowerModeLabel = QLabel()
		#PowerModeLabel.setText("FE Power Mode:")
		#self.PowerModeCombo = QComboBox()
		#self.PowerModeCombo.addItems(FEPowerUpVD.keys())

		#ANLVoltLabel = QLabel("Analog Volts(V):")
		#self.ANLVoltEdit = QLineEdit()
		#self.ANLVoltEdit.setEchoMode(QLineEdit.Normal)
		#DIGVoltLabel = QLabel("Digital Volts(V):")
		#self.DIGVoltEdit = QLineEdit()
		#self.DIGVoltEdit.setEchoMode(QLineEdit.Normal)
		#AmpLabel = QLabel("Sourcemeter I(A):")
		#self.AmpEdit = QLineEdit()
		#self.AmpEdit.setEchoMode(QLineEdit.Normal)

		#self.CheckLabel = QLabel()
		#self.CheckButton = QPushButton("&Check")
		#self.CheckButton.clicked.connect(self.checkFwPar)

		#firmwarePar.addWidget(PowerModeLabel,0,0,1,1)
		#firmwarePar.addWidget(self.PowerModeCombo,0,1,1,1)
		#firmwarePar.addWidget(ANLVoltLabel,1,0,1,1)
		#firmwarePar.addWidget(self.ANLVoltEdit,1,1,1,1)
		#firmwarePar.addWidget(DIGVoltLabel,2,0,1,1)
		#firmwarePar.addWidget(self.DIGVoltEdit,2,1,1,1)
		#firmwarePar.addWidget(AmpLabel,3,0,1,1)
		#firmwarePar.addWidget(self.AmpEdit,3,1,1,1)
		#firmwarePar.addWidget(self.CheckLabel,4,0,1,3)
		#firmwarePar.addWidget(self.CheckButton,5,0,1,1)	

		#self.firmwareCheckBox.setLayout(firmwarePar)

		BoxSize = {
			"SingleSCC" : 1,
			"DualSCC"	: 2,
			"QuadSCC"	: 4
		}

		self.FEStatusDict = {}
		for  i in range(int(BoxSize["{}".format(self.TypeCombo.currentText())])):
			ChipStatusBox = StatusBox(self.firmwareName, i)
			self.FEStatusDict[i] = ChipStatusBox
			firmwarePar.addWidget(ChipStatusBox, math.floor(i/2), math.ceil( i%2 /2), 1, 1)

		self.firmwareCheckBox.setLayout(firmwarePar)
		self.mainLayout.addWidget(self.firmwareCheckBox,1,0)

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

		self.mainLayout.addWidget(self.AppOption,2,0)

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
		for key, item in self.FEStatusDict.items():
			GlobalCheck = GlobalCheck and item.checkFwPar()
		return GlobalCheck
		''' self.CheckLabel.setStyleSheet("color:red")
		PowerMode = str(self.PowerModeCombo.currentText())
		if not str(self.ANLVoltEdit.text()) or not str(self.DIGVoltEdit.text()) or not str(self.AmpEdit.text()):
			self.CheckLabel.setText("V/I measure is missing")
			return

		comment = ''
		try:		
			if float(str(self.ANLVoltEdit.text())) < FEPowerUpVA[PowerMode][0] or float(str(self.ANLVoltEdit.text())) > FEPowerUpVA[PowerMode][1]:
				comment +=  "Analog Voltage range: {}".format(FEPowerUpVA[PowerMode])
			if float(str(self.DIGVoltEdit.text())) < FEPowerUpVD[PowerMode][0] or float(str(self.DIGVoltEdit.text())) > FEPowerUpVD[PowerMode][1]:
				comment +=  "Digital Voltage range: {}".format(FEPowerUpVA[PowerMode])
			if float(str(self.AmpEdit.text()))  < FEPowerUpAmp[PowerMode][0] or float(str(self.AmpEdit.text())) > FEPowerUpAmp[PowerMode][1]:
				comment +=  "Amp range: {}".format(FEPowerUpAmp[PowerMode])		
		except ValueError:
			comment = "Not valid input"

		if comment == '':
			comment = "Ok"
			self.CheckLabel.setText(comment)
			self.CheckLabel.setStyleSheet("color:green")
			self.NextButton.setDisabled(False)
			return

		self.CheckLabel.setText(comment)
		self.NextButton.setDisabled(True) '''


	def openRunWindow(self):
		if self.ModuleIDEdit.text() == "":
			QMessageBox.information(None,"Error","No valid modlue ID!", QMessageBox.Ok)
			return

		if self.checkFwPar() == False:
			QMessageBox().information(None, "Error", "Front-End parameter check failed", QMessageBox.Ok)
			return
		self.info = [self.ModuleIDEdit.text(), str(self.TestCombo.currentText())]
		self.runFlag = True
		self.master.RunNewTest = QtRunWindow(self.master,self.info)
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
				print('Window closed')
			else:
				event.ignore()


		


		