from PyQt5 import QtCore
from PyQt5.QtCore import *

import time


class IVCurveThread(QThread):
    measureSignal = pyqtSignal(str, object)

    def __init__(self, parent, instrument_cluster=None):
        super(IVCurveThread, self).__init__()
        self.instruments = instrument_cluster
        self.parent = parent
        self.measureSignal.connect(self.parent.transitMeasurment)
        self.exiting = False
        self.setTerminationEnabled(True)

        self.startVal = 0
        self.target = 0
        self.stopVal = -80  # FIXME set this back to -80 after testing
        self.stepLength = -2
        self.stepNum = 0
        self.stepTotal = (self.stopVal - self.startVal) / self.stepLength + 1
        self.turnOn()

    def turnOn(self):
        self.instruments.hv_off()
        self.instruments.hv_on(voltage=0, delay=0.5, step_size = 10)
        self.instruments.hv_compliance_current(0.00001)

    def abortTest(self):
        print("Aborting test...")
        self.terminate()

    def run(self):
        while not self.exiting:
            try:
                print("About to measure")
                _, measurements = self.instruments.hv_on(lv_channel = 1, voltage = self.target, step_size=-2,measure=True)
                print(measurements)
                measurements = ((value[1], value[3]) for value in measurements)
                self.measureSignal.emit("IVCurve", measurements)
            except Exception as e:
                print("IV Curve scan failed with {}".format(e))




class IVCurveHandler(QObject):
    measureSignal = pyqtSignal(str, object)
    stopSignal = pyqtSignal(object)
    finished = pyqtSignal()

    def __init__(self, window, instrument_cluster):
        super(IVCurveHandler, self).__init__()
        self.instruments = instrument_cluster
        self.window = window

        self.test = IVCurveThread(self, instrument_cluster=self.instruments)
        self.test.measureSignal.connect(self.window.updateMeasurement)
        self.test.finished.connect(self.finish)

    def isValid(self):
        return self.instruments != None

    def IVCurve(self):
        if not self.isValid():
            return
        self.test.start()

    def transitMeasurment(self, measure):
        self.measureSignal.emit("IVCurve", measure)

    def finish(self):
        self.instruments.hv_off()
        self.finished.emit()

    def stop(self):
        try:
            print("Terminating I-V Curve scanning...")
            self.test.abortTest()
            self.instruments.hv_off()
        except Exception as err:
            print(f"Failed to stop the IV test due to error {err}")
