
import logging

# Customize the logging configuration
logging.basicConfig(
   level=logging.INFO,
   format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
   filename='my_project.log',  # Specify a log file
   filemode='w'  # 'w' for write, 'a' for append
)

logger = logging.getLogger(__name__)

from PyQt5.QtCore import *
from PyQt5.QtGui import QFont, QPixmap 
from PyQt5.QtWidgets import (QAbstractItemView, QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QFileDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import sys
import os
import numpy

from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.guiUtils import *
from Gui.QtGUIutils.QtDBTableWidget import *
from Gui.QtGUIutils.QtFileDialogPreview import *

class QtImageInsertionTab(QWidget):
	def __init__(self,master):
			super(QtImageInsertionTab,self).__init__()
			self.master = master
			self.connection = self.master.connection
			self.Inserted = False

			self.mainlayout = QGridLayout()
			self.insertBox()
			self.setLayout(self.mainlayout)
	
	def insertBox(self):
		self.InsertBox = QGroupBox()
		self.InsertLayout = QGridLayout()

		self.InsertImage = QLabel('<font size="12">Insert Image:</font>')
		self.InsertImage.setMaximumHeight(60)

		self.ModuleIDLabel = QLabel("Module ID:")
		self.ModuleIDEdit = QLineEdit()
		self.ModuleIDEdit.setEchoMode(QLineEdit.Normal)
		
		self.DateTimeLabel = QLabel('Time:')
		self.DateTimeEdit = QDateTimeEdit()
		self.DateTimeEdit.setDateTime(QDateTime.currentDateTime())

		self.CaptionLabel = QLabel("Caption:")
		self.CaptionEdit = QLineEdit()
		self.CaptionEdit.setEchoMode(QLineEdit.Normal)
		self.CaptionEdit.setPlaceholderText("description")

		self.ImageLabel = QLabel("Image:")
		self.ImageEdit = QLineEdit()
		self.ImageEdit.setEchoMode(QLineEdit.Normal)
		self.ImageEdit.setPlaceholderText("image")
		self.ImageEdit.setDisabled(True)
		self.ImageButton = QPushButton("&Browser..")
		self.ImageButton.clicked.connect(self.openImageBrowser)

		self.FeedBackLabel = QLabel("")
		self.InsertButton = QPushButton("&Submit")
		self.InsertButton.clicked.connect(self.submitRequest)

		self.InsertLayout.addWidget(self.InsertImage,0,0,1,4,Qt.AlignTop)

		self.InsertLayout.addWidget(self.ModuleIDLabel,1,0,1,1)
		self.InsertLayout.addWidget(self.ModuleIDEdit,1,1,1,1)
		self.InsertLayout.addWidget(self.DateTimeLabel,1,2,1,1)
		self.InsertLayout.addWidget(self.DateTimeEdit,1,3,1,1)

		self.InsertLayout.addWidget(self.CaptionLabel,2,0,1,1)
		self.InsertLayout.addWidget(self.CaptionEdit,2,1,1,1)

		self.InsertLayout.addWidget(self.ImageLabel,3,0,1,1)
		self.InsertLayout.addWidget(self.ImageEdit,3,1,1,1)
		self.InsertLayout.addWidget(self.ImageButton,3,3,1,1)

		self.InsertLayout.addWidget(self.FeedBackLabel,4,0,1,10)
		self.InsertLayout.addWidget(self.InsertButton,4,10,1,1)
		

		self.InsertBox.setLayout(self.InsertLayout)
		self.mainlayout.addWidget(self.InsertBox,0,0,1,1)

	def submitRequest(self):
		self.InsertButton.setDisabled(True)
		if self.ModuleIDEdit.text() == "":
			self.FeedBackLabel.setStyleSheet("color:red")
			self.FeedBackLabel.setText("Failed: Module_ID is missing")

		if self.SelectedFile != "":
			file_stat = os.stat(self.SelectedFile)
			if file_stat.st_size/(1024.0 * 1024.0) > 10.0:
				self.FeedBackLabel.setStyleSheet("color:red")
				self.FeedBackLabel.setText("Failed: the image size exceeds 10 Mb")
				return
		
		try:
			args = describeTable(self.connection,"images")
			Data = []
			SubmitArgs = []
			for arg in args:
				if "_id" in arg:
					Data.append(self.ModuleIDEdit.text())
					SubmitArgs.append(arg)
				if "date" in arg:
					Data.append(self.DateTimeEdit.dateTime().toUTC().toString("yyyy-dd-MM hh:mm:ss.z"))
					SubmitArgs.append(arg)
				if "username" in arg:
					Data.append(self.master.TryUsername)
					SubmitArgs.append(arg)
				if "caption" in  arg:
					Data.append(self.CaptionEdit.text())
					SubmitArgs.append(arg)
				if "image" in arg:
					binaryData = self.GetBinary(self.SelectedFile)
					Data.append(binaryData)
					SubmitArgs.append(arg)
			self.Inserted = insertGenericTable(self.connection, "images", SubmitArgs, Data)
		except Exception as err:
			print(err)
			self.FeedBackLabel.setStyleSheet("color:red")
			self.FeedBackLabel.setText("Failed: Submission is unsuccessful")
			self.InsertButton().setDisabled(False)
			return
		if self.Inserted == False:
			self.FeedBackLabel.setStyleSheet("color:red")
			self.FeedBackLabel.setText("Failed: Submission is unsuccessful")
			self.InsertButton().setDisabled(False)
			return
		self.ModuleIDLabel.deleteLater()
		self.ModuleIDEdit.deleteLater()
		self.DateTimeLabel.deleteLater()
		self.DateTimeEdit.deleteLater()
		self.CaptionLabel.deleteLater()
		self.CaptionEdit.deleteLater()
		self.ImageLabel.deleteLater()
		self.ImageEdit.deleteLater()
		self.ImageButton.deleteLater()
		self.FeedBackLabel.deleteLater()
		self.InsertButton.deleteLater()

		self.InsertLayout.removeWidget(self.ModuleIDLabel)
		self.InsertLayout.removeWidget(self.ModuleIDEdit)
		self.InsertLayout.removeWidget(self.DateTimeLabel)
		self.InsertLayout.removeWidget(self.DateTimeEdit)
		self.InsertLayout.removeWidget(self.CaptionLabel)
		self.InsertLayout.removeWidget(self.CaptionEdit)
		self.InsertLayout.removeWidget(self.ImageLabel)
		self.InsertLayout.removeWidget(self.ImageEdit)
		self.InsertLayout.removeWidget(self.ImageButton)
		self.InsertLayout.removeWidget(self.FeedBackLabel)
		self.InsertLayout.removeWidget(self.InsertButton)


		self.FeedBackLabel = QLabel("Submitted")
		self.FeedBackLabel.setStyleSheet("color:green")
		self.ContinueButton =  QPushButton('Continue')
		self.ContinueButton.clicked.connect(self.recreateTab)
		self.CloseButton =  QPushButton('Close')
		self.CloseButton.clicked.connect(self.closeTab)
		self.InsertLayout.addWidget(self.FeedBackLabel,1,0,1,2)
		self.InsertLayout.addWidget(self.ContinueButton,1,9,1,1)
		self.InsertLayout.addWidget(self.CloseButton,1,10,1,1)


	def openImageBrowser(self):
		try:
			ImageBrowser = QFileDialog()
			options = ImageBrowser.Option()
			options |= ImageBrowser.DontUseNativeDialog
			ImageBrowser.setFileMode(QFileDialog.ExistingFiles)
			ImageBrowser.setNameFilter(("All Files (*);;Images (*.jpeg *.jpg *.png *.gif)"))
			if ImageBrowser.exec_():
				self.SelectedFileList = ImageBrowser.selectedFiles()
			if  len(self.SelectedFileList) == 0:
				return
			self.SelectedFile = self.SelectedFileList[0]
			if self.SelectedFile.split('.')[-1] not in ["jpeg","jpg","png","gif"]:
				QMessageBox.information(None,"Warning","Not a valid image file", QMessageBox.Ok)
			self.ImageEdit.setText(self.SelectedFile)
		except Exception as err:
			print(err)
			QMessageBox.information(None,"Warning","File did not selected", QMessageBox.Ok)


	def GetBinary(self, filename):
		with open(filename, 'rb') as file:
			binaryData = file.read()
		return binaryData	

	def recreateTab(self):
		self.master.closeTab(self.master.MainTabs.currentIndex())
		self.master.insertImage()
	
	def closeTab(self):
		self.master.closeTab(self.master.MainTabs.currentIndex())

		