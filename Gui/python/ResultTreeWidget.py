from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtGui import QPixmap
from PyQt5.QtWidgets import (QAbstractItemView, QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit, QListWidget, 
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableView, QTableWidget, QTabWidget, QTextEdit, QTreeWidget, QTreeWidgetItem, QWidget, QMainWindow)

import sys
import os
import subprocess
import logging

from functools import partial
from Gui.python.ROOTInterface import *

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class ResultTreeWidget(QWidget):
	def __init__(self,width,height):
		super(ResultTreeWidget,self).__init__()
		self.DisplayW = width
		self.DisplayH = height
		self.FileList = []
		self.displayingImage = ""

		self.mainLayout = QGridLayout()
		self.setLayout(self.mainLayout)
		self.setupUi()

		# For test:
		# self.updateResult("/Users/czkaiweb/Research/data")
		
	def setupUi(self):
		self.DisplayTitle = QLabel('<font size="6"> Result: </font>')
		self.DisplayLabel = QLabel()
		self.DisplayLabel.setScaledContents(True)
		self.displayingImage = 'test_plots/test_best1.png'
		self.DisplayView = QPixmap('test_plots/test_best1.png').scaled(QSize(self.DisplayW,self.DisplayH), Qt.KeepAspectRatio, Qt.SmoothTransformation)
		self.DisplayLabel.setPixmap(self.DisplayView)
		self.ReferTitle = QLabel('<font size="6"> Reference: </font>')
		self.ReferLabel = QLabel()
		self.ReferLabel.setScaledContents(True)
		self.ReferView = QPixmap('test_plots/test_best1.png').scaled(QSize(self.DisplayW,self.DisplayH), Qt.KeepAspectRatio, Qt.SmoothTransformation)
		self.ReferLabel.setPixmap(self.ReferView)

		self.OutputTree = QTreeWidget()
		self.OutputTree.setHeaderLabels(['Name'])
		self.OutputTree.itemClicked.connect(self.onItemClicked)
		self.TreeRoot = QTreeWidgetItem(self.OutputTree)
		self.TreeRoot.setText(0,"Files..")

		self.mainLayout.addWidget(self.DisplayTitle,0,0,1,2)
		self.mainLayout.addWidget(self.DisplayLabel,1,0,1,2)
		#self.mainLayout.addWidget(self.ReferTitle,0,2,1,2)
		#self.mainLayout.addWidget(self.ReferLabel,1,2,1,2)
		self.mainLayout.addWidget(self.OutputTree,0,4,2,1)

	def resizeImage(self, width, height):
		self.DisplayView = QPixmap(self.displayingImage).scaled(QSize(width,height), Qt.KeepAspectRatio, Qt.SmoothTransformation)
		self.DisplayLabel.setPixmap(self.DisplayView)
		self.update

	@QtCore.pyqtSlot(QTreeWidgetItem, int)
	def onItemClicked(self, item, col):
		## To be  removed
		#texts = []
		#while item is not None:
		#	texts.insert(0,item.text(0))
		#	item = item.parent()
		#path = "/".join(texts)
		#logger.info('Selected item: ' + path)

		if item.text(0).endswith(";TCanvas"):
			canvas = item.data(0,Qt.UserRole)
			self.displayResult(canvas)

	def DirectoryVAL(self, QTreeNode, node):
		if node.getDaugthers() != []:
			for Node in node.getDaugthers():
				CurrentNode = QTreeWidgetItem()
				if Node.getClassName() ==  "TCanvas":
					CurrentNode.setText(0,Node.getKeyName()+";TCanvas")
					CurrentNode.setData(0,Qt.UserRole,Node.getObject())
				else:
					CurrentNode.setText(0,Node.getKeyName())
				QTreeNode.addChild(CurrentNode)
				self.DirectoryVAL(CurrentNode,Node)
		else:
			return 

	def getResult(self, QTreeNode, sourceFile):
		Nodes = GetDirectory(sourceFile)
		CurrentNode = QTreeWidgetItem()
		for Node in Nodes:
			CurrentNode = QTreeWidgetItem()
			CurrentNode.setText(0,Node.getKeyName())
			QTreeNode.addChild(CurrentNode)
			self.DirectoryVAL(CurrentNode, Node)

	def updateResult(self, sourceFolder):
		process = subprocess.run('find {0} -type f -name "*.root" '.format(sourceFolder), shell=True, stdout=subprocess.PIPE)
		stepFiles = process.stdout.decode('utf-8').rstrip("\n").split("\n")
		if stepFiles == [""]:
			return
		self.FileList += stepFiles

		for File in self.FileList:
			CurrentNode = QTreeWidgetItem()
			CurrentNode.setText(0,File.split("/")[-1])
			CurrentNode.setData(0,Qt.UserRole,File)
			self.TreeRoot.addChild(CurrentNode)
			self.getResult(CurrentNode, File)
			
	def displayResult(self, canvas):
		tmpDir = os.environ.get('GUI_dir')+"/Gui/.tmp"
		if not os.path.isdir(tmpDir)  and os.environ.get('GUI_dir'):
			try:
				os.mkdir(tmpDir)
				logger.info("Creating "+tmpDir)
			except:
				logger.warning("Failed to create "+tmpDir)
		jpgFile = TCanvas2JPG(tmpDir, canvas)
		self.displayingImage = jpgFile

		try:
			self.DisplayView = QPixmap(jpgFile).scaled(QSize(self.DisplayW,self.DisplayH), Qt.KeepAspectRatio, Qt.SmoothTransformation)
			self.DisplayLabel.setPixmap(self.DisplayView)
			self.update
			logger.info("Displaying " + jpgFile)
		except:
			logger.error("Failed to display " + jpgFile)
		pass


