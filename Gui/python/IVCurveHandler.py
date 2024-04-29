from PyQt5 import QtCore
from PyQt5.QtCore import *

import time
import numpy
from Gui.python.logging_config import logger



class IVCurveThread(QThread):
    measureSignal = pyqtSignal(str, object)
    progressSignal = pyqtSignal(str, float)

    def __init__(self, parent, instrument_cluster=None):
        super(IVCurveThread, self).__init__()
        self.instruments = instrument_cluster
        self.parent = parent
        self.measureSignal.connect(self.parent.transitMeasurment)
        self.progressSignal.connect(self.parent.transmitProgress) #FIXME add slot function
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
        self.instruments.hv_on(voltage=0, delay=0.5, step_size=10, no_lock=True)
        self.instruments.hv_compliance_current(0.00001)

    # Used to break out of hv_on correctly
    def breakTest(self):
        if self.exiting:
            return True
        return False

    def getProgress(self):
        self.percentStep = abs(100*self.stepLength/self.stopVal)
        self.progressSignal.emit("IVCurve", self.percentStep)

    def abortTest(self):
        self.exiting = True

    def run(self):
        try:
            _, measurements = self.instruments.hv_on(
                lv_channel=1,
                voltage=self.stopVal,
                step_size=self.stepLength,
                delay=0.2,
                measure=True,
                break_monitoring=self.breakTest,
                execute_each_step=self.getProgress,
            )
            measurementStr = {
                "voltage": [value[1] for value in measurements],
                "current": [value[2] for value in measurements],
            }

            print("Voltages: ", measurementStr["voltage"])
            print("Currents: ", measurementStr["current"])
            self.measureSignal.emit("IVCurve", measurementStr)
        except Exception as e:
            print("IV Curve scan failed with {}".format(e))


class IVCurveHandler(QObject):
    measureSignal = pyqtSignal(str, object)
    stopSignal = pyqtSignal(object)
    finished = pyqtSignal(str, dict)
    progressSignal = pyqtSignal(str, float)

    def __init__(self, instrument_cluster):
        super(IVCurveHandler, self).__init__()
        self.instruments = instrument_cluster

        assert self.instrument is not None, logger.debug("Error instantiating instrument cluster")

        self.test = IVCurveThread(self, instrument_cluster=self.instruments)
        self.test.progressSignal.connect(self.transmitProgress)
        self.test.measureSignal.connect(self.finish)

    def isValid(self):
        return self.instruments != None

    def IVCurve(self):
        if not self.isValid():
            return
        self.test.start()

    def transitMeasurment(self, measure):
        self.measureSignal.emit("IVCurve", measure)

    def transmitProgress(self, measurementType, percentStep):
        self.progressSignal.emit(measurementType, percentStep)

    def finish(self, test: str, measure: dict):
        self.instruments.hv_off()
        self.finished.emit(test, measure)

    def stop(self):
        try:
            self.test.abortTest()
            self.instruments.hv_off(no_lock=True)
            self.test.terminate()
        except Exception as err:
            print(f"Failed to stop the IV test due to error {err}")
