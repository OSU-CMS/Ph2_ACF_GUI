from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtGui import QPixmap
from PyQt5.QtWidgets import (QAbstractItemView, QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableView, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox, QSplitter)

import sys
import os
import subprocess
from subprocess import Popen, PIPE

import logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

from Gui.Config.staticSettings import *
from Configuration.XMLUtil import *
from Gui.python.Firmware import *

class QtuDTCDialog(QDialog):
	def __init__(self, master, module):
		super(QtuDTCDialog,self).__init__()
		self.master = master
		self.expertMode = self.master.expertMode
		self.module = module
		self.uDTCFile = ""

		self.mainLayout = QGridLayout()

		self.createMain()
		self.getFPGAConfigs()
		self.setLayout(self.mainLayout)

	def createMain(self):
		self.FPGAConfigBox = QGroupBox("")
		self.FPGAConfigBox.setCheckable(False)

		CommentLabel = QLabel("Please select the uDTC firmware image")

		ConfigLabel = QLabel("firmware image:")

		self.ConfigCombo = QComboBox()

		button_login = QPushButton("&OK")
		button_login.setDefault(True)
		button_login.clicked.connect(self.configFwImage)
		#button_login.clicked.connect(self.finish)

		if not self.master.expertMode:
			CommentLabel.setText("uDTC firmware can only be changed under expert mode")
			CommentLabel.setStyleSheet("font-weight:bold")
			self.ConfigCombo.setDisabled(True)

		layout = QGridLayout()
		layout.addWidget(CommentLabel,0,0,1,4)
		layout.addWidget(ConfigLabel,1,0,1,1)
		layout.addWidget(self.ConfigCombo,1,1,1,3)
		layout.addWidget(button_login,2,3,1,1)


		layout.setColumnStretch(0, 1)
		layout.setColumnStretch(1, 1)
		layout.setColumnStretch(2, 1)

		self.FPGAConfigBox.setLayout(layout)
		self.mainLayout.addWidget(self.FPGAConfigBox, 0, 0)

	def fetchFPGAConfigs(self):
		try:
			InputFile = os.environ.get('PH2ACF_BASE_DIR')+'/settings/CMSIT.xml'
			root,tree = LoadXML(InputFile)
			fwIP = self.module.getIPAddress()
			changeMade = False
			for Node in root.findall(".//connection"):
				if fwIP not in Node.attrib['uri']:
					Node.set('uri','chtcp-2.0://localhost:10203?target={}:50001'.format(fwIP))
					changeMade = True
			for Node in root.findall(".//RD53"):
				Node.set('configfile',"../Configuration/CMSIT_RD53.txt")
				changeMode = True
			#if changeMode:
				#xml_output = ET.tostring(root,pretty_print=True)
				#print(xml_output)
			ModifiedFile = InputFile+".gui"
			tree.write(ModifiedFile)
			InputFile = ModifiedFile

		except Exception as error:
			print("Failed to modify the XML:  {}".format(error))

		try:
			pipes = subprocess.run(["fpgaconfig","-c",InputFile,"-l"], stdout=PIPE, stderr=PIPE)
			stdout, stderr = pipes.stdout,pipes.stderr
			if pipes.returncode != 0:
				logger.info("{0}s. Code: {1}".format(stderr.strip(), pipes.returncode))
				return []
		
			nImages = -1
			ImageList = []
			lines = stdout.decode('UTF-8').splitlines()
			for line in lines:
				if "firmware images on SD card:" == " ".join(line.split()[4:]):
					nImages = int(line.split()[3])
				if "-" == line.split()[3] and ".bit" in line:
					ImageList.append(line.split()[4])
			if nImages != len(ImageList)+1:
				logger.warning("fetched image number is not consistent with imaged list, please check carefully")
			return ImageList

		except Exception as error:
			logger.error("Failed to fetch the firmware image list: {}".format(error))
			return []

	def getFPGAConfigs(self):
		self.ConfigNames = self.fetchFPGAConfigs()
		self.ConfigCombo.addItems(self.ConfigNames)
		self.ConfigCombo.setCurrentIndex(0)
	
	def configFwImage(self):
		self.uDTCFile = self.ConfigCombo.currentText()
		logger.info("Loading firmware image: {}".format(self.uDTCFile))
		pipe = subprocess.run(["fpgaconfig","-c",os.environ.get('PH2ACF_BASE_DIR')+'/settings/CMSIT.xml.gui',"-i",self.uDTCFile], stdout=PIPE, stderr=PIPE)
		logger.info(pipe.stdout.decode('UTF-8'))
		self.finish()

	def finish(self):
		self.accept()


