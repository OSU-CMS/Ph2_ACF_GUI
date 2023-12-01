from PyQt5 import QtCore
from PyQt5 import QtSerialPort
from PyQt5.QtCore import *
from PyQt5.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QDateTimeEdit,
    QDial,
    QDialog,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QProgressBar,
    QPushButton,
    QRadioButton,
    QScrollBar,
    QSizePolicy,
    QSlider,
    QSpinBox,
    QStyleFactory,
    QTableWidget,
    QTabWidget,
    QTextEdit,
    QHBoxLayout,
    QVBoxLayout,
    QWidget,
    QMainWindow,
    QMessageBox,
)

from UserCustoms.python.ArduinoParser import *
import pyvisa as visa
import subprocess
import numpy as np
import Gui.siteSettings as site_settings
from Gui.python.logging_config import logger



class ArduinoWidget(QWidget):
    stop = pyqtSignal()

    def __init__(self):
        super(ArduinoWidget, self).__init__()
        self.mainLayout = QGridLayout()
        self.createArduino()
        self.setLayout(self.mainLayout)
        self.serial = None
        self.stopCount = 0
        self.ArduinoGoodStatus = False

    def createArduino(self):
        self.ArduinoGroup = QGroupBox("Arduino device")
        self.ArduinoBox = QHBoxLayout()
        self.ArduinoStatusLabel = QLabel()
        self.ArduinoStatusLabel.setText("Choose Arduino:")
        self.ArduinoList = self.listResources()
        self.ArduinoCombo = QComboBox()
        self.ArduinoCombo.addItems(self.ArduinoList)
        self.ArduinoBaudRate = QLabel()
        self.ArduinoBaudRate.setText("BaudRate:")
        self.ArduinoBaudRateList = [
            str(rate) for rate in [1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200]
        ]
        self.ArduinoBRCombo = QComboBox()
        self.ArduinoBRCombo.addItems(self.ArduinoBaudRateList)
        # self.ArduinoValues = QLabel()
        self.UseArduino = QPushButton("&Use")
        self.UseArduino.clicked.connect(self.frozeArduinoPanel)
        self.ReleaseArduino = QPushButton("&Release")
        self.ReleaseArduino.clicked.connect(self.releaseArduinoPanel)
        self.TempScriptUpload = QPushButton("&Upload script")
        self.TempScriptUpload.clicked.connect(self.TemScriptUploder)
        self.ReleaseArduino.setDisabled(True)
        self.ArduinoBox.addWidget(self.ArduinoStatusLabel)
        self.ArduinoBox.addWidget(self.ArduinoCombo)
        self.ArduinoBox.addWidget(self.ArduinoBaudRate)
        self.ArduinoBox.addWidget(self.ArduinoBRCombo)
        self.ArduinoBox.addStretch(1)
        self.ArduinoBox.addWidget(self.UseArduino)
        self.ArduinoBox.addWidget(self.ReleaseArduino)
        self.ArduinoBox.addWidget(self.TempScriptUpload)
        self.ArduinoGroup.setLayout(self.ArduinoBox)
        self.mainLayout.addWidget(self.ArduinoGroup, 0, 0)

        self.ArduinoMeasureValue = QLabel()
        self.mainLayout.addWidget(self.ArduinoMeasureValue, 1, 0)
    

    def listResources(self):
        self.ResourcesList = []
        self.ResourcesList.append(site_settings.TemArduinoPort)
        return self.ResourcesList
    

    def TemScriptUploder(self):
        
        try:


            compileResult=subprocess.run(
            "$GUI_dir/bin/arduino-cli compile --fqbn arduino:avr:uno $GUI_dir/FirmwareImages/relay_box_firmware/",
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
            )
            deviceName = self.ArduinoCombo.currentText()
            portName = deviceName.lstrip("ASRL").rstrip("::INSTR")
            uploadResult=subprocess.run(
            f"$GUI_dir/bin/arduino-cli upload -p {portName} --fqbn arduino:avr:uno $GUI_dir/FirmwareImages/relay_box_firmware/",
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
            )
            

            if "New upload port" in str(uploadResult.stdout):
                print("relay box script upload completed")

                 
        except subprocess.CalledProcessError as err:            
            logger.error("Unable to TM upload script to Arduino")
            logger.error(compileResult.stderr)
            logger.error(uploadResult.stderr)

        
    def frozeArduinoPanel(self):
        # Block for ArduinoSupply operation
        
        
        
        try:
                        
            self.setSerial(
                self.ArduinoCombo.currentText(),
                self.ArduinoBRCombo.currentText(),
            )
            self.ArduinoCombo.setDisabled(True)
            self.ArduinoBRCombo.setDisabled(True)
            self.UseArduino.setDisabled(True)
            self.ReleaseArduino.setDisabled(False)
        except Exception as err:
            logger.error("Unable to use Arduino")
            self.ArduinoGoodStatus = False
        
    def releaseArduinoPanel(self):
        self.serial.close()
        self.ArduinoCombo.setDisabled(False)
        self.ArduinoMeasureValue.setText("")
        self.UseArduino.setDisabled(False)
        self.ArduinoBRCombo.setDisabled(False)
        self.ReleaseArduino.setDisabled(True)
        self.ArduinoList = self.listResources()

    def setBaudRate(self, baudRate):
        self.ArduinoBRCombo.clear()
        self.ArduinoBRCombo.addItems([str(baudRate)])

    def setSerial(self, deviceName, baudRate):
        deviceName = deviceName.lstrip("ASRL").rstrip("::INSTR")
        baudMap = {
            "1200": QtSerialPort.QSerialPort.Baud1200,
            "2400": QtSerialPort.QSerialPort.Baud2400,
            "4800": QtSerialPort.QSerialPort.Baud4800,
            "9600": QtSerialPort.QSerialPort.Baud9600,
            "19200": QtSerialPort.QSerialPort.Baud19200,
            "38400": QtSerialPort.QSerialPort.Baud38400,
            "57600": QtSerialPort.QSerialPort.Baud57600,
            "115200": QtSerialPort.QSerialPort.Baud115200,
        }
        self.serial = QtSerialPort.QSerialPort(
            deviceName, baudRate=baudMap[baudRate], readyRead=self.receive
        )
        self.serial.open(QIODevice.ReadOnly)
        print(self.serial.isOpen())

    def disable(self):
        self.ArduinoGroup.setDisabled(True)

    def enable(self):
        self.ArduinoGroup.setDisabled(False)

    @QtCore.pyqtSlot()
    def receive(self):
        while self.serial.canReadLine():
            try:
                text = self.serial.readLine().data().decode("utf-8", "ignore")
                text = text.rstrip("\r\n")
                T = float(text.split(" ")[4])
                RH = float(text.split(" ")[1])
                N = (np.log(RH / 100) + 17.27 * T / (237.3 + T)) / 17.27
                Td = round(237.3 * N / (1 - N), 2)
                if T >= Td:
                    self.ArduinoMeasureValue.setStyleSheet("QLabel {color : green}")
                else:
                    self.ArduinoMeasureValue.setStyleSheet("QLabel {color : red}")

                climatetext = text + ", Dew Point Temperature: {0} Celsius".format(Td)
                StopSignal, measureText = ArduinoParser(text)
                self.ArduinoMeasureValue.setText(climatetext)
                # self.ArduinoMeasureValue.setText(measureText)
                if StopSignal:
                    self.stopCount += 1
                    logging.warning(
                        "Anomalous value detected, stop signal will be emitted in {}".format(
                            10 - self.stopCount
                        )
                    )
                    self.ArduinoGoodStatus = False
                else:
                    self.stopCount = 0
                    self.ArduinoGoodStatus = True

                if self.stopCount >= 10:
                    self.StopSignal()
                    self.stopCount = 0

            except Exception as err:
                logger.error("{0}".format(err))

    @QtCore.pyqtSlot()
    def StopSignal(self):
        self.stop.emit()
