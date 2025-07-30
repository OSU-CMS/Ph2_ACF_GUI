"""
Class to perform the Trimbit curve scanning
"""
from PyQt5.QtCore import QThread, pyqtSignal, QObject, QProcess
from Gui.python.logging_config import logger
from icicle.icicle.adc_board import ADCBoard
import Gui.siteSettings as site_settings
import ROOT
import os
from ctypes import c_double

import numpy as np
import os

class TrimbitCurveWorker(QThread):
    def __init__(self, instrument_cluster, moduleType, total_steps, runwindow, firmware, testhandler):
        super().__init__()
        self.instruments = instrument_cluster
        self.moduleType = moduleType
        self.total_steps = total_steps
        self.runwindow = runwindow
        self.testhandler = testhandler
        self.firmware = firmware
        self.exiting = False
        self.ProgressValue = 0
        self.pin_mapping = {}
        self.ADCmeasurements = {}
        self.trimbit_dict = {}
        self.PIN_MAPPINGS = {
            "DEFAULT": ADCBoard.DEFAULT_PIN_MAP,
            "DOUBLE": {
                2: 'VDDA_ROC13',
                6: 'VDDD_ROC13',
                12: "VDDA_ROC12",
                14: "VDDD_ROC12",
            },
            "QUAD": {
                1: "VDDA_ROC14",
                2: "VDDA_ROC15",
                3: "VDDD_ROC14",
                4: "VDDD_ROC15",
                12: "VDDA_ROC12",
                13: "VDDA_ROC13",
                14: "VDDD_ROC12",
                15: "VDDD_ROC13",
            },
        }

    def run(self):
        if not self.initialize_scan():
            return

        for chip in self.chip_list:
            if self.exiting:
                break
            self.process_chip(chip)

        if not self.exiting:
            self.measure.emit(self.ADCmeasurements, self.pin_mapping)
            self.finishedSignal.emit()

    def initialize_scan(self):
        """
        Initializes the scan by setting up the ADC board, pin mapping, and chip/pin lists.
        Returns True if initialization is successful, False otherwise.
        """
        if "adc_board" not in self.instruments._instrument_dict:
            logger.error("No ADC board found for TrimbitScan.")
            return False

        self.adc_board = self.instruments._instrument_dict["adc_board"]
        self.pin_mapping = self.PIN_MAPPINGS[
            self.moduleType.split(" ")[-1].replace("1x2", "DOUBLE").upper()
        ]
        self.adc_board._pin_map = self.pin_mapping

        if self.moduleType.split(" ")[-1] == "1x2":
            self.chip_list = [12, 13]
            self.pin_list = [2, 6, 12, 14]
        else:
            self.chip_list = [12, 13, 14, 15]
            self.pin_list = [1, 2, 3, 4, 12, 13, 14, 15]

        self.ADCmeasurements = {pin: [] for pin in self.pin_list}
        return True

    def process_chip(self, chip):
        """
        Processes a single chip by iterating through the steps and performing the scan.
        """
        self.trimbit_dict = {c: (0, 0) for c in self.chip_list}
        newTrim = np.array([0, 0])

        for _ in range(self.total_steps):
            if self.exiting:
                break
            self.process_step(chip, newTrim)

    def process_step(self, chip, newTrim):
        """
        Processes a single step for the given chip.
        """
        self.trimbit_dict[chip] = tuple(newTrim)
        self.testhandler.configTest(trimbit_dict=self.trimbit_dict)

        if self.testhandler.currentTest == "TrimbitScan_GADC":
            addTrim = self.run_VDDsweep_GADC(chip)
        else:
            addTrim = self.run_VDDsweep_normal(chip)

        newTrim += addTrim
        self.ProgressValue += 1
        percent = 100 * self.ProgressValue / (self.total_steps * len(self.chip_list))
        self.progressSignal.emit("TrimbitScan", percent)
    
    def run_VDDsweep_normal(self, chip, fc7_index=0):
        VDDsweep_process = QProcess()
        VDDsweep_process.setProcessChannelMode(QProcess.MergedChannels)
        VDDsweep_process.setWorkingDirectory(
            os.environ.get("PH2ACF_BASE_DIR") + "/test/"
        )
        VDDsweep_process.readyReadStandardOutput.connect(
            lambda: self.testhandler.on_readyReadStandardOutput_VDDsweep(VDDsweep_process, fc7_index)
        )
        VDDsweep_process.start(
            "CMSITminiDAQ",
            ["-f", f"CMSIT_{self.firmware[fc7_index].getBoardName()}.xml"],
        )
        while VDDsweep_process.state() != QProcess.NotRunning:
            if self.exiting:
                VDDsweep_process.kill()
                VDDsweep_process.waitForFinished(500)
                logger.info(f"TrimbitScan aborted: killed VDDsweep_process for chip {chip}")
                break
            VDDsweep_process.waitForFinished(100)
        if VDDsweep_process.state() != QProcess.NotRunning:
            logger.error(f"Ph2_ACF physics test on {self.firmware[fc7_index].getBoardName()} didn't execute correctly.")
            VDDsweep_process.kill()
            VDDsweep_process.waitForFinished(500)
        self.measureADC(chip)
        add_trim = self.add_trim(chip)
        return add_trim
    
    def run_VDDsweep_GADC(self, chip, fc7_index=0):
        VDDsweep_process = QProcess()
        VDDsweep_process.setProcessChannelMode(QProcess.MergedChannels)
        VDDsweep_process.setWorkingDirectory(
            os.environ.get("PH2ACF_BASE_DIR") + "/test/"
        )
        VDDsweep_process.readyReadStandardOutput.connect(
            lambda: self.testhandler.on_readyReadStandardOutput_VDDsweep(VDDsweep_process, chip, fc7_index)
        )
        VDDsweep_process.start(
                "CMSITminiDAQ",
                ["-f", f"CMSIT_{self.firmware[fc7_index].getBoardName()}.xml", "-c", "physics", "-t", str(site_settings.Trimbit_GADC['physics seconds'])],
            )
        while VDDsweep_process.state() != QProcess.NotRunning:
            if self.exiting:
                VDDsweep_process.kill()
                VDDsweep_process.waitForFinished(500)
                logger.info(f"TrimbitScan aborted: killed VDDsweep_process for chip {chip}")
                break
            VDDsweep_process.waitForFinished(100)
        if VDDsweep_process.state() != QProcess.NotRunning:
            logger.error(f"Ph2_ACF physics test on {self.firmware[fc7_index].getBoardName()} didn't execute correctly.")
            VDDsweep_process.kill()
            VDDsweep_process.waitForFinished(500)

        self.measureGADC(chip)
        add_trim = self.add_trim(chip)
        return add_trim
    
    def add_trim(self,chip):
        Add_VDDA = 0
        Add_VDDD = 0
        for pin, name in self.pin_mapping.items():
            # Get the last measurement for the pin
            measurement = self.ADCmeasurements[pin][-1][1] if self.ADCmeasurements[pin] else None
            if name.endswith(str(chip)):
                if name.startswith("VDDA"):
                    if measurement > 1.29:
                        Add_VDDA = 0
                    else:
                        Add_VDDA = 1
                else:
                    if measurement > 1.29:
                        Add_VDDD = 0
                    else:
                        Add_VDDD = 1
        # Adds a tuple (trimbit, ADC reading) for each pin in the measured chip
        return np.array([Add_VDDA, Add_VDDD])
    def measureADC(self, chip):
        for pin, name in self.pin_mapping.items():
            # Check pin is in measured chip
            logger.info("Pin: {0}, Name: {1}, Chip: {2}".format(pin, name, chip))
            if name.endswith(str(chip)):
                measurement = self.adc_board.query_channel(pin)
                if name.startswith("VDDA"):
                    self.ADCmeasurements[pin].append((self.trimbit_dict[chip][0], measurement))
                else:
                    self.ADCmeasurements[pin].append((self.trimbit_dict[chip][1], measurement))



    def measureGADC(self, chip):
        try:
            VDDD = self.getRootMeasurement(chip, "VDDD")
            VDDA = self.getRootMeasurement(chip, "VDDA")
            for pin, name in self.pin_mapping.items():
                # Check pin is in measured chip
                logger.info("Pin: {0}, Name: {1}, Chip: {2}".format(pin, name, chip))
                if name.endswith(str(chip)):
                    if name.startswith("VDDA"):
                        self.ADCmeasurements[pin].append((self.trimbit_dict[chip][0], VDDA))
                    else:
                        self.ADCmeasurements[pin].append((self.trimbit_dict[chip][1], VDDD))
        except Exception as e:
            logger.error(f"Error in measureGADC: {e}")

    def getRootMeasurement(self, chip, measurement_type):
        """
        Extracts data from a ROOT file for a specific chip and measurement type.

        Args:
            root_file_path (str): Path to the ROOT file.
            chip (int): Chip number to extract data for.
            measurement_type (str): Measurement type (e.g., 'VDDD', 'VDDA').

        Returns:
            float: The maximum measurement value for the specified chip.
        """
        root_file_path = os.path.join(
        os.environ.get("PH2ACF_BASE_DIR", ""),
        "test/Results",
        f"Run{self.testhandler.RunNumber}_MonitorDQM.root"
        )
        
        if not os.path.exists(root_file_path):
            logger.error(f"ROOT file not found: {root_file_path}")
            return None

        try:

            # Open the ROOT file
            root_file = ROOT.TFile(root_file_path, "READ")
            if root_file.IsZombie():
                logger.error(f"Failed to open ROOT file: {root_file_path}")
                return None

            # Construct the path to the desired data
            detector_path = f"Detector/Board_0/OpticalGroup_0/Hybrid_0/Chip_{chip}/D_B(0)_O(0)_H(0)_DQM_{measurement_type}_Chip({chip});3"

            # Retrieve the TGraph object
            tgraph = root_file.Get(detector_path)
            if not tgraph:
                logger.error(f"TGraph not found at path: {detector_path}")
                root_file.Close()
                return None

            # Extract data points from the TGraph
            data = []
            for i in range(tgraph.GetN()):
                x, y = c_double(), c_double() 
                tgraph.GetPoint(i, x, y)

                # Using multiplier to account for voltage splitting
                data.append(float(y.value)*int(site_settings.Trimbit_GADC['multipliers']['VDD']))

            if not data:  # Check if data is empty
                logger.error(f"No data points found in TGraph at path: {detector_path}")
                root_file.Close()
                return None

            measurement = max(data)  # Get max measurement
            root_file.Close()
            return measurement

        except Exception as e:
            logger.error(f"Error extracting data from ROOT file: {e}")
            return None

    measure = pyqtSignal(dict, dict)
    progressSignal = pyqtSignal(str, float)
    finishedSignal = pyqtSignal()
    outputString = pyqtSignal(str, object)



class TrimbitCurveHandler(QObject):
    finishedSignal = pyqtSignal()
    makeplotSignal = pyqtSignal(dict, dict)
    progressSignal = pyqtSignal(str, float)
    abortSignal = pyqtSignal()
    outputString = pyqtSignal(str, object)


    def __init__(self, instrument_cluster, moduleType, total_steps, runwindow, firmware, testhandler):
        super().__init__()
        self.runwindow = runwindow
        self.test = TrimbitCurveWorker(
            instrument_cluster=instrument_cluster,
            moduleType=moduleType,
            total_steps=total_steps,
            runwindow=runwindow,
            firmware=firmware,
            testhandler=testhandler
        )
        self.test.measure.connect(self.makePlots)
        self.test.progressSignal.connect(self.updateProgress)
        self.test.finishedSignal.connect(self.finish)


    def makePlots(self, total_result, pin_list):
        self.makeplotSignal.emit(total_result, pin_list)

    def TrimbitScan(self):
        if not self.test.instruments:
            return
        self.test.start()

    def updateProgress(self, measurementType, percentStep):
        self.progressSignal.emit(measurementType, percentStep)

    def finish(self):
        self.finishedSignal.emit()

    def stop(self, reason=None):
        try:
            self.test.exiting = True
            self.abortSignal.emit()
        except Exception as err:
            logger.error(f"Failed to stop the TrimbitScan due to error {err}")
