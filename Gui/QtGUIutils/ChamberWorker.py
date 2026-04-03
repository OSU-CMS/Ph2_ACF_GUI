"""from PyQt5.QtCore import QObject, pyqtSignal

class ChamberWorker(QObject):
    finished = pyqtSignal()
    error = pyqtSignal(str)

    def __init__(self, chamber, profile_number):
        super().__init__()
        self.chamber = chamber
        self.profile_number = profile_number
        self._running = True

    def run(self):
        try:
            self.chamber.set_profile(self.profile_number)

            err = self.chamber.query("SYST:ERR?")
            logger.debug("SCPI ERROR after set_profile: %s", err)
            
            self.chamber.start_profile()

            timeout = 10
            start_time = time.time()
            running = False

            while self._running:
                state = self.chamber.get_profile_state()

                if state and "RUN" in state.upper():
                    running = True
                    break

                if time.time() - start_time > timeout:
                    logger.error("Profle never entered RUNNING state")
                    break

                time.sleep(0.5)

            if not running:
                logger.error("Aborting: chamber never started properly")

        except Exception as e:
            self.error.emit(str(e))
        finally:
            self.finished.emit()

    def stop(self):
        self._running = False"""

from PyQt5.QtCore import QObject, pyqtSignal
import time

class ChamberWorker(QObject):
    finished = pyqtSignal()
    error = pyqtSignal(str)
    progress = pyqtSignal(str)

    def __init__(self, chamber, profile_number):
        super().__init__()
        self.chamber = chamber
        self.profile_number = profile_number
        self._running = True

    def run(self):
        try:
            # Stop old profile if any
            self.chamber.stop_profile()

            if not self._running:
                return

            # Set new profile
            selected = self.chamber.set_profile(self.profile_number)
            logger.debug("Worker set profile: %s", selected)

            if not self._running:
                return

            # Start profile
            self.chamber.start_profile()
            logger.debug("Worker started profile")

            # Wait until running state or timeout
            timeout = 30
            start_time = time.time()
            running = False

            while self._running and time.time() - start_time < timeout:
                state = self.chamber.get_profile_state()
                if state and "RUN" in state.upper():
                    running = True
                    self.progress.emit("Profile running")
                    break
                time.sleep(0.5)

            if not running:
                self.error.emit("Profile never entered RUNNING state")

            # Optional: monitor profile until finished
            while self._running:
                state = self.chamber.get_profile_state()
                if state and state.upper() in ["IDLE", "COMPLETED", "TERMINATED"]:
                    self.progress.emit("Profile finished")
                    break
                time.sleep(0.5)

        except Exception as e:
            self.error.emit(str(e))
        finally:
            self.finished.emit()

    def stop(self):
        """Abort worker loop and stop profile"""
        self._running = False
        self.chamber.stop_profile()
        self.progress.emit("Profile aborted")
        