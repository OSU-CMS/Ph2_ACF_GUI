from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtGui import QPixmap, QFont
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QFileDialog,  QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit, QMainWindow, QMessageBox,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QSplitter, QStyleFactory, QTableWidget, QTabWidget, QTextEdit,
		QVBoxLayout, QWidget)

import sys
import os
import subprocess
from subprocess import Popen, PIPE

from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.guiUtils import *

class QtCustomizeWindow(QWidget):
	def __init__(self,master):
		super(QtCustomizeWindow,self).__init__()
		self.master = master
		self.connection = self.master.connection
		self.GroupBoxSeg = [1, 3, 1]

		#Fixme: QTimer to be added to update the page automatically

		self.mainLayout = QGridLayout()
		self.setLayout(self.mainLayout)

		self.setLoginUI()
		self.createHeadLine()
		self.createMain()
		self.createApp()
		self.occupied()

	def setLoginUI(self):
		self.setGeometry(200, 200, 500, 300)  
		self.setWindowTitle('Customize Test')  
		self.show()

	def createHeadLine(self):
		self.HeadBox = QGroupBox()

		self.HeadLayout = QHBoxLayout()

		HeadLabel = QLabel('<font size="5"> Set up configuration file:  </font>')
		HeadFont=QFont()
		HeadFont.setBold(True)
		HeadLabel.setMaximumHeight(30)
		HeadLabel.setFont(HeadFont)

		self.HeadLayout.addWidget(HeadLabel)

		self.HeadBox.setLayout(self.HeadLayout)

		self.mainLayout.addWidget(self.HeadBox, sum(self.GroupBoxSeg[:0]), 0, self.GroupBoxSeg[0], 1)

	def destroyHeadLine(self):
		self.HeadBox.deleteLater()
		self.mainLayout.removeWidget(self.HeadBox)
	
	def createMain(self):
		self.MainBodyBox = QGroupBox()

		mainbodylayout = QGridLayout()

		kMinimumWidth = 120
		kMaximumWidth = 150
		kMinimumHeight = 30
		kMaximumHeight = 80

		XMLLabel = QLabel("CMSIT XML:")
		self.XMLEdit = QLineEdit('')
		self.XMLEdit.setEchoMode(QLineEdit.Normal)
		self.XMLEdit.setPlaceholderText('{0}'.format(self.master.config_file))
		self.XMLEdit.setMinimumWidth(kMinimumWidth)
		self.XMLEdit.setMaximumHeight(kMaximumHeight)
		self.XMLButton = QPushButton("&Browse..")
		self.XMLButton.clicked.connect(self.openFileBrowserXML)


		RD53Label = QLabel("RD53:")
		self.RD53Edit = QLineEdit('')
		self.RD53Edit.setEchoMode(QLineEdit.Normal)
		self.RD53Edit.setPlaceholderText('{0}'.format(self.master.rd53_file))
		self.RD53Edit.setMinimumWidth(kMinimumWidth)
		self.RD53Edit.setMaximumHeight(kMaximumHeight)
		self.RD53Button = QPushButton("&Browse..")
		self.RD53Button.clicked.connect(self.openFileBrowserText)


		mainbodylayout.addWidget(XMLLabel,   0,0,1,1, Qt.AlignRight)
		mainbodylayout.addWidget(self.XMLEdit,    0,1,1,3)
		mainbodylayout.addWidget(self.XMLButton,  0,4,1,1)
		mainbodylayout.addWidget(RD53Label,  1,0,1,1, Qt.AlignRight)
		mainbodylayout.addWidget(self.RD53Edit,   1,1,1,3)
		mainbodylayout.addWidget(self.RD53Button, 1,4,1,1)

		self.MainBodyBox.setLayout(mainbodylayout)
		self.mainLayout.addWidget(self.MainBodyBox, sum(self.GroupBoxSeg[0:1]), 0, self.GroupBoxSeg[1], 1)

	def destroyMain(self):
		self.MainBodyBox.deleteLater()
		self.mainLayout.removeWidget(self.MainBodyBox)

	def createApp(self):
		self.AppOption = QGroupBox()
		self.StartLayout = QHBoxLayout()

		self.ResetButton = QPushButton("&Reset")
		self.ResetButton.clicked.connect(self.destroyMain)
		self.ResetButton.clicked.connect(self.createMain)

		self.CancelButton = QPushButton("&Cancel")
		self.CancelButton.clicked.connect(self.release)
		self.CancelButton.clicked.connect(self.closeWindow)

		self.ConfirmButton = QPushButton("&Confirm")
		self.ConfirmButton.setDefault(True)
		self.ConfirmButton.clicked.connect(self.confirmFiles)
		self.ConfirmButton.clicked.connect(self.release)
		self.ConfirmButton.clicked.connect(self.closeWindow)

		self.StartLayout.addStretch(1)
		self.StartLayout.addWidget(self.ResetButton)
		self.StartLayout.addWidget(self.CancelButton)
		self.StartLayout.addWidget(self.ConfirmButton)
		self.AppOption.setLayout(self.StartLayout)

		self.mainLayout.addWidget(self.AppOption, sum(self.GroupBoxSeg[0:2]), 0, self.GroupBoxSeg[2], 1)

	def destroyApp(self):
		self.AppOption.deleteLater()
		self.mainLayout.removeWidget(self.AppOption)

	def closeWindow(self):
		self.close()

	def openFileBrowserText(self):
		options = QFileDialog.Options()
		options |= QFileDialog.DontUseNativeDialog
		fileName, _ = QFileDialog.getOpenFileName(self,"RD53 configuration", "","All Files (*);;Text Files (*.txt)", options=options)
		if fileName.split('.')[-1] != "txt":
			QMessageBox.information(None,"Warning","Not a valid txt file", QMessageBox.Ok)
		self.RD53Edit.setText(fileName)

	def openFileBrowserXML(self):
		options = QFileDialog.Options()
		options |= QFileDialog.DontUseNativeDialog
		fileName, _ = QFileDialog.getOpenFileName(self,"CMSIT configuration", "","All Files (*);;XML Files (*.xml)", options=options)
		if fileName.split('.')[-1] != "xml":
			QMessageBox.information(None,"Warning","Not a valid XML file", QMessageBox.Ok)
		self.XMLEdit.setText(fileName)

	def confirmFiles(self):
		self.master.config_file = self.XMLEdit.text()
		self.master.rd53_file = self.RD53Edit.text()

	def occupied(self):
		self.master.RunButton.setDisabled(True)
		self.master.ResetButton.setDisabled(True)
		self.master.CustomizedButton.setDisabled(True)

	def release(self):
		self.master.RunButton.setDisabled(False)
		self.master.ResetButton.setDisabled(False)
		self.master.CustomizedButton.setDisabled(False)

		
