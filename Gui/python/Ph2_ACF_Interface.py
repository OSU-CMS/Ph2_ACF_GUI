import sys
import os

from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtGui import (QPixmap, QTextCursor, QColor)
from PyQt5.QtWidgets import (QAbstractItemView, QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit, QListWidget, QPlainTextEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableView, QTableWidget, QTableWidgetItem, QTabWidget, QTextEdit, QTreeWidget, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox, QSplitter)

class Ph2_ACF_Interface(QObject):

    def __init__(self):
        super(Ph2_ACF_Interface, self).__init__()

        try: 
            sys.path.insert(1,os.getenv('Ph2_ACF_AREA'))
            import pythonUtils.Ph2_ACF_StateMachine as Ph2_ACF_StateMachine
            #configureLogger(os.getenv('Ph2_ACF_AREA') + "/settings/logger.config")
            self.sm = Ph2_ACF_StateMachine.StateMachine()
        except ModuleNotFoundError:
            print('Ph2_ACF is not reachable')

    def listFirmware(self, xml_file, boardId):
        try:
            firmwareList = self.sm.listFirmware(xml_file, boardId)
            return firmwareList
        except:
            print('Firmware list is empty. Check FC7 connection and xml format.')

    def loadFirmware(self, xml_file, firmwareName, boardID):
        self.sm.loadFirmware(xml_file, firmwareName, boardID)
        if self.sm.isSuccess():
            print('Firmware {0} successfully loaded'.format(firmwareName))
        else: 
            print('Warning: no firmware was loaded!')

    def uploadFirmware(self, xml_file, firmwareName, boardID):
        firmwarePath = './FirmwareImages/{0}'.formate(firmwareName)
        self.sm.uploadFirmware(xml_file, firmwareName, firmwarePath, boardID)
        if self.sm.isSuccess():
            print('Firmware {0} saved to FC7'.format(firmwareName))
        else:
            print('Warning: firmware could not be saved to FC7')

    def runCalibration(self, xml_file, outputDirectory, runNumber, testName):
      
        self.sm.setRunNumber(runNumber)
        self.sm.setConfigurationFile(xml_file)
        self.sm.setCalibrationName(testName)
        os.environ['DATA_dir'] = outputDirectory
        self.sm.runCalibration()
        error_message = self.sm.getErrorMessage()
        print(error_message)

    