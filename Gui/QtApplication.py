from PyQt5.QtCore import QDateTime, Qt, QTimer
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
	QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
	QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
	QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit,
	QVBoxLayout, QWidget)

import sys
import os
import logging

from Gui.QtGUIutils.QtApplication import *

if __name__ == "__main__":
	app = QApplication([])
	dimension = app.screens()[0].size()
	qtApp = QtApplication(dimension)
	sys.exit(app.exec_())
