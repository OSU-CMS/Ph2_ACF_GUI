from PyQt5.QtCore import *
from PyQt5.QtGui import QFont, QPixmap 
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import matplotlib
matplotlib.use('Qt5Agg')
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

import random
import numpy

from Gui.GUIutils.settings import *

## Class for Module testing Summary
class SummaryCanvas(FigureCanvas):
	def __init__(self, parent=None, width=5, height=4, dpi=100):
		fig = Figure(figsize=(width, height), dpi=dpi)
		self.axes = fig.add_subplot(111)

		self.compute_initial_figure()

		FigureCanvas.__init__(self, fig)
		self.setParent(parent)

		FigureCanvas.setSizePolicy(self,
								   QSizePolicy.Expanding,
								   QSizePolicy.Expanding)
		FigureCanvas.updateGeometry(self)
		
	def compute_initial_figure(self):
		t = numpy.arange(0.0, 3.0, 0.01)
		s = numpy.power(t,2)
		self.axes.plot(t, s)


class RunStatusCanvas(FigureCanvas):
	def __init__(self, parent, width=5, height=4, dpi=100):
		self.parent = parent
		self.xticks = ['']
		for i in range(len(CompositeList[self.parent.info[1]])):
			self.xticks.append(Test[CompositeList[self.parent.info[1]][i]])
		self.grades = self.parent.grades

		self.fig = Figure(figsize=(width, height), dpi=dpi)
		self.axes = self.fig.add_subplot(111)

		self.compute_initial_figure()

		FigureCanvas.__init__(self, self.fig)
		self.setParent(self.parent)

		FigureCanvas.setSizePolicy(self,
								   QSizePolicy.Expanding,
								   QSizePolicy.Expanding)
		FigureCanvas.updateGeometry(self)
		
	def compute_initial_figure(self):
		self.axes.cla()
		x = numpy.array(range(1,1+len(CompositeList[self.parent.info[1]])))
		t = numpy.array(self.xticks)
		y = numpy.array(self.grades)
		addLength = len(x)-len(y)
		for i in range(addLength):
			y = numpy.append(y,[0.0])
		
		self.axes.bar(x,y)
		self.axes.set_xticks(range(len(t)))
		self.axes.set_xticklabels(t)
	
	def renew(self):
		self.xticks = ['']
		for i in range(len(CompositeList[self.parent.info[1]])):
			self.xticks.append(Test[CompositeList[self.parent.info[1]][i]])
		self.grades = self.parent.grades
		self.compute_initial_figure()
		