"""
Class to perform the SLDO curve scanning
"""

from PyQt5.QtCore import QThread, pyqtSignal
from Gui.python.logging_config import logger
import numpy as np


class SLDOCurveWorker(QThread):
    finished = pyqtSignal()
    progress = pyqtSignal()
    measure = pyqtSignal(np.array)

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
        self.instrument_cluster = instrument_cluster
        self.delay = delay
        self.target_current = target_current
        self.step_size = step_size
        self.integration_cycles = integration_cycles
        self.starting_current = starting_current
        self.pin_list = pin_list
        self.exiting = False

    def run(self) -> None:
        """
        Run thread that will ramp up the LV while measuring from the multimeter, then ramp down doing the same thing.
        Combine the two list of measurments and return that from the function.

        measure: emitted after VI curve of each pin contains array of MM measurements
        progress: emitted after each cycle of the LV sweep
        """
        # Turn off instruments
        self.instrument_cluster.off()
        for pin in self.pin_list:
            # Connect to relay pin
            self.instrument_cluster.relay_pin(pin)
            # ramp up LV
            _, result = self.instrument_cluster._lv.sweep(
                "CURRENT",
                self.instrument_cluster._default_lv_channel,
                self.target_current,
                delay=self.delay,
                step_size=self.step_size,
                measure=True,
                measure_function=self.instrument_cluster.multimeter_measure,
                measure_args={"cycles": self.integration_cycles},
                log_function=logger.info,
                execute_each_step=self.progress.emit,
                break_monitoring=
            )

            result_up = np.array(result)
            # Ramp down LV and measure on the way down
            _, result = self.instrument_cluster._lv.sweep(
                "CURRENT",
                self.instrument_cluster._default_lv_channel,
                self.starting_current,
                delay=self.delay,
                step_size=self.step_size,
                measure=True,
                measure_function=self.instrument_cluster.multimeter_measure,
                measure_args={"cycles": self.integration_cycles},
                log_function=logger.info,
                execute_each_step=self.progress.emit,
                break_monitoring= self.breakTest
            )
            result_down = np.array(result)
            total_result = np.concatenate((result_up, result_down), axis=0)
            self.instrument_cluster.lv_off(None)
            self.measure.emit(total_result)

    def breakTest(self):
        """ Use to kill instrument sweep. """
        if self.exiting:
            return True
        return False

    def abortTest(self):
        self.exiting = True

class SLDOCurveHandler:
    def __init__(self, instrument_cluster):
        self.instrument_cluster = instrument_cluster
        self.test = SLDOCurveHandler(instrument_cluster=self.instrument_cluster)
        self.test.measure.connect(self.updateMeasurement)

    def updateMeasurement(self):
        pass

    def SLDOCurve(self):
        if not self.instrument_cluster:
            return
        self.test.start()

    def finish(self):
        self.instrument_cluster.off()

    def stop(self):
        try:
            self.test.abortTest()
            self.instrument_cluster.off()
            self.test.terminate()
        except Exception as err:
            logger.error(f"Failed to stop the SLDO test due to error {err}")
