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

# Setup Logging
logger = logging.getLogger("Ph2_ACF_GUI_logger")
logger.setLevel(logging.DEBUG)
# Setup Formatting
formatter = logging.Formatter('%(asctime)s-%(name)s - %(levelname)s - %(message)s')
# Send to File
file_handler = logging.FileHandler("Logs/Gui_log_file.log")
file_handler.setLevel(logging.Debug)
file_handler.setFormatter(formatter)

# Create Stream handler
stream_handler = logging.StreamHandler()
stream_handler.setLevel(logging.DEBUG)
stream_handler.setFormatter(formatter)

logger.addHandler(file_handler)
logger.addHandler(stream_handler)

app = QApplication([])
dimension = app.screens()[0].size()
qtApp = QtApplication(dimension)
sys.exit(app.exec_())
