import Gui.siteSettings as site_settings
from PyQt5.QtCore import QThread, QObject, pyqtSignal

from icicle.icicle.instrument_cluster import DummyInstrument

import numpy as np
import traceback

from Gui.python.logging_config import get_logger
logger = get_logger(__name__)

#^^^^^^ Coppied from IVCurveHandler

class ADCHandlerThread (QThread):
    measureSignal = pyqtSignal(str, object)
    progressSignal = pyqtSignal(str, float)
    def __init__(self, parent, testName, instrument_cluster=None, execute_each_step=lambda: None):
        super(ADCHandlerThread, self).__init__()
        self.instruments = instrument_cluster
        self.powergroup = None
        for group_key, group in self.instruments.powering_groups.items():
            logger.info(f"Group key: {group_key}, Group: {group}, Modules: {group.modulenames}")
            self.powergroup = group
        self.measurements = {}
        for name in self.powergroup.modulenames:
            self.measurements[name] = []
        self.parent = parent
        self.measureSignal.connect(self.parent.transitMeasurment)
        self.progressSignal.connect(
            self.parent.transmitProgress
        )
        self.measure_lv = True
        self.exiting = False
        self.setTerminationEnabled(True)
        self.startVal = 0
        
    def __del__(self): #ensures that it will stop processing before the worker object is destroyed
        self.exiting = True

    def breakTest(self):
        if self.exiting:
            return True
        return False

    def abortTest(self):
        self.exiting = True

    def run(self):
        self.device_settings = site_settings.icicle_instrument_setup
        measurementList = {}
        self.instruments.open()
        while (self.measure_lv):
            for i, module in enumerate (self.instruments._module_dict.values()):
                self.device_settings[i] = module
                print(f"the device settings are \n {self.device_settings}")
                #print(self.instruments._module_dict)
                module_rep = self.instruments._module_dict
                #print(f"{module_rep[str(i)]}")
                sensev = module_rep[f"{i}"]["lv"].measure_voltage.value
                print(f"Module {i} at {sensev} V.")

            if self.exiting:
                logger.info("ADC Calibration was aborted by user.")
                return

            for channel in self.measurements.keys():
                measurementStr = {
                    "voltage": [value[0] for value in self.measurements[channel]],
                    "current": [value[2] for value in self.measurements[channel]], #Need to add module name here
                }
                print(f"Measurements are the following: \n {self.measurements[channel]}")
                measurementList[channel] = measurementStr

                print(f"Voltages for channel {0}: ".format(channel), measurementStr["voltage"])
                print(f"Currents for channel {0}: ".format(channel), measurementStr["current"])
                #add wait 1000ms
        self.measureSignal.emit("ADC_CALIB", measurementList)


class ADCHandlerObject(QObject):
    measureSignal = pyqtSignal(str, object)
    stopSignal = pyqtSignal(object)
    finished = pyqtSignal(str, dict)
    startSignal = pyqtSignal()
    progressSignal = pyqtSignal(str, float)

    def __init__(self, testName, instrument_cluster, nextTest, execute_each_step):
        super(ADCHandlerObject, self).__init__()
        self.instruments = instrument_cluster
        self.execute_each_step = execute_each_step
        self.nextTest = nextTest

        assert self.instruments is not None, logger.debug(
            "Error instantiating instrument cluster"
        )

        self.test = ADCHandlerThread(
            self,
            testName,
            instrument_cluster=self.instruments,
            execute_each_step=self.execute_each_step,
        )
        self.test.progressSignal.connect(self.transmitProgress)
        self.test.measureSignal.connect(self.finish)

    def isValid(self):
        return self.instruments is not None
    
    def ADC_CALIB(self):
        if not self.isValid():
            return
        self.startSignal.emit()
        self.test.start()

    def transitMeasurment(self, measure):
        self.measureSignal.emit("ADC_CALIB", measure)

    def transmitProgress(self, measurementType, percentStep):
        self.progressSignal.emit(measurementType, percentStep)

    def finish(self, test: str, measure: dict):
        starting_voltages = [
            np.abs(getattr(module["lv"], "voltage"))
            for module in self.instruments._module_dict.values()
        ]
        ## Will Set voltage to default unless SLDO is the next test
        if (self.nextTest is not None) and ("SLDO" not in self.nextTest): 
            self.instruments.lv_set(voltage = site_settings.icicle_instrument_setup[
                    "instrument_dict"]["lv"]["default_voltage"], delay = 0.3, step_size = 10,
                    execute_each_step = lambda: self.execute_each_step([site_settings.icicle_instrument_setup[
                    "instrument_dict"]["lv"]["default_voltage"]] * len(self.instruments._module_dict.values()))
                )
        else:    
            self.instruments.lv_off(
                execute_each_step=lambda: self.execute_each_step(starting_voltages)
            )            
        self.finished.emit(test, measure)

    def stop(self):
        try:
            self.test.abortTest()
            starting_voltages = [
                np.abs(getattr(module["lv"], "voltage"))
                for module in self.instruments._module_dict.values()
            ]
            self.instruments.lv_off(
                no_lock=True,
                execute_each_step=lambda: self.execute_each_step(starting_voltages),
            )
            self.test.terminate()
        except Exception as err:
            print(f"Failed to stop the ADC test due to error: {err}")
            print(traceback.format_exc())