
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
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import sys
import os
import numpy

from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.guiUtils import *
from Gui.QtGUIutils.QtDBTableWidget import *

class QtViewTableTab(QWidget):
	def __init__(self,master):
			super(QtViewTableTab,self).__init__()
			self.master = master
			self.connection = self.master.connection

			self.mainlayout = QGridLayout()
			self.searchBox()
			self.viewBox()
			self.setLayout(self.mainlayout)
	
	def searchBox(self):
		self.SearchBox = QGroupBox()
		self.SearchLayout = QGridLayout()

		self.DatabaseViewer = QLabel('<font size="12"> Database Viewer </font>')

		tableList = getTableList(self.connection)

		self.TableLabel = QLabel("Table:")
		self.TableCombo = QComboBox()
		if len(tableList) > 0:
			self.TableCombo.addItems("{0}".format(x) for x in tableList)
		for i in range(self.TableCombo.count()):
				if self.TableCombo.itemText(i) == "images" or self.TableCombo.itemText(i) == "tests":
					self.TableCombo.model().item(i).setEnabled(False)

		self.ConditionSyntax = QLabel("Condition Syntax:")
		self.ConditionEdit = QLineEdit()
		self.ConditionEdit.setEchoMode(QLineEdit.Normal)
		self.ConditionEdit.setPlaceholderText('key1 = value AND/OR key2 = "str"')
		
		self.SearchButton = QPushButton("Search")
		self.SearchButton.clicked.connect(self.showSearchResult)

		self.FeedBackLabel = QLabel("")

		self.SearchLayout.addWidget(self.DatabaseViewer,0,0,1,4)

		self.SearchLayout.addWidget(self.TableLabel,1,0,1,1)
		self.SearchLayout.addWidget(self.TableCombo,1,1,1,1)

		self.SearchLayout.addWidget(self.ConditionSyntax,2,0,1,1)
		self.SearchLayout.addWidget(self.ConditionEdit,2,1,1,3)

		self.SearchLayout.addWidget(self.FeedBackLabel,3,0,1,6)
		self.SearchLayout.addWidget(self.SearchButton,3,6,1,1)
		

		self.SearchBox.setLayout(self.SearchLayout)
		self.mainlayout.addWidget(self.SearchBox,0,0,1,1)

	def viewBox(self, dataList=[]):
		self.ViewBox = QGroupBox()
		self.ViewLayout= QGridLayout()

		try:
			proxy = QtDBTableWidget(dataList)

			lineEdit       = QLineEdit()
			lineEdit.textChanged.connect(proxy.on_lineEdit_textChanged)
			view           = QTableView()
			view.setSortingEnabled(True)
			comboBox       = QComboBox()
			if len(dataList) > 0:
				comboBox.addItems(["{0}".format(x) for x in dataList[0]])
			comboBox.currentIndexChanged.connect(proxy.on_comboBox_currentIndexChanged)
			label          = QLabel()
			label.setText("Regex Filter")

			view.setModel(proxy)
			view.setSelectionBehavior(QAbstractItemView.SelectRows)
			view.setSelectionMode(QAbstractItemView.MultiSelection)
			view.setEditTriggers(QAbstractItemView.NoEditTriggers)

			self.ViewLayout.addWidget(lineEdit, 0, 1, 1, 1)
			self.ViewLayout.addWidget(view, 1, 0, 1, 3)
			self.ViewLayout.addWidget(comboBox, 0, 2, 1, 1)
			self.ViewLayout.addWidget(label, 0, 0, 1, 1)

		except:
			print("Error: failed to create viewBox")

		self.ViewBox.setLayout(self.ViewLayout)
		self.mainlayout.addWidget(self.ViewBox,1,0,2,1)

	def showSearchResult(self):
		if ";" in self.ConditionEdit.text():
			self.FeedBackLabel.setText('please remove ";" from the condition syntax')
			return

		try:
			dataList = [describeTable(self.connection, self.TableCombo.currentText(), True)]
			if self.ConditionEdit.text() == "":
				dataList += retrieveGenericTable(self.connection, self.TableCombo.currentText())
			else:
				dataList += retrieveWithConstraintSyntax(self.connection, self.TableCombo.currentText(),self.ConditionEdit.text())
		except mysql.connector.Error as error:
			self.FeedBackLabel.setText("Failed retrieving MySQL table: {}".format(error))
			return 

		if len(dataList) <= 1:
			self.FeedBackLabel.setText("No Record Found")
		self.ViewBox.deleteLater()
		self.mainlayout.removeWidget(self.ViewBox)
		self.viewBox(dataList)
		
	

		