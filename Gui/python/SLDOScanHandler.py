"""
Class to perform the SLDO curve scanning
"""

from PyQt5.QtCore import QObject, QThread, pyqtSignal
from Gui.python.logging_config import logger
import numpy as np
import numpy.typing as npt


class sldo_curve_worker(QObject):
    finished = pyqtSignal()

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

    def run(self) -> npt.NDArray[np.float_]:
        """
        Run thread that will ramp up the LV while measuring from the multimeter, then ramp down doing the same thing.
        Combine the two list of measurments and return that from the function.
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
            )
            result_down = np.array(result)
            total_result = np.concatenate((result_up, result_down), axis=0)

            self.instrument_cluster.lv_off(None)

            # NOTE this should probably be emitted as a signal
            return total_result
