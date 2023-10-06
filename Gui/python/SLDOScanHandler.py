from PyQt5 import QtCore
from PyQt5.QtCore import *
from Gui.siteSettings import *
from Gui.python.logging_config import logger

import time


class SLDOScanThread(QThread):
    measureSignal = pyqtSignal(object)

    def __init__(self, parent, powersupply=None, measuredevice=None):
        super(SLDOScanThread, self).__init__()
        self.powersupply = powersupply
        self.measuredevice = measuredevice
        self.parent = parent
        self.measureSignal.connect(self.parent.transitMeasurment)
        self.exiting = False

        self.startVal = 0
        self.target = 0
        self.stopVal = 150
        self.stepLength = 5
        self.stepNum = 0
        self.stepTotal = (self.stopVal - self.startVal) / self.stepLength + 1

        # self.Voltage = 1.3
        # self.maxCurr = 0.3
        self.Voltage = defaultSLDOscanVoltage
        self.maxCurr = defaultSLDOscanMaxCurrent
        self.deltaCurr = 0.001
        self.Current = 0.0
        self.status = None

        self.turnOn()

    def turnOn(self):
        self.powersupply.SetRange(1.6)
        self.powersupply.TurnOnLV()

    def abortTest(self):
        print("Aborting test...")
        self.exiting = True

    def run(self):
        while not self.exiting and self.Current < self.maxCurr:
            self.powersupply.SetVoltage(self.Voltage)
            time.sleep(0.1)
            self.powersupply.SetCurrent(self.Current)
            time.sleep(0.1)

            measuredV = self.powersupply.ReadVoltage()
            measuredI = self.powersupply.ReadCurrent()

            measurementStr = {
                "voltage": measuredV,
                "current": measuredI,
                "percentage": self.Current / self.maxCurr * 0.5,
            }
            self.measureSignal.emit(measurementStr)

            if self.Current > 0.1 and self.Current < self.maxCurr:
                self.deltaCurr = 0.1
            else:
                self.deltaCurr = 0.01
            self.Current += self.deltaCurr

        while not self.exiting and self.Current > 0:
            self.powersupply.SetVoltage(self.Voltage)
            time.sleep(0.1)
            self.powersupply.SetCurrent(self.Current)
            time.sleep(0.1)

            measuredV = self.powersupply.ReadVoltage()
            measuredI = self.powersupply.ReadCurrent()

            measurementStr = {
                "voltage": measuredV,
                "current": measuredI,
                "percentage": 1 - self.Current / self.maxCurr * 0.5,
            }
            self.measureSignal.emit(measurementStr)

            if self.Current > 0.1 and self.Current < self.maxCurr:
                self.deltaCurr = 0.1
            else:
                self.deltaCurr = 0.01
            self.Current -= self.deltaCurr

        self.powersupply.Reset()
        measurementStr = {"voltage": -1, "current": -1, "percentage": 1}

        # except Exception as err:
        #    print("IV-Curve test failed")


class SLDOScanHandler(QObject):
    measureSignal = pyqtSignal(str, object)
    stopSignal = pyqtSignal(object)
    finished = pyqtSignal()

    def __init__(self, window, powersupply, measuredevice=None):
        super(SLDOScanHandler, self).__init__()
        self.powersupply = powersupply
        if measuredevice == None:
            self.measuredevice = self.powersupply
        else:
            self.measuredevice = measuredevice
        self.window = window
        # self.stopSignal.connect(self.window.)
        self.measureSignal.connect(self.window.updateMeasurement)

        self.test = SLDOScanThread(
            self, powersupply=self.powersupply, measuredevice=self.measuredevice
        )
        self.test.finished.connect(self.finish)

    def isValid(self):
        return self.powersupply != None

    def SLDOScan(self):
        if not self.isValid():
            return
        self.test.start()

    def transitMeasurment(self, measure):
        self.measureSignal.emit("SLDOScan", measure)

    def finish(self):
        self.finished.emit()

    def stop(self):
        try:
            print("Terminating SLDO Scan test...")
            self.test.abortTest()
        except Exception as err:
            print("Failed to stop the SLDOScan test")
