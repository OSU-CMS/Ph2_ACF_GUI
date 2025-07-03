from PyQt5.QtCore import QThread, QObject, pyqtSignal

import numpy as np
from Gui.python.logging_config import logger
import Gui.siteSettings as site_settings


class IVCurveThread(QThread):
    measureSignal = pyqtSignal(str, object)
    progressSignal = pyqtSignal(str, float)

    def __init__(
        self, parent, testName, instrument_cluster=None, execute_each_step=lambda: None
    ):
        super(IVCurveThread, self).__init__()
        self.instruments = instrument_cluster
        self.parent = parent
        self.measureSignal.connect(self.parent.transitMeasurment)
        self.progressSignal.connect(
            self.parent.transmitProgress
        )  # FIXME add slot function
        self.exiting = False
        self.setTerminationEnabled(True)
        self.execute_each_step = execute_each_step

        self.startVal = 0
        self.target = 0
        # Making sure IVcurve peak is a negative voltage
        if site_settings.IVcurve_range[testName] < 0:
            self.stopVal = site_settings.IVcurve_range[testName]
            print("IVcurve range: ", self.stopVal)
        else:
            self.stopVal = -80
        self.stepLength = 2
        self.stepNum = 0
        self.stepTotal = (self.stopVal - self.startVal) / self.stepLength + 1
        self.turnOn()

    def turnOn(self):
        starting_voltages = [
            np.abs(getattr(module["hv"], "voltage"))
            for module in self.instruments._module_dict.values()
        ]
        self.instruments.hv_off(
            execute_each_step=lambda: self.execute_each_step(starting_voltages)
        )
        self.instruments.hv_on(voltage=0, delay=0.5, step_size=10, no_lock=True)

    # Used to break out of hv_on correctly
    def breakTest(self):
        if self.exiting:
            return True
        return False

    def getProgress(self):
        self.percentStep = abs(100 * self.stepLength / self.stopVal)
        self.progressSignal.emit("IVCurve", self.percentStep)

    def abortTest(self):
        self.exiting = True

    def run(self):
        try:
            starting_voltages = [
                np.abs(getattr(module["hv"], "voltage"))
                for module in self.instruments._module_dict.values()
            ]
            self.instruments.hv_off(
                execute_each_step=lambda: self.execute_each_step(starting_voltages)
            )

            _, measurements = self.instruments.hv_on(
                voltage=self.stopVal,
                step_size=self.stepLength,
                delay=0.2,
                measure=True,
                execute_each_step=self.getProgress,
            )[0]

            # The physics test can be stopped by pressing enter

            measurementStr = {
                "voltage": [value[4] for value in measurements],
                "current": [value[5] for value in measurements],
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
    startSignal = pyqtSignal()

    def __init__(self, testName, instrument_cluster, nextTest, execute_each_step):
        super(IVCurveHandler, self).__init__()
        self.instruments = instrument_cluster
        self.execute_each_step = execute_each_step
        self.nextTest = nextTest

        assert self.instruments is not None, logger.debug(
            "Error instantiating instrument cluster"
        )

        self.test = IVCurveThread(
            self,
            testName,
            instrument_cluster=self.instruments,
            execute_each_step=self.execute_each_step,
        )
        self.test.progressSignal.connect(self.transmitProgress)
        self.test.measureSignal.connect(self.finish)

    def isValid(self):
        return self.instruments is not None

    def IVCurve(self):
        if not self.isValid():
            return
        self.test.start()
        self.startSignal.emit()

    def transitMeasurment(self, measure):
        self.measureSignal.emit("IVCurve", measure)

    def transmitProgress(self, measurementType, percentStep):
        self.progressSignal.emit(measurementType, percentStep)

    def finish(self, test: str, measure: dict):
        starting_voltages = [
            np.abs(getattr(module["hv"], "voltage"))
            for module in self.instruments._module_dict.values()
        ]
        ## Will Set voltage to default unless SLDO is the next test
        if (self.nextTest is not None) and ("SLDO" not in self.nextTest): 
            self.instruments.hv_set(voltage = site_settings.icicle_instrument_setup[
                    "instrument_dict"]["hv"]["default_voltage"], delay = 0.3, step_size = 10,
                    execute_each_step = lambda: self.execute_each_step([site_settings.icicle_instrument_setup[
                    "instrument_dict"]["hv"]["default_voltage"]] * len(self.instruments._module_dict.values()))
                )
        else:    
            self.instruments.hv_off(
                execute_each_step=lambda: self.execute_each_step(starting_voltages)
            )    
        self.finished.emit(test, measure)

    def stop(self):
        try:
            self.test.abortTest()
            starting_voltages = [
                np.abs(getattr(module["hv"], "voltage"))
                for module in self.instruments._module_dict.values()
            ]
            self.instruments.hv_off(
                no_lock=True,
                execute_each_step=lambda: self.execute_each_step(starting_voltages),
            )
            self.test.terminate()
        except Exception as err:
            print(f"Failed to stop the IV test due to error {err}")
