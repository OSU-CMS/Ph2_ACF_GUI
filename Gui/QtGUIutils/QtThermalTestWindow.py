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

from Gui.QtGUIutils.QtThermalModulesWindow import ThermalModulesWindow

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


class ThermalTestWindow(QWidget):
    def __init__(self, master, chamber):
        super().__init__()
        self.master = master
        self.chamber = chamber
        self.initUI()
        
    def initUI(self):
        self.setWindowTitle("Thermal Test Window")
        
        
        layout = QGridLayout()
        self.setLayout(layout)

        self.ThermalTestButton = QPushButton("&Thermal Test")
        self.ThermalTestButton.setEnabled(True)
        self.UpdateProfilesButton = QPushButton("&Update Profiles")
        self.UpdateProfilesButton.setEnabled(True)
        self.AbortThermalTestButton = QPushButton("&Abort thermal test")
        self.AbortThermalTestButton.setEnabled(True)
        self.EndChamberOutputButton = QPushButton("End Chamber Output")
        self.EndChamberOutputButton.setEnabled(True)
        self.closeButton = QPushButton("Exit")

        self.closeButton.setDisabled(False)

        if not self.chamber:
            self.ThermalTestButton.setDisabled(True)
            self.AbortThermalTestButton.setDisabled(True)
            self.UpdateProfilesButton.setDisabled(True)

        self.closeButton.clicked.connect(self.close)

        kMinimumWidth = 120
        kMaximumWidth = 150
        kMinimumHeight = 30
        kMaximumHeight = 100

        self.UpdateProfilesButton.setMinimumWidth(kMinimumWidth)
        self.UpdateProfilesButton.setMaximumWidth(kMaximumWidth)
        self.UpdateProfilesButton.setMinimumHeight(kMinimumHeight)
        self.UpdateProfilesButton.setMaximumHeight(kMaximumHeight)
        self.UpdateProfilesButton.clicked.connect(self.updateProfiles)

        self.AbortThermalTestButton.setMinimumWidth(kMinimumWidth)
        self.AbortThermalTestButton.setMaximumWidth(kMaximumWidth)
        self.AbortThermalTestButton.setMinimumHeight(kMinimumHeight)
        self.AbortThermalTestButton.setMaximumHeight(kMaximumHeight)
        self.AbortThermalTestButton.clicked.connect(self.abortThermalTest)

        self.startThermalTestButton = QPushButton("&Start Thermal Test")
        self.startThermalTestButton.setMinimumWidth(kMinimumWidth)
        self.startThermalTestButton.setMaximumWidth(kMaximumWidth)
        self.startThermalTestButton.setMinimumHeight(kMinimumHeight)
        self.startThermalTestButton.setMaximumHeight(kMaximumHeight)
        self.startThermalTestButton.clicked.connect(self.runThermalTest)

        self.EndChamberOutputButton.setMinimumWidth(kMinimumWidth)
        self.EndChamberOutputButton.setMaximumWidth(kMaximumWidth)
        self.EndChamberOutputButton.setMinimumHeight(kMinimumHeight)
        self.EndChamberOutputButton.setMaximumHeight(kMaximumHeight)
        self.EndChamberOutputButton.clicked.connect(self.endChamberOutput)

        label = QLabel("Thermal Test Window")
        layout.addWidget(label, 0, 0)

        if self.chamber and self.chamber.profiles:
            self.thermalProfilesStatusLabel = QLabel("Loaded cached profiles")
        else:
            self.thermalProfilesStatusLabel = QLabel("No cached profiles found")
        self.ThermalProfileCombo = QComboBox()
        self.ThermalProfileCombo.clear()

        if self.chamber and self.chamber.profiles:
            for name in self.chamber.profiles.keys():
                if name not in ["", "New Profile", "0"]:
                    self.ThermalProfileCombo.addItem(name)

        layout.addWidget(self.UpdateProfilesButton, 4, 0, 1, 1)
        layout.addWidget(self.ThermalProfileCombo, 4, 1, 1, 1)
        layout.addWidget(self.thermalProfilesStatusLabel, 5, 0, 1, 2)
        layout.addWidget(self.startThermalTestButton, 6, 0, 1, 1)
        layout.addWidget(self.AbortThermalTestButton, 7, 0, 1, 1)
        layout.addWidget(self.EndChamberOutputButton, 9, 0, 1, 1)
        layout.addWidget(self.closeButton, 11, 0, 1, 1)

        if not self.chamber.profiles:
            profiles, source = self.chamber.query_profiles(force_refresh=True)
            self.on_profiles_loaded(profiles, source)
        elif self.chamber and self.chamber.profiles:
            self.on_profiles_loaded(self.chamber.profiles, "cache")
        
        self.show()

    def updateProfiles(self):
        """Update the profiles in the QComboBox using profileworker"""

        # start the profile loading process
        result = self.chamber.query_profiles(force_refresh=True)
        if result is None:
            profiles, source = {}, "error"
        else:
            profiles, source = result
        
        self.on_profiles_loaded(profiles, source)

    def abortThermalTest(self):
        """Stop the current profile running on thermal chamber"""

        #Stop the profile
        self.chamber.stop_profile()

        message_box = QMessageBox()
        message_box.setText("Profile Aborted")
        message_box.setStandardButtons(QMessageBox.Ok)
        message_box.exec()

    def runThermalTest(self):
        # Verify input of input data:
        self.profile_name = self.ThermalProfileCombo.currentText()

        profile_dictionary = self.chamber.profiles

        # Debug: Show what we're comparing
        logger.info("Selected profile from QComboBox: '{}'".format(self.profile_name))
        logger.info("Available profile names: {}".format(list(profile_dictionary.keys())))
        
        # Check if this value corresponds to a profile in the dictionary
        if self.profile_name in profile_dictionary:
            # Find the profile number by searching values
            profile_number = profile_dictionary[self.profile_name]
            logger.info("Found profile: '{}' with number: {}".format(self.profile_name, profile_number))
        else:
            logger.error("Could not find profile name: {})".format(self.profile_name))
            logger.debug("Available profiles: %s", profile_dictionary)
            return  # Exit early if profile not found

        message_box = QMessageBox()
        message_box.setText(f'"{self.profile_name}" will run')
        message_box.setInformativeText("Do you want to continue?")
        message_box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        message_box.setDefaultButton(QMessageBox.Yes)
        response = message_box.exec()

        if response != QMessageBox.Yes:
            return

        #check state to see if chamber is busy
        state = self.chamber.get_profile_state()
        if state and state.strip().upper() in ["RUNNING", "PAUSED"]:
            logger.warning("Chamber is currently running or paused")
            message_box = QMessageBox()
            message_box.setText("Chamber is currently running or paused")
            message_box.setInformativeText("Please stop the current profile before starting a new one")
            message_box.setIcon(QMessageBox.Warning)
            message_box.exec()
            return
        
        try:
            # Set the profile on the chamber
            self.chamber.set_profile(profile_number)

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

            # Create and show the thermal test window
            logger.info("Controller selected profile: %s" % self.chamber.get_profile_name())
            self.openModulesWindow()
        except Exception as e:
            logger.error("Could not set profile: %s", e)
            return

    def endChamberOutput(self):
        message_box0 = QMessageBox()
        message_box0.setText("Are you sure you want to turn off the thermal chamber output?")
        message_box0.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        message_box0.setDefaultButton(QMessageBox.No)
        reply = message_box0.exec()

        if reply == QMessageBox.Yes:
            """Turn off the thermal chamber output"""
            self.chamber.turn_off()
            message_box = QMessageBox()
            message_box.setText("Profiles stopped and output turned off")
            message_box.setStandardButtons(QMessageBox.Ok)
            message_box.exec()

        if reply == QMessageBox.No:
            return

    def on_profiles_loaded(self, profiles, source):
        #save current selection
        current = self.ThermalProfileCombo.currentText()

        self.ThermalProfileCombo.blockSignals(True)
        self.ThermalProfileCombo.clear()

        cleaned = {}

        for name, number in profiles.items():
            name = name.strip('"')
            if name not in ["", "New Profile", "0"]:
                cleaned[name] = number
                self.ThermalProfileCombo.addItem(name)

        #store in memory
        self.chamber.profiles = cleaned

        #restore selection if it still exists
        if current in cleaned:
            index = self.ThermalProfileCombo.findText(current)
            self.ThermalProfileCombo.setCurrentIndex(index)

        self.ThermalProfileCombo.setCurrentIndex(-1)
        self.ThermalProfileCombo.blockSignals(False)
        self.thermalProfilesStatusLabel.setText("Profiles updated")

    def openModulesWindow(self):
        """Open the thermal modules window"""
        self.modules_window = ThermalModulesWindow(self.master)
        self.modules_window.show()

    

    