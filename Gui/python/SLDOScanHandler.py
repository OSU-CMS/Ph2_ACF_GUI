"""
Class to perform the SLDO curve scanning
"""

from PyQt5.QtCore import QThread, pyqtSignal, QObject
from Gui.python.logging_config import logger
import numpy as np
import matplotlib.pyplot as plt
import time

class SLDOCurveWorker(QThread):
    finishedSignal = pyqtSignal()
    progressSignal = pyqtSignal(str, float)
    measure = pyqtSignal(np.ndarray, str)


    def __init__(
        self,
        instrument_cluster=None,
        target_current=10,
        step_size=0.5,
        delay=1,
        integration_cycles=1,
        starting_current=1,
        max_voltage=1.8,
        pin_list=[],
    ):
        super().__init__()
        self.instruments = instrument_cluster
        self.delay = delay
        self.target_current = target_current
        self.step_size = step_size
        self.integration_cycles = integration_cycles
        self.starting_current = starting_current
        self.max_voltage = max_voltage
        self.pin_list = pin_list
        self.SLDO_measurements = {}
        self.exiting = False


    def run(self) -> None:
        """
        Run thread that will ramp up the LV while measuring from the multimeter, then ramp down doing the same thing.
        Combine the two list of measurments and return that from the function.

        measure: emitted after VI curve of each pin contains array of MM measurements
        progress: emitted after each cycle of the LV sweep
        """
        # Initialize a list to store the results
        self.result_list = []
        self.labels = []
        # Turn off instruments
        self.instruments.hv_off()
        self.instruments.lv_off()
        self.instruments._multimeter.set("SYSTEM_MODE","REM")
        logger.info('turned off the lv and hv')
        for key in self.instruments._relay_board.PIN_MAP.keys():
            if "VDD" in key:
                self.pin_list.append(key)
                print('adding {0} to pin_list'.format(key))
        for pin in self.pin_list:
            # Make label for plot
            self.labels.append(pin)
            # Connect to relay pin
            logger.info('made it inside the pin loop')
            self.instruments.relay_pin(pin)
            # ramp up LV
            self.instruments.lv_on(current=self.starting_current, voltage=self.max_voltage)
            _, result_ = self.instruments._lv.sweep(
                "CURRENT",
                self.instruments._default_lv_channel,
                self.target_current,
                delay=self.delay,
                step_size=self.step_size,
                measure=True,
                measure_function=self.instruments.multimeter_measure,
                measure_args={"measured_unit":"SLDO", "cycles": self.integration_cycles},
                log_function=logger.info,
                #execute_each_step=self.getProgress,
                break_monitoring=self.breakTest
            )

            result_up = np.array(result_)
            # Ramp down LV and measure on the way down
            _, result_ = self.instruments._lv.sweep(
                "CURRENT",
                self.instruments._default_lv_channel,
                self.starting_current,
                delay=self.delay,
                step_size=self.step_size,
                measure=True,
                measure_function=self.instruments.multimeter_measure,
                measure_args={"measured_unit":"SLDO", "cycles": self.integration_cycles},
                log_function=logger.info,
                #execute_each_step=self.getProgress,
                break_monitoring= self.breakTest
            )
            result_down = np.array(result_)
            total_result = np.concatenate((result_up, result_down), axis=0)
            print('total result is {0}'.format(total_result))
            self.instruments.lv_off()
            
        # Emit a signal that passees the list of results to the SLDOCurveHandler.  
            self.getProgress()  
            self.measure.emit(total_result, pin)
        #All pins have been scanned so we emit the finished signal
        self.finishedSignal.emit()

    def getProgress(self):
        self.percentStep = abs(100/len(self.pin_list))
        #self.percentStep = abs(100*self.step_size/(2*len(self.pin_list)*self.target_current)) #This assumes that the starting_current is zero.
        self.progressSignal.emit("SLDOScan", self.percentStep)

    def updateProgress(self):
        #lv_voltage_measurement, lv_current_measurement = self.instruments._lv.measure()
        #self.lv_voltage.append(lv_voltage_measurement)
        #self.lv_current.append(lv_current_measurement)
        pass



    def breakTest(self):
        """ Use to kill instrument sweep. """
        if self.exiting:
            return True
        return False

    def abortTest(self):
        self.exiting = True

class SLDOCurveHandler(QObject):
    finishedSignal = pyqtSignal()
    makeplotSignal = pyqtSignal(np.ndarray, str)
    progressSignal = pyqtSignal(str, float)
    #measureSignal = pyqtSignal(str, object)

    def __init__(self, instrument_cluster, end_current, voltage_limit):
        super(SLDOCurveHandler, self).__init__()
        self.instruments = instrument_cluster
        self.end_current = end_current
        self.voltage_limit = voltage_limit
        self.test = SLDOCurveWorker(self.instruments, target_current=self.end_current, max_voltage=self.voltage_limit)
        self.test.measure.connect(self.makePlots)
        self.test.progressSignal.connect(self.transmitProgress)
        self.test.finishedSignal.connect(self.finish)

    #This should take the list of results and make plots.  I think we should maybe move this to the TestHandler.
    def makePlots(self, total_result, pin):
        self.makeplotSignal.emit(total_result, pin)


    def SLDOScan(self):
        if not self.instruments:
            return
        self.test.start()

    def transmitProgress(self, measurementType, percentStep):
        self.progressSignal.emit(measurementType, percentStep)

    def finish(self):
        self.instruments.hv_off()
        self.instruments.lv_off()
        self.finishedSignal.emit()

    def stop(self):
        try:
            self.test.abortTest()
            self.instruments.hv_off()
            self.instruments.lv_off()
            self.test.terminate()
        except Exception as err:
            logger.error(f"Failed to stop the SLDO test due to error {err}")
