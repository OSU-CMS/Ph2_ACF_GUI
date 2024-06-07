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
        moduleType=None,
        target_current=10,
        step_size=1.0,
        delay=0.2,
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
        self.exiting = False
        self.moduleType = moduleType
        self.PIN_MAPPINGS = {
            'DEFAULT': self.instruments._adc_board.PIN_MAP,
        }


    def run(self) -> None:
        """
        Run thread that will ramp up the LV while measuring from the multimeter, then ramp down doing the same thing.
        Combine the two list of measurments and return that from the function.

        measure: emitted after VI curve of each pin contains array of MM measurements
        progress: emitted after each cycle of the LV sweep
        """
        # Initialize a list to store the results
        self.labels = []
        # Turn off instruments
        self.instruments.hv_off()
        self.instruments.lv_off()
        #self.instruments._multimeter.set("SYSTEM_MODE","REM")
        self.instruments._adc_board.__enter__()
        
        
        logger.info('turned off the lv and hv')
        for key, value in self.PIN_MAPPINGS[self.moduleType].items():
            if "VDD" in value:
                self.pin_list.append(key)
                print('adding {0} to pin_list'.format(key))
        for pin in self.pin_list:
            # Make label for plot
            self.labels.append(pin)
            # Connect to relay pin
            logger.info('made it inside the pin loop')
            #self.instruments.relay_pin(pin)
            # ramp up LV
            self.instruments.lv_on(current=self.starting_current, voltage=self.max_voltage)
            _, result_ = self.instruments._lv.sweep(
                "CURRENT",
                self.instruments._default_lv_channel,
                self.target_current,
                delay=self.delay,
                step_size=self.step_size,
                measure=True,
                measure_function=self.measureFunction,
                #measure_args={"measured_unit":"SLDO", "cycles": self.integration_cycles},
                log_function=logger.info,
                #execute_each_step=self.getProgress,
                break_monitoring=self.breakTest
            )
            
            currentList = [res[1][0] for res in result_]
            lvVoltageList = [res[1][1] for res in result_]
            adcVoltageList = [res[2][pin] for res in result_]
            
            print(f"Currents: {currentList}\nPower Supply Voltages: {lvVoltageList}\nBoard Voltages: {adcVoltageList}")
            
            result_up = np.array([currentList, lvVoltageList, adcVoltageList])
            # Ramp down LV and measure on the way down
            _, result_ = self.instruments._lv.sweep(
                "CURRENT",
                self.instruments._default_lv_channel,
                self.starting_current,
                delay=self.delay,
                step_size=self.step_size,
                measure=True,
                measure_function=self.measureFunction,
                #measure_args={"measured_unit":"SLDO", "cycles": self.integration_cycles},
                log_function=logger.info,
                #execute_each_step=self.getProgress,
                break_monitoring= self.breakTest
            )
            
            currentList = [res[1][0] for res in result_]
            lvVoltageList = [res[1][1] for res in result_]
            adcVoltageList = [res[2][pin] for res in result_]
            
            result_down = np.array([currentList, lvVoltageList, adcVoltageList])
            total_result = np.concatenate((result_up, result_down), axis=0)
            #0:up current, 1:up lv voltage, 2: up adc voltage 3: down current, 4: down lv voltage, 5: down adc voltage
            print('total result is {0}'.format(total_result))
            self.instruments.lv_off()
            
        # Emit a signal that passees the list of results to the SLDOCurveHandler.  
            self.getProgress()
            self.measure.emit(total_result, self.PIN_MAPPINGS[self.moduleType][pin]) 
        #All pins have been scanned so we emit the finished signal
        self.finishedSignal.emit()

    def getProgress(self):
        self.percentStep = abs(100/len(self.pin_list))
        #self.percentStep = abs(100*self.step_size/(2*len(self.pin_list)*self.target_current)) #This assumes that the starting_current is zero.
        self.progressSignal.emit("SLDOScan", self.percentStep)

    def measureFunction(self, settings=None, no_lock=True):
        #not sure if settings param is needed, the call to the function in instrument.py includes **measure_args as a param
        module_output_current = self.instruments._lv.query_channel("OUTPUT_CURRENT", channel=self.instruments._default_lv_channel, no_lock=True)
        module_output_voltage = self.instruments._lv.query_channel("OUTPUT_VOLTAGE", channel=self.instruments._default_lv_channel, no_lock=True)
        pin_voltages, temperatures = self.instruments._adc_board.query_adc(no_lock=no_lock)
        return (module_output_current, module_output_voltage), pin_voltages
    
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

    def __init__(self, instrument_cluster, moduleType, end_current, voltage_limit):
        super(SLDOCurveHandler, self).__init__()
        self.instruments = instrument_cluster
        self.end_current = end_current
        self.voltage_limit = voltage_limit
        self.test = SLDOCurveWorker(self.instruments, moduleType=moduleType, target_current=self.end_current, max_voltage=self.voltage_limit)
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
