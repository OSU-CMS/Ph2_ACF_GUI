from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtGui import (QPixmap, QTextCursor)
from PyQt5.QtWidgets import (QAbstractItemView, QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit, QListWidget, QPlainTextEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableView, QTableWidget, QTabWidget, QTextEdit, QTreeWidget, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox, QSplitter)

class QtTCanvasWidget(QWidget):
	resized = pyqtSignal()
	def __init__(self,master,canvas):
		super(QtTCanvasWidget,self).__init__()
		self.canvas = canvas
		self.master = master
		self.DisplayH = self.height()*0.9
		self.DisplayW = self.width()*0.9

		self.mainLayout = QGridLayout()
		self.setLayout(self.mainLayout)

		self.setupUi()
		self.drawPlot()
		self.resized.connect(self.rescaleImage)

	def setupUi(self):
		X = self.master.dimension.width()*6./10
		Y = self.master.dimension.height()*6./10
		W = self.master.dimension.width()*3./10
		H = self.master.dimension.height()*3./10
		self.setGeometry(X,Y,W,H)  
		self.setWindowTitle('Result') 
		self.DisplayH = self.height()*0.9
		self.DisplayW = self.width()*0.9
		self.show()

	def drawPlot(self):
		self.DisplayTitle = QLabel('<font size="6"> Result: </font>')
		self.DisplayLabel = QLabel()
		self.DisplayLabel.setScaledContents(True)
		self.DisplayView = QPixmap(self.canvas).scaled(QSize(self.DisplayW,self.DisplayH), Qt.KeepAspectRatio, Qt.SmoothTransformation)
		self.DisplayLabel.setPixmap(self.DisplayView)

		self.mainLayout.addWidget(self.DisplayTitle,0,0,1,1)
		self.mainLayout.addWidget(self.DisplayLabel,1,0,9,1)

	def resizeEvent(self, event):
		self.resized.emit()
		return super(QtTCanvasWidget, self).resizeEvent(event)

	def rescaleImage(self):
		self.DisplayH = self.height()*0.9
		self.DisplayW = self.width()*0.9
		self.DisplayView = QPixmap(self.canvas).scaled(QSize(self.DisplayW,self.DisplayH), Qt.KeepAspectRatio, Qt.SmoothTransformation)
		self.DisplayLabel.setPixmap(self.DisplayView)
		self.update()