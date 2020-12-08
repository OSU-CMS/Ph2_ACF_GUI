from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import sys
import os

class QtFwStatusWindow(QWidget):
	def __init__(self,master,firmwareName, verboseInfo, comment):
		super(QtFwStatusWindow,self).__init__()
		self.master = master
		self.firmwareName = firmwareName
		self.verboseInfo = verboseInfo
		self.comment = comment
		self.mainLayout = QGridLayout()
		self.setLayout(self.mainLayout)
		self.setLoginUI()
		self.createMain()
		self.occupied()
		#self.run_process.waitForFinished()

	def setLoginUI(self):
		self.setGeometry(400, 200, 400, 200)  
		self.setWindowTitle('Firmware Status Summary')  
		self.show()
	
	def createMain(self):
		self.MainOption = QGroupBox("Firmware: {0}".format(self.firmwareName))

		SummaryLayout = QGridLayout()

		if type(self.verboseInfo) == type({}):
			for index, key in enumerate(self.verboseInfo):
				KeyLabel = QLabel("{}".format(key))
				ValueLabel = QLabel("{}".format(self.verboseInfo[key]))
				if("Failed" in "{}".format(self.verboseInfo[key])):
					ValueLabel.setStyleSheet("color:red")
				if("Success" in "{}".format(self.verboseInfo[key])):
					ValueLabel.setStyleSheet("color:green")
				SummaryLayout.addWidget(KeyLabel,index,0,1,1)
				SummaryLayout.addWidget(ValueLabel,index,1,1,1)
		CommentLabel = QLabel("{}".format(self.comment))
		CommentLabel.setStyleSheet("font-weight: bold")
		SummaryLayout.addWidget(CommentLabel,len(self.verboseInfo),0,2,3)

		self.MainOption.setLayout(SummaryLayout)
		
		self.AppOption = QGroupBox()
		self.StartLayout = QHBoxLayout()

		self.CloseButton = QPushButton("&Close")
		self.CloseButton.clicked.connect(self.release)
		self.CloseButton.clicked.connect(self.closeWindow)

		self.StartLayout.addStretch(1)
		self.StartLayout.addWidget(self.CloseButton)
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
		pass

	def release(self):
		pass



		


		