
import logging

# Customize the logging configuration
logging.basicConfig(
   level=logging.INFO,
   format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
   filename='my_project.log',  # Specify a log file
   filemode='w'  # 'w' for write, 'a' for append
)

logger = logging.getLogger(__name__)

from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtGui import QBrush, QColor, QPixmap, QStandardItemModel, QStandardItem
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableView, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox, QSplitter)

import sys
import os
import subprocess
from subprocess import Popen, PIPE

from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.guiUtils import *
from Gui.GUIutils.settings import *

class QtDBTableWidget(QSortFilterProxyModel):
	def __init__(self, dataList, orderIndex = 0, withButton = False):
		super(QtDBTableWidget,self).__init__()

		self.orderIndex = int(orderIndex)

		if len(dataList) ==  0:
			self.dataHeader = []
			self.dataBody = []
		elif len(dataList) == 1:
			self.dataHeader = dataList[0]
			self.dataBody = []
		else:
			self.dataHeader = dataList[0]
			self.dataBody = dataList[1:]
			self.dataBody.sort(key=lambda x: x[self.orderIndex], reverse=True)

		self.model = QStandardItemModel()
		if withButton:
			self.model.setHorizontalHeaderLabels(["Details"]+self.dataHeader)
		else:
			self.model.setHorizontalHeaderLabels(self.dataHeader)

		for row in range(len(self.dataBody)):
			RowContents = []
			if withButton:
				ButtonItem = QStandardItem()
				RowContents.append(ButtonItem)
			for column in range(len(self.dataBody[row])):
				item = QStandardItem("{0}".format(self.dataBody[row][column])) 
				RowContents.append(item)
			self.model.invisibleRootItem().appendRow(RowContents)		
		self.setSourceModel(self.model)

	@QtCore.pyqtSlot()
	def on_actionAll_triggered(self):
		filterColumn = self.logicalIndex
		filterString = QtCore.QRegExp(  "",
										QtCore.Qt.CaseInsensitive,
										QtCore.QRegExp.RegExp
										)

		self.setFilterRegExp(filterString)
		self.setFilterKeyColumn(filterColumn)

	@QtCore.pyqtSlot(int)
	def on_signalMapper_mapped(self, i):
		stringAction = self.signalMapper.mapping(i).text()
		filterColumn = self.logicalIndex
		filterString = QtCore.QRegExp(  stringAction,
										QtCore.Qt.CaseSensitive,
										QtCore.QRegExp.FixedString
										)

		self.setFilterRegExp(filterString)
		self.setFilterKeyColumn(filterColumn)

	@QtCore.pyqtSlot(str)
	def on_lineEdit_textChanged(self, text):
		search = QtCore.QRegExp(    text,
									QtCore.Qt.CaseInsensitive,
									QtCore.QRegExp.RegExp
									)

		self.setFilterRegExp(search)

	@QtCore.pyqtSlot(int)
	def on_comboBox_currentIndexChanged(self, index):
		self.setFilterKeyColumn(index)
