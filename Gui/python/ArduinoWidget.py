from PyQt5 import QtCore
from PyQt5 import QtSerialPort
from PyQt5.QtCore import *
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDateTimeEdit,
		QDial, QDialog, QFormLayout, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QLineEdit,
		QProgressBar, QPushButton, QRadioButton, QScrollBar, QSizePolicy,
		QSlider, QSpinBox, QStyleFactory, QTableWidget, QTabWidget, QTextEdit, QHBoxLayout,
		QVBoxLayout, QWidget, QMainWindow, QMessageBox)

import pyvisa as visa
import subprocess

import logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class ArduinoWidget(QWidget):
	def __init__(self):
		super(ArduinoWidget,self).__init__()
		self.mainLayout = QGridLayout()
		self.createArduino()
		self.setLayout(self.mainLayout)
		self.serial = None


	def createArduino(self):
		self.ArduinoGroup = QGroupBox("Arduino device")
		self.ArduinoBox = QHBoxLayout()
		self.ArduinoStatusLabel = QLabel()
		self.ArduinoStatusLabel.setText("Choose Arduino:")
		self.ArduinoList = self.listResources()
		self.ArduinoCombo = QComboBox()
		self.ArduinoCombo.addItems(self.ArduinoList)
		self.UseArduino = QPushButton("&Use")
		self.UseArduino.clicked.connect(self.frozeArduinoPanel)
		self.ReleaseArduino = QPushButton("&Release")
		self.ReleaseArduino.clicked.connect(self.releaseArduinoPanel)
		self.ReleaseArduino.setDisabled(True)

		self.ArduinoBox.addWidget(self.ArduinoStatusLabel)
		self.ArduinoBox.addWidget(self.ArduinoCombo)
		self.ArduinoBox.addStretch(1)
		self.ArduinoBox.addWidget(self.UseArduino)
		self.ArduinoBox.addWidget(self.ReleaseArduino)
		self.ArduinoGroup.setLayout(self.ArduinoBox)
		self.mainLayout.addWidget(self.ArduinoGroup,0,0)

		self.ArduionMeasureValue = QLabel()
		self.mainLayout.addWidget(self.ArduionMeasureValue,0,1)


	def listResources(self):
		self.ResourcesManager = visa.ResourceManager('@py')
		try:
			self.ResourcesList = self.ResourcesManager.list_resources()
			self.getDeviceName()
			return list(self.deviceMap.keys())
		except Exception as err:
			logger.error("Failed to list all resources: {}".format(err))
			self.ResourcesList = ()
			return self.ResourcesList

	def getDeviceName(self):
		self.deviceMap = {}
		for device in self.ResourcesList:
			try:
				pipe = subprocess.Popen(['udevadm', 'info', ' --query', 'all', '--name', device.lstrip("ASRL").rstrip("::INSTR"), '--attribute-walk'], stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
				raw_output = pipe.communicate()[0]
				vendor_list = [info for info in  raw_output.splitlines() if b'ARRTS{idVendor}' in info or b'ARRTS{vendor}' in info]
				product_list = [info for info in  raw_output.splitlines() if b'ARRTS{idProduct}' in info or b'ARRTS{product}' in info]
				idvendor = vendor_list[0].decode("UTF-8").split("==").lstrip('"').rstrip('"').replace('0x','')
				idproduct = product_list[0].decode("UTF-8").split("==").lstrip('"').rstrip('"').replace('0x','')
				deviceId = "{}:{}".format(idvendor,idproduct)
				pipeUSB = subprocess.Popen(['lsusb','-d',deviceId], stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
				usbInfo = pipeUSB.communicate()[0]
				deviceName = usbInfo.decode("UTF-8").split(deviceId)[-1].lstrip(' ')
				
				if deviceName == None:
					logger.warning("No device name found for {}:".format(device))
					#self.deviceMap[device] = device
				elif "Arduino" in deviceName:
					self.deviceMap[deviceName] = device
				else:
					pass
			except Exception as err:
				logger.error("Error found:{}".format(err))
				#self.deviceMap[device] = device

	def frozeArduinoPanel(self):
		# Block for ArduinoSupply operation
		self.setSerial(self.deviceMap[self.ArduinoCombo.currentText()])
		self.ArduinoCombo.setDisabled(True)
		self.UseArduino.setDisabled(True)
		self.ReleaseArduino.setDisabled(False)


	def releaseArduinoPanel(self):
		self.serial.close()
		self.ArduinoCombo.setDisabled(False)
		self.ArduinoStatusValue.setText("")
		self.UseArduinoSupply.setDisabled(False)
		self.ReleaseArduinoSupply.setDisabled(True)
		self.ArduinoList = self.listResources()

	def setSerial(self, deviceName):
		self.serial = QtSerialPort.QSerialPort(
			deviceName,
			baudRate=QtSerialPort.QSerialPort.Baud9600,
			readyRead=self.receive
		)
		self.serial.open()

	@QtCore.pyqtSlot()
	def receive(self):
		while self.serial.canReadLine():
			text = self.serial.readLine().data().decode()
			text = text.rstrip('\r\n')
			logger.info("Arduino Measurement: {}".format(text))
			self.ArduionMeasureValue.setText(text)