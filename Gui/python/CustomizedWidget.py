from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QFormLayout, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import sys
import os
import math
import copy

from Gui.python.Firmware import *
from Gui.GUIutils.settings import *
from Gui.GUIutils.FirmwareUtil import *
from Gui.QtGUIutils.QtFwCheckDetails import *

## for configuration

class ModuleBox(QWidget):
	typechanged = pyqtSignal()
	destroy = pyqtSignal()

	def __init__(self):
		super(ModuleBox,self).__init__()
		self.mainLayout = QGridLayout()
		self.createRow()
		self.setLayout(self.mainLayout)

	def createRow(self):

		IDLabel = QLabel("Module  ID:")
		self.IDEdit = QLineEdit()
		self.IDEdit.setMinimumWidth(120)
		self.IDEdit.setMaximumWidth(200)
		self.IDEdit.textChanged.connect(self.on_TypeChanged)

		TypeLabel = QLabel("Type:")
		self.TypeCombo = QComboBox()
		self.TypeCombo.addItems(ModuleType.values())
		self.TypeCombo.currentIndexChanged.connect(self.on_TypeChanged)
		TypeLabel.setBuddy(self.TypeCombo)

		self.mainLayout.addWidget(IDLabel,0,0,1,1)
		self.mainLayout.addWidget(self.IDEdit,0,1,1,1)
		self.mainLayout.addWidget(TypeLabel,0,2,1,1)
		self.mainLayout.addWidget(self.TypeCombo,0,3,1,1)

	def getID(self):
		return self.IDEdit.text()

	def getType(self):
		return self.TypeCombo.currentText()

	@QtCore.pyqtSlot()
	def on_TypeChanged(self):
		self.typechanged.emit()
		
class BeBoardBox(QWidget):
	changed = pyqtSignal()

	def __init__(self,firmware):
		super(BeBoardBox,self).__init__()
		self.firmware = firmware
		self.ModuleList = []
		self.mainLayout = QGridLayout()

		self.initList()
		self.createList()

		self.setLayout(self.mainLayout)

	def initList(self):
		ModuleRow = ModuleBox()
		self.ModuleList.append(ModuleRow)

	def createList(self):
		self.ListBox = QGroupBox()

		self.ListLayout = QGridLayout()
		self.ListLayout.setVerticalSpacing(0)
		
		self.updateList()

		self.ListBox.setLayout(self.ListLayout)
		self.mainLayout.addWidget(self.ListBox,0,0)

	def deleteList(self):
		self.ListBox.deleteLater()
		self.mainLayout.removeWidget(self.ListBox)

	def updateList(self):
		[columns, rows] = [  self.ListLayout.columnCount(),  self.ListLayout.rowCount()]
		
		for i in range(columns):
			for  j in range(rows):
				item  =  self.ListLayout.itemAtPosition(j,i)
				if item:
					widget =  item.widget()
					self.ListLayout.removeWidget(widget)

		for index, module in enumerate(self.ModuleList):
			module.setMaximumWidth(500)
			module.setMaximumHeight(50)
			module.typechanged.connect(self.on_TypeChanged)
			self.ListLayout.addWidget(module,index,0,1,1)
			if index > 0:
				RemoveButton = QPushButton("remove")
				RemoveButton.setMaximumWidth(150)
				RemoveButton.clicked.connect(lambda x = index: self.removeModule(x))
				self.ListLayout.addWidget(RemoveButton,index,1,1,1)
		#ModuleLayout = QFormLayout()
		#ModuleItem = ModuleBox()
		
		#ModuleItem.destroy.connect(partial(self.removeModule,ModuleItem))
		#ModuleLayout.addRow(ModuleBox())
		
		NewButton = QPushButton("add")
		NewButton.setMaximumWidth(150)
		NewButton.clicked.connect(self.addModule)
		self.ListLayout.addWidget(NewButton,len(self.ModuleList),1,1,1)
		self.update()

	def removeModule(self, index):
		self.ModuleList.pop(index)
		if str(sys.version).startswith("3.8"):
			self.deleteList()
			self.createList()
		elif str(sys.version).startswith(("3.7","3.9")):
			self.updateList()
		else:
			self.updateList()
		self.changed.emit()
	
	def addModule(self):
		self.ModuleList.append(ModuleBox())
		if str(sys.version).startswith("3.8"):
			self.deleteList()
			self.createList()
		elif str(sys.version).startswith(("3.7","3.9")):
			self.updateList()
		else:
			self.updateList()
		self.changed.emit()

	def getModules(self):
		return self.ModuleList

	def getFirmwareDescription(self, **kwargs):
		for index, module in enumerate(self.ModuleList):
			FwModule = QtModule()
			#FwModule.setModuleID(module.getID())
			FwModule.setOpticalGroupID(module.getID())
			FwModule.setModuleType(module.getType())
			self.firmware.addModule(index,FwModule)
		return self.firmware

	@QtCore.pyqtSlot()
	def on_TypeChanged(self):
		self.changed.emit()

class StatusBox(QWidget):
	def __init__(self,verbose,index = 0):
		super(StatusBox,self).__init__()
		self.index = index
		self.verbose = verbose
		self.mainLayout = QGridLayout()
		self.createBody()
		self.checkFwPar()
		self.setLayout(self.mainLayout)

	
	def createBody(self):
		FEIDLabel = QLabel("ID: {}".format(self.index))
		FEIDLabel.setStyleSheet("font-weight:bold")

		self.LabelList = []
		self.EditList = []

		for  i, (key, value) in enumerate(self.verbose.items()):
			Label = QLabel()
			Label.setText(str(key) + ":")
			Edit = QLineEdit()
			Edit.setText(str(value))
			Edit.setDisabled(True)
			self.LabelList.append(Label)
			self.EditList.append(Edit)

		self.CheckLabel = QLabel()

		self.mainLayout.addWidget(FEIDLabel,0,0,1,1)

		for index in range(len(self.LabelList)):
			self.mainLayout.addWidget(self.LabelList[index],index+1,0,1,1)
			self.mainLayout.addWidget(self.EditList[index],index+1,1,1,1)

	def checkFwPar(self):
		return True
		'''
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
		'''