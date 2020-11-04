from PyQt5.QtCore import *
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import sys
import os

from Gui.QtGUIutils.QtRunWindow import *
from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.FirmwareUtil import *

class QtStartWindow(QWidget):
	def __init__(self,master):
		super(QtStartWindow,self).__init__()
		self.master = master
		self.connection = self.master.connection
		self.mainLayout = QGridLayout()
		self.setLayout(self.mainLayout)
		self.runFlag = False

		self.setLoginUI()
		self.createMain()
		self.occupied()

	def setLoginUI(self):
		self.setGeometry(400, 400, 400, 400)  
		self.setWindowTitle('Start a new test')  
		#QApplication.setStyle(QStyleFactory.create('macintosh'))
		#QApplication.setPalette(QApplication.style().standardPalette())
		#QApplication.setPalette(QApplication.palette())
		self.show()
	
	def createMain(self):
		self.MainOption = QGroupBox()

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

		TestLabel = QLabel("Test:")
		TestLabel.setMinimumWidth(kMinimumWidth)
		TestLabel.setMaximumWidth(kMaximumWidth)
		TestLabel.setMinimumHeight(kMinimumHeight)
		TestLabel.setMaximumHeight(kMaximumHeight)

		self.TestCombo = QComboBox()
		self.TestCombo.addItems(getAllTests(self.master.connection))
		TestLabel.setBuddy(self.TestCombo)

		layout = QGridLayout()
		layout.addWidget(ModuleIDLabel,0, 0, 1, 1, Qt.AlignRight)
		layout.addWidget(self.ModuleIDEdit, 0, 1, 1, 2, Qt.AlignLeft)
		layout.addWidget(TestLabel,1, 0, 1, 1, Qt.AlignRight)
		layout.addWidget(self.TestCombo,1, 1, 1, 2, Qt.AlignLeft)

		self.MainOption.setLayout(layout)

		self.firmwareCheckBox = QGroupBox()

		firmwarePar  = QGridLayout()

		PowerModeLabel = QLabel()
		PowerModeLabel.setText("FE Power Mode:")
		self.PowerModeCombo = QComboBox()
		self.PowerModeCombo.addItems(FEPowerUpVD.keys())

		ANLVoltLabel = QLabel("Analog V:")
		self.ANLVoltEdit = QLineEdit()
		self.ANLVoltEdit.setEchoMode(QLineEdit.Normal)
		DIGVoltLabel = QLabel("Digital V:")
		self.DIGVoltEdit = QLineEdit()
		self.DIGVoltEdit.setEchoMode(QLineEdit.Normal)
		AmpLabel = QLabel("Measured I:")
		self.AmpEdit = QLineEdit()
		self.AmpEdit.setEchoMode(QLineEdit.Normal)

		self.CheckLabel = QLabel()
		self.CheckButton = QPushButton("&Check")
		self.CheckButton.clicked.connect(self.checkFwPar)
		

		firmwarePar.addWidget(PowerModeLabel,0,0,1,1)
		firmwarePar.addWidget(self.PowerModeCombo,0,1,1,1)
		firmwarePar.addWidget(ANLVoltLabel,1,0,1,1)
		firmwarePar.addWidget(self.ANLVoltEdit,1,1,1,1)
		firmwarePar.addWidget(DIGVoltLabel,2,0,1,1)
		firmwarePar.addWidget(self.DIGVoltEdit,2,1,1,1)
		firmwarePar.addWidget(AmpLabel,3,0,1,1)
		firmwarePar.addWidget(self.AmpEdit,3,1,1,1)
		firmwarePar.addWidget(self.CheckLabel,4,0,1,3)
		firmwarePar.addWidget(self.CheckButton,5,0,1,1)	

		self.firmwareCheckBox.setLayout(firmwarePar)

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
		self.NextButton.setDisabled(True)
		self.NextButton.clicked.connect(self.openRunWindow)

		self.StartLayout.addStretch(1)
		self.StartLayout.addWidget(self.CancelButton)
		self.StartLayout.addWidget(self.ResetButton)
		self.StartLayout.addWidget(self.NextButton)
		self.AppOption.setLayout(self.StartLayout)

		self.mainLayout.addWidget(self.MainOption)
		self.mainLayout.addWidget(self.firmwareCheckBox)
		self.mainLayout.addWidget(self.AppOption)

	def destroyMain(self):
		self.MainOption.deleteLater()
		self.firmwareCheckBox.deleteLater()
		self.AppOption.deleteLater()
		self.mainLayout.removeWidget(self.MainOption)
		self.mainLayout.removeWidget(self.firmwareCheckBox)
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

	def checkFwPar(self):
		self.CheckLabel.setStyleSheet("color:red")
		PowerMode = str(self.PowerModeCombo.currentText())
		if not str(self.ANLVoltEdit.text()) or not str(self.DIGVoltEdit.text()) or not str(self.AmpEdit.text()):
			self.CheckLabel.setText("V/I measure is missing")
			return

		comment = ''
		try:		
			if float(str(self.ANLVoltEdit.text())) < FEPowerUpVA[str(self.PowerModeCombo.currentText())][0] or float(str(self.ANLVoltEdit.text())) > FEPowerUpVA[str(self.PowerModeCombo.currentText())][1]:
				comment +=  "Analog Voltage range: {}".format(FEPowerUpVA[str(self.PowerModeCombo.currentText())])
			if float(str(self.DIGVoltEdit.text())) < FEPowerUpVD[str(self.PowerModeCombo.currentText())][0] or float(str(self.DIGVoltEdit.text())) > FEPowerUpVD[str(self.PowerModeCombo.currentText())][1]:
				comment +=  "Digital Voltage range: {}".format(FEPowerUpVA[str(self.PowerModeCombo.currentText())])
			if float(str(self.AmpEdit.text()))  < FEPowerUpAmp[str(self.PowerModeCombo.currentText())][0] or float(str(self.AmpEdit.text())) > FEPowerUpAmp[str(self.PowerModeCombo.currentText())][1]:
				comment +=  "Amp range: {}".format(FEPowerUpAmp[str(self.PowerModeCombo.currentText())])		
		except ValueError:
			comment = "Not valid input"

		if comment == '':
			comment = "Ok"
			self.CheckLabel.setText(comment)
			self.CheckLabel.setStyleSheet("color:green")
			self.NextButton.setDisabled(False)
			return

		self.CheckLabel.setText(comment)
		self.NextButton.setDisabled(True)


	def openRunWindow(self):
		if self.ModuleIDEdit.text() == "":
			QMessageBox.information(None,"Error","No valid modlue ID!", QMessageBox.Ok)
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


		


		