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
from Gui.python.thermal_chamber import F4TTemperatureChamber

from Gui.python.logging_config import get_logger
logger = get_logger(__name__)


class ThermalTestModules(QWidget):
    def __init__(self):
        super(ThermalTestModules, self).__init__()
        self.setWindowTitle("Thermal Test Modules")
        self.createMain()

    def createMain(self):   
        self.layout = QGridLayout()
        self.setLayout(self.layout)

        self.numberOfModules = 4 #initial number of modules to display
        self.modules = [] #list of module identification strings

        #initialize buttons
        self.addModuleButton = QPushButton("Add Module")
        self.closeButton = QPushButton("Exit")

        self.closeButton.setDisabled(False)

        #button clicks
        self.addModuleButton.clicked.connect(self.addModule)
        self.closeButton.clicked.connect(self.close)

        #Initial Layout
        for i in range(self.numberOfModules):
            self.modules.append(QLineEdit())
            self.layout.addWidget(QLabel(f"Module {i + 1}: "), i, 0)
            self.layout.addWidget(self.modules[i], i, 1)
        self.layout.addWidget(self.addModuleButton, self.numberOfModules - 1, 2)
        self.layout.addWidget(self.closeButton, self.numberOfModules, 4)
        
        # Set the layout on the widget
        self.setLayout(self.layout)
        self.show()

    def addModule(self):
        #remove buttons so they can be repositioned
        self.layout.removeWidget(self.addModuleButton)
        self.layout.removeWidget(self.closeButton)
        
        # Add new module row
        self.layout.addWidget(QLabel(f"Module {self.numberOfModules + 1}: "), self.numberOfModules, 0)
        self.modules.append(QLineEdit())
        self.layout.addWidget(self.modules[self.numberOfModules], self.numberOfModules, 1)
        
        # Move add button to new row
        self.layout.addWidget(self.addModuleButton, self.numberOfModules, 2)
        self.layout.addWidget(self.closeButton, self.numberOfModules + 1, 4)
        
        self.numberOfModules += 1

    """def startThermalTest(self):
        message_box = QMessageBox()
        message_box.setText(f"{self.profile_name} will run")
        message_box.setInformativeText("Do you want to continue?")
        message_box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        message_box.setDefaultButton(QMessageBox.Yes)
        response = message_box.exec_()

        if response != QMessageBox.Yes:
            return

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

        #Force clean state before starting
        logger.info("Forcing chamber into known state before test")

        self.chamber.stop_profile()

        err = self.chamber.query("SYST:ERR?")
        if err:
            logger.warning("SCPI ERROR after stop_profile: %s", err)

        self.chamber.start_profile()

        err = self.chamber.query("SYST:ERR?")
        if err:
            logger.warning("SCPI ERROR after start_profile: %s", err)

    def stopThermalTest(self):
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
            if err:
                logger.warning("SCPI ERROR after stop_profile: %s", err)
            
            self.stopThermalTestButton.setDisabled(True)"""

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = ThermalTestModules(QWidget(), "test_profile")
    sys.exit(app.exec())
