from PyQt5 import QtCore
from PyQt5.QtCore import *

import time

class IVCurveThread(QThread):
    measureSignal = pyqtSignal(object)

    def __init__(self,parent,powersupply = None):
        super(IVCurveThread, self).__init__()
        self.powersupply = powersupply
        self.parent = parent
        self.measureSignal.connect(self.parent.transitMeasurment)
        self.exiting = False

        self.startVal = 0
        self.target = 0
        self.stopVal = 150
        self.stepLength = 5
        self.stepNum = 0
        self.stepTotal = (self.stopVal-self.startVal)/self.stepLength+1
        self.turnOn()

    def turnOn(self):
        self.powersupply.TurnOnHV()
        self.powersupply.SetHVRange(200)
        self.powersupply.SetHVComplianceLimit(0.1)

    def abortTest(self):
        print("Aborting test...")
        self.exiting = True

    def run(self):
        while not self.exiting and self.target <= self.stopVal:
            try:
                self.powersupply.SetHVVoltage(self.target)
                time.sleep(0.5)
                voltage = self.powersupply.ReadVoltage()
                current = self.powersupply.ReadCurrent()
                #MAX_TRIES = 10
                #N_TRIES = 1
                #while abs(target-float(voltage))/abs(target+0.01) > 0.05 and  N_TRIES < MAX_TRIES:
                #    N_TRIES += 1
                #    time.sleep(0.5)
                #    voltage = self.powersupply.ReadVoltage()
                #    current = self.powersupply.ReadCurrent()
                #    print("voltage:",voltage, " current:",current)
                self.stepNum += 1
                measurementStr = {"voltage":voltage,"current":current,"percentage":self.stepNum/self.stepTotal}
                print(measurementStr)
                if voltage == None or current == None:
                    self.stepNum -= 1
                    self.target = self.startVal + self.stepLength * self.stepNum
                    continue
                self.measureSignal.emit(measurementStr)
                self.target = self.startVal + self.stepLength * self.stepNum
            except Exception as err:
                print("IV Curve scan failed with {}".format(err))
            
        #except Exception as err:
        #    print("IV-Curve test failed")

class IVCurveHandler(QObject):
    measureSignal = pyqtSignal(str,object)
    stopSignal = pyqtSignal(object)
    finished = pyqtSignal()
    def __init__(self,window,powersupply):
        super(IVCurveHandler,self).__init__()
        self.powersupply = powersupply
        self.window = window
        #self.stopSignal.connect(self.window.)
        self.measureSignal.connect(self.window.updateMeasurement)

        self.test = IVCurveThread(self, powersupply= self.powersupply)
        self.test.finished.connect(self.finish)
    
    def isValid(self):
        return self.powersupply != None

    def IVCurve(self):
        if not self.isValid():
            return
        self.test.start()

    def transitMeasurment(self, measure):
        self.measureSignal.emit("IVCurve",measure)

    def finish(self):
        self.finished.emit()

    def stop(self):
        try:
            print("Terminating I-V Curve scanning...")
            self.test.abortTest()
        except Exception as err:
            print("Fialed to stop the IV test")


