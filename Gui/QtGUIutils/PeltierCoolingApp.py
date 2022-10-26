# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'untitled.ui'
#
# Created by: PyQt5 UI code generator 5.15.4
#
# WARNING: Any manual changes made to this file will be lost when pyuic5 is
# run again.  Do not edit this file unless you know what you are doing.


from cgi import test
from PyQt5 import QtCore, QtGui, QtWidgets 
from PyQt5.QtWidgets import QWidget, QMessageBox
from PyQt5.QtCore import *
from Gui.python.Peltier import *
import time
import os

class Peltier(QWidget):
    def __init__(self, dimension):
        super(Peltier, self).__init__()
        self.pool = QThreadPool.globalInstance()
        print("Number of threads being used", self.pool.activeThreadCount())
        print("Max Number of Threads", self.pool.maxThreadCount())
        self.Ph2ACFDirectory = os.getenv("GUI_dir")
        self.setupUi()
        self.show()

    def setupUi(self):
        # MainWindow.setObjectName("MainWindow")
        # MainWindow.resize(400, 300)

        # self.centralwidget = QtWidgets.QWidget(MainWindow)
        # self.centralwidget.setObjectName("centralwidget")
        self.gridLayout = QtWidgets.QGridLayout(self)
        self.currentSetTemp = QtWidgets.QLabel("Current Set Temperature: ", self)
        self.gridLayout.addWidget(self.currentSetTemp, 3,2,1,1)
        self.startButton = QtWidgets.QPushButton("Start Peltier Controller", self)
        self.startButton.clicked.connect(self.setup)
        self.gridLayout.addWidget(self.startButton, 0,0,1,1)
        self.currentTempDisplay = QtWidgets.QLCDNumber(self)
        self.gridLayout.addWidget(self.currentTempDisplay, 3, 0, 1, 2)
        self.setTempButton = QtWidgets.QPushButton("Set Temperature", self)
        self.setTempButton.setEnabled(False)
        self.gridLayout.addWidget(self.setTempButton, 1, 1, 1, 1)
        self.setTempInput = QtWidgets.QDoubleSpinBox(self)
        self.gridLayout.addWidget(self.setTempInput, 1, 0, 1, 1)
        self.currentTempLabel = QtWidgets.QLabel(self)
        self.gridLayout.addWidget(self.currentTempLabel, 2, 0, 1, 1)

        self.polarityButton = QtWidgets.QPushButton("Change Polarity", self)
        self.polarityButton.setEnabled(False)
        self.gridLayout.addWidget(self.polarityButton, 2, 1, 1, 1)
        self.polarityButton.clicked.connect(self.changePolarity)
        self.polarityLabel = QtWidgets.QLabel("N/a", self)
        self.gridLayout.addWidget(self.polarityLabel, 0, 2, 1, 1)

        self.powerStatus = QtWidgets.QLabel(self)
        self.powerStatusLabel = QtWidgets.QLabel("Power Status of Peltier: ", self)
        self.powerButton = QtWidgets.QPushButton("Peltier Power On/Off")
        self.powerButton.setEnabled(False)
        self.powerButton.clicked.connect(self.powerToggle)
        self.gridLayout.addWidget(self.powerButton, 0, 2, 1, 1)

        self.image = QtGui.QPixmap()
        redledimage = QtGui.QImage(self.Ph2ACFDirectory + "/Gui/icons/led-red-on.png").scaled(QtCore.QSize(60,10), Qt.KeepAspectRatio, Qt.SmoothTransformation)
        self.redledpixmap = QtGui.QPixmap.fromImage(redledimage)
        greenledimage = QtGui.QImage(self.Ph2ACFDirectory + "/Gui/icons/green-led-on.png" ).scaled(QtCore.QSize(60,10), Qt.KeepAspectRatio, Qt.SmoothTransformation)
        self.greenledpixmap = QtGui.QPixmap.fromImage(greenledimage)
        self.powerStatus.setPixmap(self.redledpixmap) # The power status will initially always show that it's off, if it's actually on the status will be update in 0.5 seconds.

        self.gridLayout.addWidget(self.powerStatusLabel, 1, 2, 1, 1)
        self.gridLayout.addWidget(self.powerStatus, 1, 3, 1, 1)
        self.setTempButton.clicked.connect(self.setTemp)
        self.setLayout(self.gridLayout)

    def setup(self):
        try:
            self.pelt = PeltierController()

            # Setup controller to be controlled by computer and turn off Peltier if on
            self.setupWorker = startupWorker()
            self.setupWorker.signal.finishedSignal.connect(self.enableButtons)
            self.pool.start(self.setupWorker)
            time.sleep(1) # Needed to avoid collision with temperature and power reading

            #Start temperature and power monitoring
            self.tempPower = tempPowerReading()
            self.tempPower.signal.tempSignal.connect(lambda temp: self.currentTempDisplay.display(temp))
            self.tempPower.signal.powerSignal.connect(lambda power : self.setPowerStatus(power))
            self.pool.start(self.tempPower)

        except Exception as e:
            print("Error while attempting to setup Peltier Controller: ", e)


    def enableButtons(self):
        self.powerButton.setEnabled(True)
        self.polarityButton.setEnabled(True)
        self.setTempButton.setEnabled(True)


    def setPowerStatus(self, power):
        if power:
            self.powerStatus.setPixmap(self.greenledpixmap)
            self.powerStatusValue = 1
        else:
            self.powerStatus.setPixmap(self.redledpixmap)
            self.powerStatusValue = 0

    def powerToggle(self):
        if self.powerStatusValue == 0:
            try:
                signalworker = signalWorker('Power On/Off Write', ['0','0','0','0','0','0','0','1'])
                self.pool.start(signalworker)
                print("Turning on controller")
            except Exception as e:
                print("Could not turn on controller due to error: ", e)
        elif self.powerStatusValue == 1:
            try:
                signalworker = signalWorker('Power On/Off Write', ['0','0','0','0','0','0','0','0'])
                self.pool.start(signalworker)
                print('Turning off controller')
            except Exception as e:
                print("Could not turn off controller due to error: " , e)
        time.sleep(0.5) 


    def setTemp(self):
        try:
            self.pelt.setTemperature(self.setTempInput.value())
            self.currentSetTemp.setText(f"Current Set Temperature: {self.setTempInput.value()}")
        except Exception as e:
            print("Could not set Temperature")
            self.currentSetTemp.setText("N/a")
        # Send temperature reading to device

    def changePolarity(self):
        try:
            polarity = self.pelt.changePolarity()
            self.polarityLabel.setText(f"Change Polarity: {polarity}")
        except Exception as e:
            print("Could not change polarity due to error: " , e)

    def getPower(self):
        try:
            self.power = self.pelt.checkPower()
        except Exception as e:
            self.powerTimer.stop()
            print("Could not check power due to error: " , e)
    
    def showTemp(self):
        try:
            temp = self.pelt.readTemperature()
            self.currentTempDisplay.display(temp)
        except Exception as e:
            self.timer.stop()
            print("Could not read temperature due to error: ", e)

if __name__ == "__main__":
    import sys
    app = QtWidgets.QApplication(sys.argv)
    ui = Peltier(500)
    sys.exit(app.exec_())
