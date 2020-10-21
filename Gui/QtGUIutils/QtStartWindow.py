from PyQt5.QtCore import *
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import sys
import os

from Gui.QtGUIutils.QtApplication import *
from Gui.GUIutils.DBConnection import *

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
		QApplication.setStyle(QStyleFactory.create('Fusion'))
		QApplication.setPalette(QApplication.style().standardPalette())
		#QApplication.setPalette(QApplication.palette())
		self.show()
	
	def createMain(self):
		self.MainOption = QGroupBox()

		kMinimumWidth = 120
		kMaximumWidth = 150
		kMinimumHeight = 30
		kMaximumHeight = 80

		ModuleIDLabel = QLabel("Module ID:")
		ModuleIDLabel.setMinimumWidth(kMinimumWidth)
		ModuleIDLabel.setMaximumWidth(kMaximumWidth)
		ModuleIDLabel.setMinimumHeight(kMinimumHeight)
		ModuleIDLabel.setMaximumHeight(kMaximumHeight)
		ModuleIDEdit = QLineEdit('')
		ModuleIDEdit.setEchoMode(QLineEdit.Normal)
		ModuleIDEdit.setPlaceholderText('Enter Module ID')

		layout = QGridLayout()
		layout.addWidget(ModuleIDLabel,0, 0, 1, 1)
		layout.addWidget(ModuleIDEdit, 0, 1, 1, 2)

		self.MainOption.setLayout(layout)

		self.AppOption = QGroupBox()
		self.StartLayout = QHBoxLayout()

		self.CancelButton = QPushButton("&Cancel")
		self.CancelButton.clicked.connect(self.release)
		self.CancelButton.clicked.connect(self.closeWindow)

		self.ResetButton = QPushButton("&Reset")
		self.ResetButton.clicked.connect(self.destroyMain)
		self.ResetButton.clicked.connect(self.createMain)

		self.NextButton = QPushButton("&Next")
		self.NextButton.clicked.connect(self.openRunWindow)
		self.NextButton.clicked.connect(self.closeWindow)

		self.StartLayout.addStretch(1)
		self.StartLayout.addWidget(self.CancelButton)
		self.StartLayout.addWidget(self.ResetButton)
		self.StartLayout.addWidget(self.NextButton)
		self.AppOption.setLayout(self.StartLayout)

		self.mainLayout.addWidget(self.MainOption)
		self.mainLayout.addWidget(self.AppOption)

	def destroyMain(self):
		self.MainOption.deleteLater()
		self.AppOption.deleteLater()
		self.mainLayout.removeWidget(self.MainOption)
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

	def openRunWindow(self):
		self.runFlag = True
		pass

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


		


		