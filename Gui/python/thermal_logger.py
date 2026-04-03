import time
import csv
from PyQt5.QtCore import QObject, pyqtSignal
from Gui.python.logging_config import get_logger

logger = get_logger(__name__)

class ThermalLoggerWorker(QObject):
    finished = pyqtSignal()
    error = pyqtSignal(str)

    def __init__(self, chamber, log_file):
        super().__init__()
        self.chamber = chamber
        self.log_file = log_file
        self._running = True

    def run(self):
        try:
            self._running = True

            with open(self.log_file, "w", newline="") as f:
                writer = csv.writer(f)
                writer.writerow(["time s", "temperature", "setpoint", "profile"])

                start_time = time.time()
                started = False

                while self._running:
                    state = self.chamber.get_profile_state()
                    logger.debug("STATE RAW: %s", state)

                    if state:
                        state_clean = state.strip().upper()

                        if state_clean == "RUNNING":
                            started = True

                        if started:
                            temp = self.chamber.get_temperature()
                            setpoint = self.chamber.get_setpoint()
                            profile = self.chamber.current_profile()

                            timestamp = time.time() - start_time
                            writer.writerow([timestamp, temp, setpoint, profile])
                            f.flush()

                        if started and state_clean in ["STOP", "END", "IDLE", "COMPLETED", "TERMINATED"]:
                            logger.info("Profile finished automatically")
                            break

                    time.sleep(2)

        except Exception as e:
            self.error.emit(str(e))
        finally:
            self.finished.emit()

    def stop(self):
        self._running = False