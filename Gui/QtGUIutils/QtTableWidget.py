
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

Color={
	"Local"  : QColor(255,0,0),
	"Remote" : QColor(0,255,0),
	"Synced" : QColor(0,255,255),
}

class QtTableWidget(QSortFilterProxyModel):
	filtering = pyqtSignal()
	def __init__(self, dataList, orderIndex = 4):
		super(QtTableWidget,self).__init__()

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
		self.model.setHorizontalHeaderLabels(["Details"]+self.dataHeader)

		for row in range(len(self.dataBody)):
			RowContents = []
			ButtonItem = QStandardItem()			
			RowContents.append(ButtonItem)
			for column in range(len(self.dataBody[row])):
				if column == 0:
					brush = QBrush()
					brush.setColor(Color[self.dataBody[row][0]])
				item = QStandardItem("{0}".format(self.dataBody[row][column])) 
				item.setForeground(brush)
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
		self.setFilterKeyColumn(filterColumn+1)
		self.filtering.emit()

	@QtCore.pyqtSlot(int)
	def on_signalMapper_mapped(self, i):
		stringAction = self.signalMapper.mapping(i).text()
		filterColumn = self.logicalIndex
		filterString = QtCore.QRegExp(  stringAction,
										QtCore.Qt.CaseSensitive,
										QtCore.QRegExp.FixedString
										)

		self.setFilterRegExp(filterString)
		self.setFilterKeyColumn(filterColumn+1)
		self.filtering.emit()

	@QtCore.pyqtSlot(str)
	def on_lineEdit_textChanged(self, text):
		search = QtCore.QRegExp(    text,
									QtCore.Qt.CaseInsensitive,
									QtCore.QRegExp.RegExp
									)

		self.setFilterRegExp(search)
		self.filtering.emit()

	@QtCore.pyqtSlot(int)
	def on_comboBox_currentIndexChanged(self, index):
		self.setFilterKeyColumn(index+1)
		self.filtering.emit()
