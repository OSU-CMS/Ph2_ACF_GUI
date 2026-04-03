from PyQt5 import QtCore
from PyQt5.QtCore import Qt, QThread, pyqtSignal, QTimer
from PyQt5.QtWidgets import (
    QApplication,  # ← Add this import
    QCheckBox,
    QComboBox,
    QGridLayout,
    QGroupBox,
    QLabel,
    QLineEdit,
    QMessageBox,
    QPushButton,
    QWidget,
    QVBoxLayout,
    QScrollArea,
)
from Gui.python.thermal_utils import generate_thermal_file_paths
from Gui.QtGUIutils.ChamberWorker import ChamberWorker
from Gui.python.thermal_logger import ThermalLoggerWorker
from Gui.python.thermal_plotter import generate_plot

# Comment out QtApplication import for standalone testing
# from .QtApplication import QtApplication
import sys
import os
import time
# Add current directory to Python path (this worked yesterday)

# Skip logging for testing to avoid path issues
# from Gui.python.logging_config import get_logger
# logger = get_logger(__name__)

# Use relative import - works when running from project root
from Gui.python.f4t_temperature_chamber import F4TTemperatureChamber

from Gui.python.logging_config import get_logger
logger = get_logger(__name__)


class ThermalTestModules(QWidget):
    def __init__(self, chamber, profile_name, profile_number):
        super(ThermalTestModules, self).__init__()
        self.chamber = chamber
        self.chamber_busy = False
        self.profile_name = profile_name
        self.profile_number = profile_number
        self.setWindowTitle("Thermal Test Modules")
        self.createMain()

    def createMain(self):   
        self.layout = QGridLayout()
        self.setLayout(self.layout)

        self.numberOfModules = 4 #initial number of modules to display
        self.modules = [] #list of module identification strings

        #initialize buttons
        self.addModuleButton = QPushButton("Add Module")
        self.startThermalTestButton = QPushButton("Run Thermal Test")
        self.stopThermalTestButton = QPushButton("Stop Thermal Test")
        self.closeButton = QPushButton("Exit")

        self.closeButton.setDisabled(True)

        #button clicks
        self.addModuleButton.clicked.connect(self.addModule)
        self.startThermalTestButton.clicked.connect(self.startThermalTest)
        self.stopThermalTestButton.clicked.connect(self.stopThermalTest)
        self.closeButton.clicked.connect(self.close)

        #Initial Layout
        for i in range(self.numberOfModules):
            self.modules.append(QLineEdit())
            self.layout.addWidget(QLabel(f"Module {i + 1}: "), i, 0)
            self.layout.addWidget(self.modules[i], i, 1)
        self.layout.addWidget(self.addModuleButton, self.numberOfModules - 1, 2)
        self.layout.addWidget(self.startThermalTestButton, self.numberOfModules, 2)
        self.layout.addWidget(self.stopThermalTestButton, self.numberOfModules, 3)
        self.layout.addWidget(self.closeButton, self.numberOfModules, 4)
        
        # Set the layout on the widget
        self.setLayout(self.layout)
        self.show()

    def addModule(self):
        #remove buttons so they can be repositioned
        self.layout.removeWidget(self.addModuleButton)
        self.layout.removeWidget(self.startThermalTestButton)
        self.layout.removeWidget(self.stopThermalTestButton)
        self.layout.removeWidget(self.closeButton)
        
        # Add new module row
        self.layout.addWidget(QLabel(f"Module {self.numberOfModules + 1}: "), self.numberOfModules, 0)
        self.modules.append(QLineEdit())
        self.layout.addWidget(self.modules[self.numberOfModules], self.numberOfModules, 1)
        
        # Move add button to new row
        self.layout.addWidget(self.addModuleButton, self.numberOfModules, 2)
        self.layout.addWidget(self.startThermalTestButton, self.numberOfModules + 1, 2)
        self.layout.addWidget(self.stopThermalTestButton, self.numberOfModules + 1, 3)
        self.layout.addWidget(self.closeButton, self.numberOfModules + 1, 4)
        
        self.numberOfModules += 1

    def startThermalTest(self):
        message_box = QMessageBox()
        message_box.setText(f"{self.profile_name} will run")
        message_box.setInformativeText("Do you want to continue?")
        message_box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        message_box.setDefaultButton(QMessageBox.Yes)
        response = message_box.exec_()

        if response != QMessageBox.Yes:
            return

        # Generate file paths for thermal test
        self.log_file, self.plot_file = generate_thermal_file_paths()
        logger.info("Logging to: %s", self.log_file)
        logger.info("Plotting will be saved to: %s", self.plot_file)

        profile_number = self.chamber.profiles.get(self.profile_name) #get profile number safely
        logger.info("Using cached mapping: %s --> %s", self.profile_name, profile_number)
        
        if profile_number is None:
            QMessageBox.warning(self, "Error", f"Profile {self.profile_name} not found")
            logger.error("Profile %s not found", self.profile_name)
            return
        if self.chamber_busy:
            QMessageBox.warning(self, "Busy", "Chamber is currently in use")
            return

        self.chamber_busy = True

        #disable start button to prevent double clicks
        self.startThermalTestButton.setDisabled(True)

        #create unique thread + worker
        self.start_thread = QThread()
        self.logger_thread = QThread()

        self.start_worker = ChamberWorker(self.chamber, profile_number)
        self.logger_worker = ThermalLoggerWorker(self.chamber, self.log_file)

        self.start_worker.moveToThread(self.start_thread)
        self.logger_worker.moveToThread(self.logger_thread)

        # connections
        self.start_thread.started.connect(self.start_worker.run)
        self.start_worker.finished.connect(self.start_thread.quit)
        self.start_worker.finished.connect(self.logger_thread.start)
        self.start_worker.finished.connect(self.start_worker.deleteLater)
        self.start_thread.finished.connect(self.start_thread.deleteLater)

        self.logger_thread.started.connect(self.logger_worker.run)
        self.logger_worker.finished.connect(self.on_logging_finished)
        self.logger_worker.finished.connect(self.logger_thread.quit)
        self.logger_worker.finished.connect(self.logger_worker.deleteLater)
        self.logger_thread.finished.connect(self.logger_thread.deleteLater)

        # re-enable buttons when done
        self.start_worker.finished.connect(lambda: self.startThermalTestButton.setDisabled(False))
        self.start_worker.finished.connect(lambda: self.stopThermalTestButton.setDisabled(False))

        #Force clean state before starting
        logger.info("Forcing chamber into known state before test")

        self.chamber.stop_profile()
        time.sleep(1)

        err = self.chamber.query("SYST:ERR?")
        logger.debug("SCPI ERROR after stop_profile: %s", err)

        #self.chamber.control_output("OFF")
        #time.sleep(1)

        #self.chamber.control_output("ON")
        #time.sleep(1)
        
        self.start_thread.start()

    def stopThermalTest(self):
        from Gui.python.thermal_plotter import generate_plot
        message_box = QMessageBox()
        message_box.setText("You are about to stop the thermal test")
        message_box.setInformativeText("Do you want to continue?")
        message_box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        message_box.setDefaultButton(QMessageBox.Yes)
        response = message_box.exec_()

        if response == QMessageBox.No:
            return
        
        if response == QMessageBox.Yes:
            self.chamber.stop_profile()

            err = self.chamber.query("SYST:ERR?")
            logger.debug("SCPI ERROR after stop_profile: %s", err)
            
            self.stopThermalTestButton.setDisabled(True)

        if hasattr(self, "logger_worker") and self.logger_worker:
            self.logger_worker.stop()

        if hasattr(self, "logger_thread") and self.logger_thread:
            if self.logger_thread.isRunning():
                self.logger_thread.quit()
                self.logger_thread.wait()

        
        if hasattr(self, "plot_file"):
            QMessageBox.information(self, "Done", f"Plot saved to: \n{self.plot_file}")
        else:
            QMessageBox.information(self, "Done", "Plot saved (path unavailable)")

    def on_logging_finished(self):
        logger.info("Logging finished, generating plot...")

        generate_plot(self.log_file, self.plot_file)

        QMessageBox.information(
            self,
            "Thermal Test Complete",
            f"Plot saved to:\n{self.plot_file}"
        )

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = ThermalTestModules(QWidget(), "test_profile")
    sys.exit(app.exec())
