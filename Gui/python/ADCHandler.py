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
        self.progressSignal.connect(self.parent.transmitProgress)
        self.measure_lv = True
        self.exiting = False
        self.setTerminationEnabled(True)
        self.execute_each_step = execute_each_step
        self.ProgressValue = 0
        self.total_steps = 25
        self.timestep = 5000 #length between readings in ms
        self.stopVal = self.timestep * self.total_steps
        self.steps_done = 0
        self.turnOn()

    def turnOn(self):
        self.start_time = time.time()
        starting_voltages = [
            np.abs(getattr(module["lv"], "voltage"))
            for module in self.instruments._module_dict.values()
        ]
        logger.info(f"ADC_CALIB starting; LV already on at {starting_voltages} V")

    def __del__(self): #ensures that it will stop processing before the worker object is destroyed
        print("setting self.exiting true, del")
        self.exiting = True

    def breakTest(self):
        if self.exiting:
            return True
        return False

    def abortTest(self):
        print("trying to end test")
        self.exiting = True

    def getProgress(self):
        self.percentStep = abs(100 * self.steps_done / (self.total_steps))
        logger.info(self.percentStep)
        # self.ProgressValue += self.percentStep
        # logger.info(self.ProgressValue)
        self.steps_done += 1
        logger.info(self.steps_done)
        self.progressValue = self.percentStep
        self.progressSignal.emit("ADC_CALIB",self.percentStep) 
        
        #FIXME Percent doesnt update for the progress bar
        
        # self.steps_done = 0

        # for _ in range(self.total_steps):
        #     if self.exiting:
        #         break
        #     steps_done += 1
            
        #     remaining_steps = self.total_steps - steps_done
        #     if remaining_steps > 0:
        #         self.ProgressValue += remaining_steps
        #         percent = 100 * self.ProgressValue / (self.remaining_steps)
        #         self.progressSignal.emit("ADC_CALIB", percent)
        #     break
        
        
    def run(self):
        self.device_settings = site_settings.icicle_instrument_setup
        measurementList = {}
        self.instruments.open()
        datapts = 0
        while not self.exiting:
            for name, module in zip(self.powergroup.modulenames, self.powergroup.modules):
                #name is the module identifier/key from the powering group,
                self.device_settings[name] = module

                #print("module in start of for loop is {0}".format(module))
                #print(f"the device settings are:\n{self.device_settings}\n")

                sensev = module["lv"].measure_voltage.value
                module_label = module.get("name", name)
                elapsed = time.time() - self.start_time

                print(f"Module {name} ({module_label}) at {sensev} V, t={elapsed:.1f}s.\n")

                #print(f"measurements {self.measurements}")

                self.measurements[name].append([sensev, module_label, elapsed])

            if self.exiting or datapts >= self.total_steps:
                logger.info("ADC Calibration was aborted by user.")
                break

            for channel in self.measurements.keys():

                print(f"measurement channels are {self.measurements[channel]}")
                measurementStr = {
                    "voltage": [value[0] for value in self.measurements[channel]],
                    "module": [value[1] for value in self.measurements[channel]],
                    "time": [value[2] for value in self.measurements[channel]],
                }
                #print(f"Measurements are the following: \n {self.measurements[channel]}")
                
                measurementList[channel] = measurementStr

                print("Modules for channel {0}: ".format(channel), measurementStr["module"])
                print("Voltages for channel {0}: ".format(channel), measurementStr["voltage"])

            self.getProgress()
            QThread.msleep(self.timestep) #Time between printed readouts
            datapts += 1

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
        print("finish called in adchandler")
        starting_voltages = [
            np.abs(getattr(module["lv"], "voltage"))
            for module in self.instruments._module_dict.values()
        ]
        print("emit finish signal")
        self.finished.emit(test, measure)
        ## Will Set voltage to default unless SLDO is the next test
        if (self.nextTest is not None) and ("SLDO" not in self.nextTest): 
            self.instruments.lv_set(voltage = site_settings.icicle_instrument_setup[
                    "instrument_dict"]["lv"]["default_voltage"], delay = 0.3, step_size = 10,
                    execute_each_step = lambda: self.execute_each_step([site_settings.icicle_instrument_setup[
                    "instrument_dict"]["lv"]["default_voltage"]] * len(self.instruments._module_dict.values()))
                )
        else:    
            self.instruments.lv_off()        

    def stop(self):
        try:
            self.test.abortTest()
            # starting_voltages = [
            #     np.abs(getattr(module["lv"], "voltage"))
            #     for module in self.instruments._module_dict.values()
            # ]
            self.test.terminate()
            self.instruments.lv_off()
        except Exception as err:
            print(f"Failed to stop the ADC test due to error: {err}")
            print(traceback.format_exc())