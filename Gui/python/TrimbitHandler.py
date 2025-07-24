
"""
Class to perform the Trimbit curve scanning
"""
from PyQt5.QtCore import QThread, pyqtSignal, QObject, QProcess
from Gui.python.logging_config import logger
from Gui.python.ANSIColoringParser import parseANSI
from icicle.icicle.adc_board import ADCBoard

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
        if "adc_board" not in self.instruments._instrument_dict:
            logger.error("No ADC board found for TrimbitScan.")
            return
        self.adc_board = self.instruments._instrument_dict["adc_board"]
        self.pin_mapping = self.PIN_MAPPINGS[
            self.moduleType.split(" ")[-1].replace("1x2", "DOUBLE").upper()
        ]
        self.adc_board._pin_map = self.pin_mapping
        if self.moduleType.split(" ")[-1] == "1x2":
            chip_list = [12, 13]
            pin_list = [2, 6, 12, 14]
        else:
            chip_list = [12, 13, 14, 15]
            pin_list = [1, 2, 3, 4, 12, 13, 14, 15]
        self.ADCmeasurements = {pin: [] for pin in pin_list}
        for chip in chip_list:
            if self.exiting:
                break
            self.trimbit_dict = {c: (0, 0) for c in chip_list}
            newTrim = np.array([0, 0])
            for step in range(self.total_steps):
                if self.exiting:
                    break
                self.trimbit_dict[chip] = tuple(newTrim)
                self.testhandler.configTest(trimbit_dict=self.trimbit_dict)
                addTrim = self.run_VDDsweep(chip)
                newTrim += addTrim
                self.ProgressValue += 1
                percent = 100 * self.ProgressValue / (self.total_steps * len(chip_list))
                self.progressSignal.emit("TrimbitScan", percent)
        if not self.exiting:
            self.measure.emit(self.ADCmeasurements, self.pin_mapping)
            self.finishedSignal.emit()
    def run_VDDsweep(self, chip, fc7_index=0):
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
        # Wait for process to finish, but check for abort every 100ms
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

        add_trim = self.measureADC(chip)
        # ProgressValue increment and progress bar update now handled in run()
        return add_trim
    
    def measureADC(self, chip):
        Add_VDDA = 0
        Add_VDDD = 0
        for pin, name in self.pin_mapping.items():
            # Check pin is in measured chip
            logger.info("Pin: {0}, Name: {1}, Chip: {2}".format(pin, name, chip))
            if name.endswith(str(chip)):
                measurement = self.adc_board.query_channel(pin)
                if name.startswith("VDDA"):
                    self.ADCmeasurements[pin].append((self.trimbit_dict[chip][0], measurement))
                    if measurement > 1.29:
                        Add_VDDA = 0
                    else:
                        Add_VDDA = 1
                else:
                    self.ADCmeasurements[pin].append((self.trimbit_dict[chip][1], measurement))
                    if measurement > 1.29:
                        Add_VDDD = 0
                    else:
                        Add_VDDD = 1
        # Adds a tuple (trimbit, ADC reading) for each pin in the measured chip
        return np.array([Add_VDDA, Add_VDDD])
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
