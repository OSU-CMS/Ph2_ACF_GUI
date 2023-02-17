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
from Gui.GUIutils.guiUtils import *

class ScanCanvas(FigureCanvas):
	def __init__(self,parent,xlabel = "X", ylabel = "Y", sorted = True, invert = False):
		self.parent = parent
		self.sortX = sorted
		self.invert = invert
		self.fig = Figure(figsize=(5, 4), dpi=100)
		self.axes = self.fig.add_subplot(111)
		self.xlabel = xlabel
		self.ylabel = ylabel
		self.X = numpy.array([])
		self.Y = numpy.array([])
		self.compute_initial_figure()
		FigureCanvas.__init__(self, self.fig)
		self.setMinimumHeight(100)
		if type(parent) == type(QWidget()):
			self.setParent(parent)
		FigureCanvas.setSizePolicy(self,
								   QSizePolicy.Expanding,
								   QSizePolicy.Expanding)
		FigureCanvas.updateGeometry(self)

	def compute_initial_figure(self):
		self.axes.cla()
		self.axes.set_xlabel(self.xlabel)
		self.axes.set_ylabel(self.ylabel)
		self.axes.plot(self.X, self.Y, color='green', linestyle='dashed',linewidth=3)
		self.axes.invert_xaxis()
		self.axes.invert_yaxis()


	def updatePlots(self, points):
		self.coordinates = points
		if self.sortX:
			self.coordinates.sort(key=lambda x:x[0])
		self.X = numpy.array([])
		self.Y = numpy.array([])
		for coordicate in self.coordinates:
			self.X = numpy.append(self.X,coordicate[0])
			self.Y = numpy.append(self.Y,coordicate[1])
		self.compute_initial_figure()
	
	def saveToSVG(self,output):
		self.fig.savefig(output, format="svg", dpi=1200)
		return output

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


# Grading display Not used
class RunStatusCanvas(FigureCanvas):
	def __init__(self, parent, width=5, height=4, dpi=100):
		self.parent = parent
		self.xticks = ['']

		if isCompositeTest(self.parent.info[1]):
			for i in range(len(CompositeList[self.parent.info[1]])):
				self.xticks.append(Test[CompositeList[self.parent.info[1]][i]])
		if isSingleTest(self.parent.info[1]):
			self.xticks.append(Test[self.parent.info[1]])
		self.grades = self.parent.grades

		self.fig = Figure(figsize=(width, height), dpi=dpi)
		self.axes = self.fig.add_subplot(111)

		self.compute_initial_figure()

		FigureCanvas.__init__(self, self.fig)
		self.setParent(self.parent)
		self.setMinimumHeight(1)

		FigureCanvas.setSizePolicy(self,
								   QSizePolicy.Expanding,
								   QSizePolicy.Expanding)
		FigureCanvas.updateGeometry(self)
		
	def compute_initial_figure(self):
		self.axes.cla()
		if isCompositeTest(self.parent.info[1]):
			xList = [ x for x in range(1,1+len(CompositeList[self.parent.info[1]]))]
		if isSingleTest(self.parent.info[1]):
			xList = [ x for x in range(1,2)]
		
		t = numpy.array(self.xticks)
		# Fixme: Extend to grades as dictionary

		if len(self.grades) == 0:
			return

		if len(self.grades[0].keys()) == 0:
			return
		
		allGrades = []
		allXs = []
		moduleList = self.grades[0].keys()

		width_cluster = 0.7
		width_bar = width_cluster/len(moduleList)
		
		index = 0
		for module in moduleList:
			module_grades = []
			for grade in self.grades:
				module_grades.append(grade.get(module,0))
			for i in range(len(xList)-len(self.grades)):
				module_grades.append(0.0)
			allXs.append( [ x + (width_bar*index)-width_cluster/2.0 for x in xList])
			allGrades.append(module_grades)
			index = index + 1

		for i, module in enumerate(moduleList):
			x = numpy.array(allXs[i])
			self.axes.bar(x,allGrades[i], width = width_bar, align = "edge", label="Module {}".format(module))

		self.axes.plot([0,len(t)], [0.6,0.6], color='green', linestyle='dashed',linewidth=3)
		self.axes.set_xticks(range(len(t)))
		self.axes.set_xticklabels(t)
		self.axes.legend()
		self.axes.set_xlim([0,len(t)])
	
	def renew(self):
		self.xticks = ['']
		if isCompositeTest(self.parent.info[1]):
			for i in range(len(CompositeList[self.parent.info[1]])):
				self.xticks.append(Test[CompositeList[self.parent.info[1]][i]])
		if isSingleTest(self.parent.info[1]):
			self.xticks.append(Test[self.parent.info[1]])
		self.grades = self.parent.grades
		self.compute_initial_figure()
		