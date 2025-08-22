import Gui.siteSettings as site_settings
from PyQt5.QtCore import QThread, QObject, pyqtSignal

from icicle.icicle.instrument_cluster import DummyInstrument

import numpy as np
import traceback

from Gui.python.logging_config import get_logger
logger = get_logger(__name__)


class IVCurveThread(QThread):
    measureSignal = pyqtSignal(str, object)
    progressSignal = pyqtSignal(str, float)

    def __init__(
        self, parent, testName, instrument_cluster=None, execute_each_step=lambda: None
    ):
        super(IVCurveThread, self).__init__()
        self.instruments = instrument_cluster
        self.powergroup = None
        for group_key, group in self.instruments.powering_groups.items():
            self.powergroup = group
        self.measurements = {}
        for name in self.powergroup.modulenames:
            self.measurements[name] = []
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
        ### This loop should create a list of list of measurements for each "channel" in the json file.
        ### The "channel" number is the key in the "measurements" dictionary. 
        ### Each "channel" number is associated with a module.
        for name, module in zip(self.powergroup.modulenames, self.powergroup.modules):
            
            source_voltage = self.powergroup.source_channel.measure_voltage.value
            source_current = self.powergroup.source_channel.measure_current.value
            if not isinstance(module["hb"], DummyInstrument):
                module_current = -1*module["hb"]._measure_current.value
                module_channel = module["hb"]._hvbox_channel
            else:
                module_current = source_current
                module_channel = name
            result = [source_voltage, source_current, module_current, module_channel]
            self.measurements[name].append(result)

    def abortTest(self):
        self.exiting = True

    def run(self):
        try:
            starting_voltages = [
                np.abs(getattr(module["hv"], "voltage"))
                for module in self.instruments._module_dict.values()
            ]
            self.instruments.hv_on(
                execute_each_step=lambda: self.execute_each_step(starting_voltages)
            )

            self.powergroup.ramp_hv(
                voltage=self.stopVal,
                delay=0.2,
                step_size=self.stepLength,
                execute_each_step=self.getProgress,
                break_loop= lambda: self.exiting,
            )

##### Replacing the following with the new hv_on function
            #_, measurements = self.instruments.hv_on(
            #    voltage=self.stopVal,
            #    step_size=self.stepLength,
            #    delay=0.2,
            #    measure=True,
            #    execute_each_step=self.getProgress,
            #    break_loop= lambda: self.exiting,
            #)[0]

#####  End of replacement block

            if self.exiting:
                print("IV Curve scan was aborted by user.")
                return
                        
            # The physics test can be stopped by pressing enter
            measurementStr = {
                "voltage": [value[0] for value in self.measurements['0']],
                "current": [value[2] for value in self.measurements['0']],
            }

            print("Voltages: ", measurementStr["voltage"])
            print("Currents: ", measurementStr["current"])
            self.measureSignal.emit("IVCurve", measurementStr)
        except Exception as e:
            print(f"IV Curve scan failed with error: {e}")
            print(traceback.format_exc())

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
            print(f"Failed to stop the IV test due to error: {err}")
            print(traceback.format_exc())
