"""
Class to perform the SLDO curve scanning
"""

from PyQt5.QtCore import QThread, pyqtSignal, QObject
from Gui.python.logging_config import logger
import numpy as np
import matplotlib.pyplot as plt
import time

class SLDOCurveWorker(QThread):
    #finished = pyqtSignal()
    progress = pyqtSignal()
    measure = pyqtSignal(np.ndarray, str)

    def __init__(
        self,
        instrument_cluster=None,
        target_current=10,
        step_size=5,
        delay=1,
        integration_cycles=1,
        starting_current=1,
        pin_list=[],
    ):
        super().__init__()
        self.instruments = instrument_cluster
        self.delay = delay
        self.target_current = target_current
        self.step_size = step_size
        self.integration_cycles = integration_cycles
        self.starting_current = starting_current
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
        self.instruments.lv_off(1)
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
            self.instruments.lv_on(1)
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
                #execute_each_step=self.updateProgress,
                #execute_each_step=self.progress.emit,
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
                #execute_each_step=self.updateProgress,
                #execute_each_step=self.progress.emit,
                break_monitoring= self.breakTest
            )
            result_down = np.array(result_)
            total_result = np.concatenate((result_up, result_down), axis=0)
            print('total result is {0}'.format(total_result))
            self.instruments.lv_off(1)
            
           # self.SLDO_measurements["Voltage"] = total_result_stacked[:,1]
           # self.SLDO_measurements["Current"] = total_result_stacked[:,2]
           # self.SLDO_measurements["{0}_voltage".format(pin)] = total_result_stacked[:,3]
           # print("sldo measurement is {0}".format(self.SLDO_measurements))
            # At this point in the loop, we have the results for one of the pins. 
            # Should maybe make a plot here and store it in a list of plots?
        # Emit a signal that passees the list of results to the SLDOCurveHandler.    
            self.measure.emit(total_result, pin)

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
    finished = pyqtSignal(str, dict)
    makeplotSignal = pyqtSignal(np.ndarray, str)
    #measureSignal = pyqtSignal(str, object)

    def __init__(self, instrument_cluster):
        super(SLDOCurveHandler, self).__init__()
        self.instruments = instrument_cluster
        self.test = SLDOCurveWorker(self.instruments)
        self.test.measure.connect(self.makePlots)

    #This should take the list of results and make plots.  I think we should maybe move this to the TestHandler.
    def makePlots(self, total_result, pin):
        self.makeplotSignal.emit(total_result, pin)
        #The pin is passed here, so we can use that as the key in the chipmap dict from settings.py
        #total_result_stacked = np.vstack(total_result)
        #plt.figure()
        #plt.plot(total_result_stacked[:,2],total_result_stacked[:,1],'-x',label="module input voltage")
        #plt.plot(total_result_stacked[:,2],total_result_stacked[:,3],'-x',label=pin)
        #plt.grid(True)
        #plt.xlabel("Current (A)")
        #plt.ylabel("Voltage (V)")
        #plt.legend()
        #plt.savefig("SLDO_{0}.png".format(pin)) #FIXME need to add the path for the data directory here.
        #plt.show()
        #time.sleep(5)
        #all_results = result_list[0][:,1]
        #for res in result_list:
        #    all_results = np.vstack([all_results, res[:,3]])
        

        pass

    def SLDOScan(self):
        if not self.instruments:
            return
        self.test.start()

    def finish(self):
        self.instruments.hv_off()
        self.instruments.lv_off()
        self.finished.emit(test, measure)

    def stop(self):
        try:
            self.test.abortTest()
            self.instruments.hv_off()
            self.instruments.lv_off()
            self.test.terminate()
        except Exception as err:
            logger.error(f"Failed to stop the SLDO test due to error {err}")
