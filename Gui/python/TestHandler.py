from PyQt5 import QtCore
from PyQt5.QtCore import pyqtSignal, QObject, QProcess, Qt
from PyQt5.QtWidgets import (
    QMessageBox,
    QTableWidget,
    QTableWidgetItem,
    QPlainTextEdit,
    QProgressBar,
    QDialog,
    QVBoxLayout,
    QPushButton,
    QLabel,
)

import os
import glob
import subprocess
import threading
import time
import re
import traceback
from typing import Optional
from datetime import datetime
import numpy as np
import matplotlib.pyplot as plt

from Gui.GUIutils.settings import (
    ModuleLaneMap,
    firmware_image,
    updatedXMLValues,
    optimizationTestMap,
)
from Gui.GUIutils.guiUtils import (
    ConfigureTest,
    SetupXMLConfigfromFile,
    SetupRD53Config,
    SetupRD53ConfigfromFile,
    GenerateXMLConfig,
    isCompositeTest,
    isSingleTest,
    UpdateXMLValue,
)
from Gui.python.ROOTInterface import executeCommandSequence
from felis.felis import Felis
from InnerTrackerTests.Analysis.IVCurve_CSV_to_ROOT import IVCurve_CSV_to_ROOT
from InnerTrackerTests.RootFilesDict import root_files
from InnerTrackerTests.Analysis.SLDO_CSV_to_ROOT import SLDO_CSV_to_ROOT
from Gui.QtGUIutils.QtMatplotlibUtils import ScanCanvas
from Gui.python.TestValidator import ResultGrader
from Gui.python.ANSIColoringParser import parseANSI
from Gui.python.IVCurveHandler import IVCurveHandler
from Gui.python.SLDOScanHandler import SLDOCurveHandler
from Gui.python.TrimbitHandler import TrimbitCurveHandler
import Gui.siteSettings as site_settings
from Gui.python.logging_config import logger
from Gui.python.CustomizedWidget import chip_iref_db
from InnerTrackerTests.TestSequences import CompositeTests_Modules, Test_to_Ph2ACF_Map
from icicle.icicle.adc_board import ADCBoard


class TestHandler(QObject):
    backSignal = pyqtSignal(object)
    haltSignal = pyqtSignal(object)
    finishSignal = pyqtSignal(object)
    proceedSignal = pyqtSignal(object)
    outputString = pyqtSignal(str, QPlainTextEdit)
    stepFinished = pyqtSignal(object)
    historyRefresh = pyqtSignal()
    updateResult = pyqtSignal(object)
    updateIVResult = pyqtSignal(object)
    updateSLDOResult = pyqtSignal(object)
    updateValidation = pyqtSignal(object)
    updateFinishedTests = pyqtSignal(object)
    powerSignal = pyqtSignal()
    updateProgressBar = pyqtSignal(QProgressBar, int, str)

    def __init__(self, runwindow, master, info, firmware, txt_files={}):
        super(TestHandler, self).__init__()
        self.master = master
        self.instruments = self.master.instruments
        self.runwindow = runwindow
        self.firmware = firmware
        self.info = info
        self.txt_files = txt_files if txt_files else {}
        self.init_signals()
        self.initialize_variables()
        self.prepare_firmware_components()

    def init_signals(self):
        self.master.globalStop.connect(self.urgentStop)
        self.haltSignal.connect(self.runwindow.finish)
        self.outputString.connect(self.runwindow.updateConsoleInfo)
        self.stepFinished.connect(self.runwindow.finish)
        self.historyRefresh.connect(self.runwindow.refreshHistory)
        self.updateResult.connect(self.runwindow.updateResult)
        self.updateIVResult.connect(self.runwindow.updateIVResult)
        self.updateSLDOResult.connect(self.runwindow.updateSLDOResult)
        self.updateValidation.connect(self.runwindow.updateValidation)
        self.updateFinishedTests.connect(self.runwindow.updateFinishedTests)
        self.updateProgressBar.connect(self.runwindow.updateProgressBar)

    def initialize_variables(self):
        self.modules = [
            module for beboard in self.firmware for module in beboard.getModules()
        ]
        self.communicationTestResults = {
            module.getModuleName(): None for module in self.modules
        }
        self.communicationTestModule = None
        self.figurelist = {}
        self.SLDOfilelist = []

        # Variables used when parsing Ph2_ACF output
        self.mod_dict = {}
        self.fused_dict_index = [-1, -1]
        self.GADC_meas_chip = None

        # Define variables to store "Handlers" to help with shutdown process
        self.IVCurveHandler = None
        self.SLDOScanHandler = None
        self.trimbitHandler = None

        self.ModuleMap = {}
        self.BBanalysis_root_files = []
        self.numChips = self.count_enabled_chips()
        self.ModuleType = self.runwindow.ModuleType
        self.boardType = "RD53B" if "CROC" in self.ModuleType else "RD53A"
        self.set_module_and_hdi_version()
        self.registerKey = f"{self.ModuleType.replace(' ', '_')}_HDIv{self.hdiVersion}"
        self.setupTestList()
        self.finished_tests = []
        self.prepare_firmware_paths()
        self.RunNumber = "-1"
        self.isTDACtuned = False
        self.setup_processing_variables()
        self.configure_felis()
        self.setup_process_connections()

    def prepare_firmware_paths(self):
        self.Ph2_ACF_ver = os.environ.get("Ph2_ACF_VERSION")
        logger.info(f"Using version {self.Ph2_ACF_ver} of Ph2_ACF")
        self.firmwareImage = firmware_image[self.ModuleType][self.Ph2_ACF_ver]
        logger.info(f"Firmware version is {self.firmwareImage}")

    def setupTestList(self):
        try:
            self.test_list = (
                CompositeTests_Modules[self.registerKey][self.info]
                if isCompositeTest(self.info)
                else (self.info,)
            )
            self.module_test_history = {
                module.getModuleName(): {
                    test: {"Passed": 0, "Failed": 0} for test in self.test_list
                }
                for module in self.modules
            }

        except KeyError:
            logger.error(
                f"Test {self.info} not found in CompositeTests_Modules for ModuleType {self.registerKey}."
            )
            raise KeyError

    def count_enabled_chips(self):
        return len(
            [
                chipID
                for module in self.modules
                for chipID in module.getEnabledChips().keys()
            ]
        )

    def set_module_and_hdi_version(self):
        if "CROC" in self.ModuleType:
            self.moduleVersion = self.firmware[0].getModuleData()["version"]
            self.hdiVersion = self.firmware[0].getModuleData()["hdiVersion"]
        else:
            self.moduleVersion = ""

    def prepare_firmware_components(self):
        self.rd53_file = {}
        self.initializeRD53Dict()
        self.iref_match_status = {
            module.getModuleName(): True for module in self.modules
        }

    def configure_felis(self):
        felisScratchDir = "/home/cmsTkUser/Ph2_ACF_GUI/data/scratch"
        if not os.path.isdir(felisScratchDir):
            self.create_directory(felisScratchDir)
        self.felis = Felis(felisScratchDir, False)

    def create_directory(self, directory):
        try:
            os.makedirs(directory)
            logger.info(f"New directory created: {directory}")
        except OSError as e:
            logger.error(f"Error making directory: {e.strerror}")
            raise OSError

    def setup_processing_variables(self):
        self.processingFlag = False
        self.ProgresBarList = []
        self.input_dir = ""
        self.output_dir = ""
        self.config_file = ""
        self.grade = -1
        self.currentTest = ""
        self.outputFile = ""
        self.errorFile = ""
        self.comment = ""
        self.autoSave = False
        self.halt = False
        self.runNext = threading.Event()
        self.testIndexTracker = 0
        self.testsAttempted = 0
        self.listWidgetIndex = 0
        self.outputDirQueue = []
        self.readingOutput = False
        self.starttime = None
        self.ProgressingMode = "None"
        self.ProgressValue = 0
        self.IVProgressValue = 0
        self.SLDOProgressValue = 0

    def setup_process_connections(self):
        self.run_processes = [self.create_process(i) for i in range(len(self.firmware))]
        self.fw_processes = [
            self.create_process(i, fw_mode=True) for i in range(len(self.firmware))
        ]

    def create_process(self, index, fw_mode=False):
        process = QProcess()
        process.readyReadStandardOutput.connect(
            lambda: self.on_readyReadStandardOutput(index)
        )
        if fw_mode:
            process.readyReadStandardOutput.connect(
                lambda: self.on_readyReadStandardOutput_info(index)
            )
        process.finished.connect(
            lambda exitCode, exitStatus: self.finished_run_process(
                exitCode, exitStatus, index
            )
        )
        return process

    def finished_run_process(self, _, exitStatus, index):
        if exitStatus == QProcess.NormalExit:
            self.on_finish(index)

    def initializeRD53Dict(self):
        self.rd53_file = {}
        for module in self.modules:
            ogId = module.getOpticalGroup().getOpticalGroupID()
            beboardId = module.getOpticalGroup().getBeBoard().getBoardID()
            moduleName = module.getModuleName()
            moduleId = module.getFMCPort()
            moduleType = module.getModuleType()
            for i in ModuleLaneMap[moduleType].keys():
                self.rd53_file[
                    f"{moduleName}_{moduleId}_{ModuleLaneMap[moduleType][i]}"
                ] = None
            fwPath = f"{beboardId}_{ogId}_{moduleId}"
            self.ModuleMap[fwPath] = moduleName
            print(f"module map is {fwPath}:{self.ModuleMap[fwPath]}")

    def config_output_dir(self, testName):
        ModuleIDs = [str(module.getModuleName()) for module in self.modules]
        return ConfigureTest(
            testName, "_Module".join(ModuleIDs), self.output_dir, self.input_dir
        )

    def configTest(self, **kwargs):
        self.get_run_number_from_file()
        self.set_current_test_name()
        self.output_dir, self.input_dir = self.config_output_dir(self.currentTest)
        self.setup_rd53_file_keys()
        self.setup_xml_config(kwargs)
        self.initializeRD53Dict()
        self.config_file = ""

    def get_run_number_from_file(self):
        try:
            RunNumberFileName = (
                f"{os.environ.get('PH2ACF_BASE_DIR')}/test/RunNumber.txt"
            )
            if os.path.isfile(RunNumberFileName):
                with open(RunNumberFileName, "r") as runNumberFile:
                    self.RunNumber = runNumberFile.readline().strip()
                    logger.info(f"RunNumber: {self.RunNumber}")
        except OSError:
            logger.warning("Failed to retrieve RunNumber due to OSError")

    def set_current_test_name(self):
        if self.currentTest == "" and isCompositeTest(self.info):
            self.currentTest = self.test_list[0]
        elif self.currentTest is None:
            self.currentTest = self.info

    def setup_rd53_file_keys(self):
        for key in self.rd53_file.keys():
            if self.rd53_file[key] is None:
                self.rd53_file[key] = (
                    f"{os.environ.get('PH2ACF_BASE_DIR')}/settings/RD53Files/CMSIT_{self.boardType}{self.moduleVersion}.txt"
                )
                print(f"Getting config file {self.rd53_file[key]}")

    def setup_xml_config(self, kwargs):
        if self.input_dir == "":
            SetupRD53ConfigfromFile(self.rd53_file, self.output_dir)
            self.create_xml_config_if_needed(kwargs)
        else:
            SetupRD53Config(self.input_dir, self.output_dir, self.rd53_file)
            if self.config_file != "":
                for firmware in self.firmware:
                    SetupXMLConfigfromFile(
                        self.config_file, self.output_dir, firmware.getBoardName()
                    )

    def create_xml_config_if_needed(self, kwargs):
        if self.config_file == "":
            tmpDir = os.path.join(os.environ.get("GUI_dir"), "Gui/.tmp")
            if not os.path.isdir(tmpDir):
                try:
                    os.mkdir(tmpDir)
                    logger.info(f"Creating {tmpDir}")
                except OSError:
                    logger.warning(f"Failed to create {tmpDir}")
            for firmware in self.firmware:
                config_file = GenerateXMLConfig(
                    firmware, self.currentTest, tmpDir, self.txt_files, **kwargs
                )
                if config_file:
                    SetupXMLConfigfromFile(
                        config_file, self.output_dir, firmware.getBoardName()
                    )
                else:
                    logger.warning("No Valid XML configuration file")

    def saveConfigs(self):
        for key in self.rd53_file.keys():
            self.copy_config_file(key)

    def copy_config_file(self, key):
        try:
            os.system(
                f"cp {os.environ.get('PH2ACF_BASE_DIR')}/test/CMSIT_RD53_{key}.txt {self.output_dir}/CMSIT_RD53_{key}_OUT.txt"
            )
        except OSError:
            logger.error(
                f"Failed to copy {os.environ.get('PH2ACF_BASE_DIR')}/test/CMSIT_RD53_{key}.txt to {self.output_dir}/CMSIT_RD53_{key}_OUT.txt"
            )

    def resetConfigTest(self):
        self.input_dir = ""
        self.output_dir = ""
        self.config_file = ""
        self.initializeRD53Dict()

    def runTest(self, reRun=False):
        if reRun:
            self.halt = False
            self.testIndexTracker = 0
            self.testsAttempted = 0
        testName = self.info
        self.input_dir = self.output_dir
        self.output_dir = ""

        if isCompositeTest(testName):
            self.runCompositeTest(testName)
        elif isSingleTest(testName):
            self.runSingleTest(testName)
        else:
            QMessageBox.information(None, "Warning", "Not a valid test", QMessageBox.Ok)

    def runCompositeTest(self, testName):
        if self.halt:
            return
        runTestList = self.test_list
        if self.testIndexTracker < len(runTestList):
            testName = runTestList[self.testIndexTracker]
            nextTest = (
                runTestList[self.testIndexTracker + 1]
                if self.testIndexTracker + 1 < len(runTestList)
                else None
            )
            self.runSingleTest(testName, nextTest)

    def ramp_progress_bar(self, max):
        voltages = [
            getattr(module["hv"], "voltage")
            for module in self.instruments._module_dict.values()
        ]
        for i, bar in enumerate(self.runwindow.RampProgressBars):
            text = f"{np.abs(voltages[i])} V"
            value = 100 * np.abs(voltages[i] / max[i]) if max[i] != 0 else 0
            self.updateProgressBar.emit(bar, value, text)

    def runADC(self):
        if "adc_board" in self.instruments._instrument_dict.keys():
            self.adc_board = self.instruments._instrument_dict["adc_board"]
            logger.info("Running with ADC.")
            self.adc_board.__enter__()
        else:
            logger.error(
                "You do not have instruments required to run a Trimbit scan connected. You must have an Adc Board."
            )

    def GADC_execute_each_step(
        self,
        upOrDown: str,
        total_steps: int,
        physics_seconds: int = site_settings.SLDOScan_GADC["physics seconds"],
        fc7_index: int = 0,
    ) -> None:
        GADC_processes = [
            self.create_GADC_process(i, upOrDown, physics_seconds, fc7_index)
            for i in range(len(self.instruments._module_dict))
        ]
        self.wait_for_process_completion(GADC_processes)
        self.ProgressValue += 1
        for i in range(len(self.firmware)):
            self.runwindow.ResultWidget.ProgressBars[i][self.testIndexTracker].setValue(
                100 * self.ProgressValue / total_steps
            )

    def create_GADC_process(self, i, upOrDown, physics_seconds, fc7_index):
        process = QProcess()
        voltage = getattr(
            tuple(self.instruments._module_dict.values())[i]["lv"], "voltage"
        )
        current = getattr(
            tuple(self.instruments._module_dict.values())[i]["lv"], "current"
        )
        print(f"Beginning physics test at {voltage}V and {current}A")
        self.outputString.emit(
            f"Beginning physics test at {voltage}V and {current}A",
            self.runwindow.ConsoleViews[fc7_index],
        )
        process.setProcessChannelMode(QtCore.QProcess.MergedChannels)
        process.setWorkingDirectory(f"{os.environ.get('PH2ACF_BASE_DIR')}/test/")
        process.readyReadStandardOutput.connect(
            lambda: self.on_readyReadStandardOutput_GADC(
                process,
                i,
                upOrDown,
                current,
                channel=tuple(self.instruments._module_dict.keys())[i],
            )
        )
        process.start(
            "CMSITminiDAQ",
            [
                "-f",
                f"CMSIT_{self.firmware[fc7_index].getBoardName()}.xml",
                "-c",
                "physics",
                "-t",
                str(physics_seconds),
            ],
        )
        return process

    def wait_for_process_completion(self, GADC_processes):
        for process in GADC_processes:
            if process.state() != QProcess.NotRunning:
                result = process.waitForFinished(-1)  # waits indefinitely
                if not result:
                    process.kill()

    def runSingleTest(self, testName, nextTest=None):
        if "analyze" in testName.lower():
            self.output_dir, self.input_dir = self.config_output_dir(testName)
            self.currentTest = testName
            EnableReRun = self.onFinalTest(self.testIndexTracker + 1)
            self.stepFinished.emit(EnableReRun)
            if self.master.expertMode:
                self.updateResult.emit(self.output_dir)
            else:
                step = f"{self.testIndexTracker}:{self.currentTest}"
                self.updateResult.emit((step, self.figurelist))
            for i in range(len(self.firmware)):
                self.runwindow.ResultWidget.ProgressBars[i][
                    self.testIndexTracker
                ].setValue(100)
            return

        print("Executing Single Step test...")
        for console in self.runwindow.ConsoleViews:
            self.outputString.emit("Executing Single Step test...", console)

        self.currentTest = testName

        self.updateOptimizedXMLValues()
        self.configTest()

        print("Ran configTest()")

        self.outputFile = os.path.join(self.output_dir, "output.txt")
        self.errorFile = os.path.join(self.output_dir, "error.txt")

        if testName == "SLDOScan_GADC":
            self.run_SLDOScan_GADC()

        if self.instruments:
            print("Right before turnin on LV")
            self.turn_on_lv_power_supply()

        if "IVCurve" in testName:
            self.run_IVCurve(testName, nextTest)

        elif testName == "SLDOScan":
            self.run_SLDOScan(nextTest)

        self.tempHistory = [0.0] * self.numChips
        self.tempindex = 0
        if "TrimbitScan" in testName:
            self.currentTest = testName
            total_steps = 16
            self.trimbitHandler = TrimbitCurveHandler(
                instrument_cluster=self.instruments,
                moduleType=self.ModuleType,
                total_steps=total_steps,
                runwindow=self.runwindow,
                firmware=self.firmware,
                testhandler=self,
            )
            self.trimbitHandler.makeplotSignal.connect(self.storeTrimbitResults)
            self.trimbitHandler.progressSignal.connect(self.updateProgress)
            self.trimbitHandler.finishedSignal.connect(self.TrimbitScanFinished)
            self.trimbitHandler.abortSignal.connect(self.urgentStop)
            self.trimbitHandler.TrimbitScan()
            return
        else:
            print("Right before setupQProcess")
            self.setup_run_processes()

    def updateOptimizedXMLValues(self):
        print("trying to update the xml value")
        try:
            if Test_to_Ph2ACF_Map[self.currentTest] in optimizationTestMap.keys():
                updatedFEKeys = optimizationTestMap[
                    Test_to_Ph2ACF_Map[self.currentTest]
                ]
                for module in self.modules:
                    chipIDs = [
                        chip.getID()
                        for chip in module.getChips().values()
                        if chip.getStatus()
                    ]

                    hybridID = module.getFMCPort()
                    print("HybridID {0}".format(hybridID))
                    print("chipIDs {0}".format(chipIDs))
                    CROC = "CROC" in module.getModuleType()
                    for chipID in chipIDs:
                        updatedXMLValues[f"{hybridID}/{chipID}"] = {}
                        for updatedFEKey in updatedFEKeys:
                            if CROC:
                                if updatedFEKey in [
                                    "LATENCY_CONFIG",
                                    "Vthreshold_LIN",
                                ]:  # registers not on CROC modules
                                    continue
                            elif not CROC:
                                if updatedFEKey in [
                                    "TriggerConfig",
                                    "DAC_GDAC_",
                                    "CAL_EDGE_FINE_DELAY",
                                ]:  # registers only on CROC modules
                                    continue
                            updatedXMLValues[f"{hybridID}/{chipID}"][updatedFEKey] = ""
        except Exception as err:
            logger.error(f"Failed to update, {err}")

    def run_SLDOScan_GADC(self):
        starting_voltages = [
            np.abs(getattr(module["hv"], "voltage"))
            for module in self.instruments._module_dict.values()
        ]
        self.instruments.hv_off(
            execute_each_step=lambda: self.ramp_progress_bar(starting_voltages)
        )

        SLDOScan_GADC_dict = (
            site_settings.SLDOScan_GADC["1x2"]
            if "1x2" in self.ModuleType.lower()
            else site_settings.SLDOScan_GADC["quad"]
        )
        self.ProgressValue = 0
        total_steps = 2 * (
            1
            + np.ceil(
                np.abs(
                    SLDOScan_GADC_dict["target current"]
                    - SLDOScan_GADC_dict["starting current"]
                )
                / SLDOScan_GADC_dict["step size"]
            )
        )

        for i in range(len(self.firmware)):
            self.runwindow.ResultWidget.ProgressBars[i][self.testIndexTracker].setValue(
                0
            )

        self.instruments.lv_on(
            voltage=SLDOScan_GADC_dict["voltage"],
            current=SLDOScan_GADC_dict["starting current"],
        )

        up_sweep = self.instruments.lv_sweep(
            target=SLDOScan_GADC_dict["target current"],
            delay=0.1,
            set_property="current",
            measure=True,
            step_size=SLDOScan_GADC_dict["step size"],
            execute_each_step=lambda: self.GADC_execute_each_step("up", total_steps),
        )
        down_sweep = self.instruments.lv_sweep(
            target=SLDOScan_GADC_dict["starting current"],
            delay=0.1,
            set_property="current",
            measure=True,
            step_size=SLDOScan_GADC_dict["step size"],
            execute_each_step=lambda: self.GADC_execute_each_step("down", total_steps),
        )
        self.instruments.lv_off()

        for i in range(len(self.firmware)):
            self.runwindow.ResultWidget.ProgressBars[i][self.testIndexTracker].setValue(
                100
            )

        for datatype in ("VDDD", "VDDA"):
            self.process_sldo_data(datatype, up_sweep, down_sweep)

        self.SLDOScanFinished()

    def SLDOScanFinished(self):
        for module in self.modules:
            ogId = module.getOpticalGroup().getOpticalGroupID()
            beboardId = module.getOpticalGroup().getBeBoard().getBoardID()
            moduleName = module.getModuleName()
            hybridId = module.getFMCPort()
            module_canvas_path = (
                "Detector/Board_{boardID}/OpticalGroup_{ogID}/Hybrid_{hybridID}".format(
                    boardID=beboardId, ogID=ogId, hybridID=hybridId
                )
            )

            SLDO_CSV_to_ROOT(
                moduleName, module_canvas_path, self.SLDOfilelist, self.output_dir
            )

        self.validateTest()
        self.testIndexTracker += 1
        self.testsAttempted += 1

        EnableReRun = False
        # Will send signal to turn off power supply after composite or single tests are run
        if isCompositeTest(self.info):
            self.instruments.lv_on(
                voltage=site_settings.ModuleVoltageMapSLDO[self.master.module_in_use],
                current=site_settings.ModuleCurrentMap[self.master.module_in_use],
            )
            default_hv_voltage = site_settings.icicle_instrument_setup[
                "instrument_dict"
            ]["hv"]["default_voltage"]
            # assumes only 1 HV titled 'hv' in instruments.json
            self.master.instruments.hv_on(
                voltage=default_hv_voltage,
                delay=0.3,
                step_size=5,
                measure=False,
                execute_each_step=lambda: self.ramp_progress_bar(
                    [default_hv_voltage] * len(self.instruments._module_dict.values())
                ),
            )

            if self.testIndexTracker == len(self.test_list):
                self.powerSignal.emit()
                EnableReRun = True
                if self.autoSave:
                    self.runwindow.upload_to_Panthera_starter()
        elif isSingleTest(self.info):
            EnableReRun = True
            self.powerSignal.emit()
            if self.autoSave:
                self.runwindow.upload_to_Panthera_starter()

        self.stepFinished.emit(EnableReRun)

        self.historyRefresh.emit()
        if self.master.expertMode:
            self.updateSLDOResult.emit(self.output_dir)
        else:
            self.updateSLDOResult.emit(
                ("SLDOScan", self.figurelist)
            )  ##Add else statement to add signal in simple mode

        if isCompositeTest(self.info):
            self.runTest()

    def process_sldo_data(self, datatype, up_sweep, down_sweep):
        for channel in self.instruments._module_dict:
            for chip in self.VDDDup[channel]:
                data = [
                    [sweep_step[-1] for sweep_step in up_sweep[0][1]],
                    [
                        float(i)
                        for i in getattr(self, f"VIN{datatype[-1]}up")[channel][
                            chip
                        ].values()
                    ],
                    [
                        float(i)
                        for i in getattr(self, f"{datatype}up")[channel][chip].values()
                    ],
                    [sweep_step[-1] for sweep_step in down_sweep[0][1]],
                    [
                        float(i)
                        for i in getattr(self, f"VIN{datatype[-1]}down")[channel][
                            chip
                        ].values()
                    ],
                    [
                        float(i)
                        for i in getattr(self, f"{datatype}down")[channel][
                            chip
                        ].values()
                    ],
                ]
                print(data)
                self.makeSLDOPlot(data, f"{datatype}_ROC{int(chip)}")

    def makeSLDOPlot(self, total_result: np.ndarray, pin: str):
        for module in self.modules:
            moduleName = module.getModuleName()
            filename = "{0}/SLDOCurve_Module_{1}_{2}.svg".format(
                self.output_dir, moduleName, pin
            )
            csvfilename = "{0}/SLDOCurve_Module_{1}_{2}.csv".format(
                self.output_dir, moduleName, pin
            )
            self.SLDOfilelist.append(csvfilename)
            # The pin is passed here, so we can use that as the key in the chipmap dict from settings.py
            total_result_stacked = np.vstack(total_result)
            np.savetxt(csvfilename, total_result_stacked, delimiter=",")

            # Make the actual graph
            plt.figure()
            plt.plot(
                total_result_stacked[0],
                total_result_stacked[1],
                "-x",
                label="module input voltage (up)",
            )
            plt.plot(
                total_result_stacked[0],
                total_result_stacked[2],
                "-x",
                label=f"{pin} (up)",
            )
            plt.plot(
                total_result_stacked[3],
                total_result_stacked[4],
                "-x",
                label="module input voltage (down)",
            )
            plt.plot(
                total_result_stacked[3],
                total_result_stacked[5],
                "-x",
                label=f"{pin} (down)",
            )
            plt.grid(True)
            plt.xlabel("Current (A)")
            plt.ylabel("Voltage (V)")
            plt.legend()
            plt.savefig(filename)

            self.figurelist[moduleName] = [filename]

    def turn_on_lv_power_supply(self):
        lv_on = False
        for number in self.instruments.get_modules().keys():
            if self.instruments.status()[number]["lv"]:
                lv_on = True
                break
        if not lv_on:
            self.instruments.lv_on(
                voltage=site_settings.ModuleVoltageMapSLDO[self.ModuleType],
                current=site_settings.ModuleCurrentMap[self.ModuleType],
            )

    def run_IVCurve(self, testName, nextTest):
        self.currentTest = testName
        self.configTest()
        self.IVCurveData = []
        self.IVProgressValue = 0
        self.IVCurveHandler = IVCurveHandler(
            self.currentTest,
            self.instruments,
            nextTest,
            execute_each_step=self.ramp_progress_bar,
        )
        self.IVCurveHandler.finished.connect(self.IVCurveFinished)
        self.IVCurveHandler.progressSignal.connect(self.updateProgress)
        self.IVCurveHandler.startSignal.connect(self.setupQProcess)

        for console in self.runwindow.ConsoleViews:
            self.outputString.emit("Beginning IVCurve", console)
        self.IVCurveHandler.IVCurve()

    def run_SLDOScan(self, nextTest):
        self.configTest()
        self.SLDOScanData = []
        self.SLDOProgressValue = 0
        self.SLDOScanHandler = SLDOCurveHandler(
            self.instruments,
            moduleType=self.ModuleType[5:],
            end_current=site_settings.ModuleCurrentMap[self.master.module_in_use],
            voltage_limit=site_settings.ModuleVoltageMapSLDO[self.master.module_in_use],
            execute_each_step=self.ramp_progress_bar,
        )
        self.SLDOScanHandler.makeplotSignal.connect(self.makeSLDOPlot)
        self.SLDOScanHandler.finishedSignal.connect(self.SLDOScanFinished)
        self.SLDOScanHandler.progressSignal.connect(self.updateProgress)
        self.SLDOScanHandler.abortSignal.connect(self.urgentStop)

        for console in self.runwindow.ConsoleViews:
            self.outputString.emit("Beginning SLDOScan", console)
        self.SLDOScanHandler.SLDOScan()

    def setup_run_processes(self):
        self.tempHistory = [0.0] * self.numChips
        self.tempindex = 0
        self.outputFile = os.path.join(self.output_dir, "output.txt")
        self.errorFile = os.path.join(self.output_dir, "error.txt")

        logger.debug("Setup run_processes")
        print("Setup run processes")
        for process in self.run_processes:
            process.setProcessChannelMode(QtCore.QProcess.MergedChannels)
            process.setWorkingDirectory(
                os.path.join(os.environ.get("PH2ACF_BASE_DIR"), "test/")
            )

        logger.info("About to run %s", self.currentTest)
        print("About to run", self.currentTest)
        if self.currentTest == "CommunicationTest":
            for process, firmware in zip(self.run_processes, self.firmware):
                process.start(
                    "CMSITminiDAQ", ["-f", f"CMSIT_{firmware.getBoardName()}.xml", "-p"]
                )
        if self.currentTest == "IREF_GADC":
            for process, firmware in zip(self.run_processes, self.firmware):
                process.start(
                    "CMSITminiDAQ",
                    [
                        "-f",
                        f"CMSIT_{firmware.getBoardName()}.xml",
                        "-c",
                        "{}".format(Test_to_Ph2ACF_Map[self.currentTest]),
                    ],
                )
        elif self.currentTest == "TrimbitScan":
            for process, firmware in zip(self.run_processes, self.firmware):
                process.start(
                    "CMSITminiDAQ", ["-f", f"CMSIT_{firmware.getBoardName()}.xml"]
                )
        else:
            for process, firmware in zip(self.run_processes, self.firmware):
                process.start(
                    "CMSITminiDAQ",
                    [
                        "-f",
                        f"CMSIT_{firmware.getBoardName()}.xml",
                        "-c",
                        "{}".format(Test_to_Ph2ACF_Map[self.currentTest]),
                    ],
                )

    @QtCore.pyqtSlot()
    def on_finish(self, processIndex: int):
        if self.halt:
            self.haltSignal.emit(True)
            return

        if self.run_processes[processIndex].state() == QProcess.Running:
            print(
                "process is still running...  Attempting to terminate before next test."
            )
            self.run_processes[processIndex].terminate()
            if not self.run_processes[processIndex].waitForFinished(3000):
                print("process would not terminate, so killing it now...")
                self.run_processes[processIndex].kill()

        if "IVCurve" in self.currentTest:
            self.saveTest(processIndex, self.run_processes[processIndex])
            return

        # Save the output ROOT file to output_dir

        self.saveTest(processIndex, self.run_processes[processIndex])

        # validate the results
        self.validateTest()

        self.testIndexTracker += 1
        self.testsAttempted += 1

        EnableReRun = self.onFinalTest(
            self.testIndexTracker
        )  # This function uses BBanalysis_root_files when all composite tests will not make use of it
        self.stepFinished.emit(EnableReRun)

        # show the score of test
        self.historyRefresh.emit()
        if self.master.expertMode:
            self.updateResult.emit(self.output_dir)
        else:
            step = "{}:{}".format(self.testIndexTracker, self.currentTest)
            self.updateResult.emit((step, self.figurelist))

        if isCompositeTest(self.info):
            self.runTest()

    def onFinalTest(self, index):
        for process in self.run_processes:
            if process.state() == QProcess.Running:
                return

        EnableReRun = False
        # Will send signal to turn off power supply after composite or single tests are run
        if isCompositeTest(self.info):
            if index == len(
                self.test_list
            ):  # Checks that this was the last test in the sequence.
                self.powerSignal.emit()
                EnableReRun = True

                if self.info == "FWD-RVS Bias" or self.info == "CrossTalk":
                    self.bumpbond_analysis()

                if (
                    len(self.BBanalysis_root_files) > 0
                    and "analyze" in self.currentTest
                ):
                    for beboard in self.firmware:
                        boardID = beboard.getBoardID()
                        for OG in beboard.getAllOpticalGroups().values():
                            ogID = OG.getOpticalGroupID()
                            for module in OG.getAllModules().values():
                                hybridID = module.getFMCPort()
                                module_data = {
                                    "boardID": boardID,
                                    "ogID": ogID,
                                    "hybridID": hybridID,
                                    "module": module,
                                }
                                index -= 1
                                self.felis.set_result(
                                    self.BBanalysis_root_files,
                                    module_data["module"].getModuleName(),
                                    f"{index:02d}_CrossTalk",
                                    "crosstalk",
                                )
                                self.figurelist[module.getModuleName()] = (
                                    self.collect_plots(module.getModuleName())
                                )
                if self.autoSave:
                    self.runwindow.upload_to_Panthera_starter()

        elif isSingleTest(self.info):
            EnableReRun = True
            self.powerSignal.emit()
            if self.autoSave:
                self.runwindow.upload_to_Panthera_starter()

        return EnableReRun

    def abortTest(self):
        self.halt = True
        for process in self.run_processes:
            process.kill()

        self.haltSignal.emit(self.halt)
        self.starttime = None
        self.testsAttempted = 0

        if self.IVCurveHandler:
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("Aborting IVCurve", console)
            self.IVCurveHandler.stop()
        if self.SLDOScanHandler:
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("Aborting SLDOScan", console)
            self.SLDOScanHandler.stop()
        if self.trimbitHandler:
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("Aborting TrimbitScan", console)
            self.trimbitHandler.stop()

    def urgentStop(self):
        for process in self.run_processes:
            process.kill()
        self.halt = True
        self.haltSignal.emit(self.halt)
        self.starttime = None

    def validateTest(self):
        for process in self.run_processes:
            if process.state() == QProcess.Running:
                return

        self.finished_tests.append(self.currentTest)
        try:
            passed = []
            results = []
            runNumber = "000000" if self.RunNumber == "-1" else self.RunNumber

            for beboard in self.firmware:
                boardID = beboard.getBoardID()
                for OG in beboard.getAllOpticalGroups().values():
                    ogID = OG.getOpticalGroupID()
                    for module in OG.getAllModules().values():
                        hybridID = module.getFMCPort()
                        module_data = {
                            "boardID": boardID,
                            "ogID": ogID,
                            "hybridID": hybridID,
                            "module": module,
                        }
                        result, self.BBanalysis_root_files = ResultGrader(
                            self.felis,
                            self.output_dir,
                            self.currentTest,
                            self.testIndexTracker,
                            runNumber,
                            module_data,
                            self.BBanalysis_root_files,
                            self.info,
                            self.registerKey,
                            self.communicationTestResults,
                            self.comment,
                            self.iref_match_status,
                        )

                        results.append(result)
                        passed.append(list(result.values())[0][0])

                        self.module_test_history[next(iter(result))][self.currentTest][
                            "Passed" if next(iter(result.values()))[0] else "Failed"
                        ] += 1

                        self.figurelist[module.getModuleName()] = self.collect_plots(
                            module.getModuleName()
                        )

            self.updateValidation.emit(results)
            self.updateFinishedTests.emit(self.finished_tests)
        except Exception as err:
            logger.error(err)

    def collect_plots(self, moduleName):
        try:
            plot_paths = []
            scratch = os.path.join(self.felis.path_scratch, moduleName)
            test = f"{self.testIndexTracker:02d}_{self.currentTest}"
            directory = os.path.join(scratch, test)

            for filename in os.listdir(directory):
                if filename.lower().endswith(".svg") or filename.lower().endswith(
                    ".png"
                ):
                    plot_paths.append(os.path.join(directory, filename))

            return plot_paths
        except Exception as e:
            if "IVCurve" in self.currentTest or "SLDOScan" in self.currentTest:
                return self.figurelist.get(moduleName, [])
            else:
                logger.error(f"collect_plots Exception: {repr(e)}")
                return []

    def saveTest(self, processIndex: int, process: QProcess):
        if process.state() == QProcess.Running:
            QMessageBox.critical(self, "Error", "Process not finished", QMessageBox.Ok)
            return

        try:
            if self.RunNumber == "-1":
                os.system(
                    f"cp {os.environ.get('PH2ACF_BASE_DIR')}/test/Results/Run000000*.root {self.output_dir}/"
                )
            elif "IVCurve" in self.currentTest or "IREF_GADC" in self.currentTest:
                logger.info("Copying MonitorDQM.root file to output directory")
                os.system(
                    f"cp {os.environ.get('PH2ACF_BASE_DIR')}/test/Results/Run{self.RunNumber}_MonitorDQM.root {self.output_dir}/"
                )
            else:
                print("Ph2_ACF Base Directory:", os.environ.get("PH2ACF_BASE_DIR"))
                for fc7 in self.firmware:
                    self.copyMostRecentRootFile(
                        self.RunNumber,
                        os.path.join(
                            os.environ.get("PH2ACF_BASE_DIR"),
                            f"test/{fc7.getBoardName()}",
                        ),
                        self.output_dir,
                        self.currentTest,
                    )
        except Exception as e:
            logger.error(e)
            if self.currentTest != "CommunicationTest":
                self.forceContinue(self.firmware[processIndex])

    def forceContinue(
        self, board
    ):  # board:QtBeBoard. Runs when module disconnection is suspected.
        fc7modules = board.getModules()

        # Create the main widget
        self.force_continue_window = QDialog()
        self.force_continue_window.setMaximumWidth(375)
        self.force_continue_window.setWindowTitle(
            f"{board.getBoardName()}: Failed Component Detected"
        )

        # Create layout
        self.force_continue_window.layout = QVBoxLayout(self.force_continue_window)

        # Add custom buttons
        exit_button = QPushButton("Exit")
        retry_button = QPushButton("Retry")
        continue_button = QPushButton("Continue")

        self.force_continue_window.layout.addWidget(exit_button)
        self.force_continue_window.layout.addWidget(retry_button)
        self.force_continue_window.layout.addWidget(continue_button)

        # Create table for modules with checkboxes
        self.force_continue_window.table = QTableWidget(len(fc7modules), 2)
        self.force_continue_window.table.setHorizontalHeaderLabels(
            ["Module", "Enabled"]
        )

        for row, module in enumerate(fc7modules):
            # Set the module name in the first column
            module_name = module.getModuleName()
            self.force_continue_window.table.setItem(
                row, 0, QTableWidgetItem(module_name)
            )

            # Create a QTableWidgetItem for the checkbox in the second column
            checkbox_item = QTableWidgetItem()
            checkbox_item.setCheckState(
                Qt.Checked
            ) if module.getEnabled() == "1" else checkbox_item.setCheckState(
                Qt.Unchecked
            )
            self.force_continue_window.table.setItem(row, 1, checkbox_item)

        self.force_continue_window.layout.addWidget(self.force_continue_window.table)
        self.force_continue_window.setLayout(self.force_continue_window.layout)

        self.force_continue_window.abort = True

    def copyMostRecentRootFile(self, RunNumber, base_dir, output_dir, test):
        files = root_files[test] if test in root_files.keys() else (test,)
        for name in files:
            name = name.split("_")[0]
            if "SCurveScan" in name:
                name = "SCurve"
            elif "GainScan" in name:
                name = "Gain"
            elif "Threshold" in name:
                name = name.replace("Threshold", "Thr")

            search_pattern = f"{base_dir}/Run{RunNumber}_{name}.root"
            logger.debug(f"Looking for {search_pattern}")
            print(f"Looking for {search_pattern}")

            matching_files = glob.glob(search_pattern)
            if len(matching_files) == 0:
                raise Exception(
                    f"Failed to copy root file to output directory. Module disconnection detected because Ph2_ACF didn't create {search_pattern}"
                )

            latest_file = max(matching_files, key=os.path.getmtime)

            if os.path.getsize(latest_file) == 0:
                raise Exception(
                    f"Failed to copy root file to output directory. Module disconnection detected because {latest_file} created by Ph2_ACF is empty."
                )

            os.system(f"cp {latest_file} {output_dir}/")

    @QtCore.pyqtSlot()
    def on_readyReadStandardOutput(self, processIndex: int):
        if getattr(self, "readingOutput", False):
            logger.warning("Thread competition detected")
            return
        self.readingOutput = True

        alltext = (
            self.run_processes[processIndex].readAllStandardOutput().data().decode()
        )
        mode = "a" if os.path.exists(self.outputFile) else "w"
        with open(self.outputFile, mode) as outputfile:
            outputfile.write(alltext)

        self.process_standard_output(alltext, processIndex)
        self.readingOutput = False

    def process_standard_output(self, alltext, processIndex):
        for textStr in alltext.split("\n"):
            if self.starttime is not None:
                self.currentTime = time.time()
                runningTime = self.currentTime - self.starttime
                self.runwindow.ResultWidget.runtimes[processIndex][
                    self.testIndexTracker
                ].setText(f"{round(runningTime, 1)} s")
            else:
                self.starttime = time.time()

            text = textStr.encode("ascii")
            _, text = parseANSI(text)
            self.parse_output(text)
            self.readingOutput = False
            self.outputString.emit(
                text.decode("utf-8"), self.runwindow.ConsoleViews[processIndex]
            )

    def parse_output(self, text: str):
        self.parse_iref_fused_id(text)
        self.parse_progress(text)
        self.parse_sensor_temperature(text)
        self.parse_NTC_temperature(text)
        self.parse_communication_status(text)

    def parse_iref_fused_id(self, text: str) -> dict[str, object]:
        output = {
            "fused_id": None,
            "iref_value": None,
            "chip_number": None,
            "hybrid_id": None,
        }

        # Find iref value if there
        # hybrid refers to the FMC port which there can be multiple if testing
        # multiple modules

        # If in statement is faster than regex so only run regex to capture value if necessary
        if "Configuring chips of hybrid" in text:
            hybrid_id: Optional[re.Match[str]] = re.search(
                r"Configuring chips of hybrid: (\d+)$", text
            )
            if hybrid_id:
                self.mod_dict[hybrid_id] = {}
                self.fused_dict_index[0] = hybrid_id
                logger.info("Hyrid ID: %s", hybrid_id)
            else:
                logger.warning("Unable to gather hybrid ID")

        if "Configuring RD53:" in text:
            chip_number: Optional[re.Match[str]] = re.search(
                r"Configuring RD53: (\d+)$", text
            )
            if chip_number:
                self.fused_dict_index[1] = chip_number
                logger.info("Chip Number: %s", chip_number)
            else:
                logger.warning("Unable to gather chip_number")
        if "Wire bonded Iref =" in text:
            iref_value: Optional[re.Match[str]] = re.search(
                r"Wire bonded Iref = (\d+)$", text
            )

            # The values for hybrid_id and iref won't be in the same block of text
            # so we need to reference mod_dict and not just use hybrid_id
            self.mod_dict[self.fused_dict_index[0]][self.fused_dict_index[1]] = (
                iref_value
            )
            hybrid_id = self.fused_dict_index[0]
            chip_number = self.fused_dict_index[1]
            module_name = self.modules[0].getModuleName()
            db_iref = chip_iref_db.get(str(chip_number))
            logger.info(
                "IREF value for chip %s on FMC Port %s : %s",
                chip_number,
                hybrid_id,
                iref_value,
            )

            if module_name not in self.iref_match_status:
                self.iref_match_status[module_name] = True

            if db_iref:
                if db_iref != iref_value:
                    logger.error(
                        "Mismatch in IREF for chip %s on FMC port %s (database: %s, module: %s)",
                        chip_number,
                        hybrid_id,
                        db_iref,
                        iref_value,
                    )
                    self.iref_match_status[module_name] = False
            else:
                logger.warning(
                    "No database IREF for chip %s on FMC port %s",
                    chip_number,
                    hybrid_id,
                )
                self.iref_match_status[module_name] = False

        if "Fused ID:" in text:
            fuse_id: Optional[re.Match[str]] = re.search(r"Fused ID: (\d+)$", text)

            if fuse_id:
                self.mod_dict[self.fused_dict_index[0]][self.fused_dict_index[1]] = (
                    fuse_id
                )
            else:
                logger.error("Unable to gather fuse ID")

    def parse_progress(self, text: str, process_index: int):
        """
        Parse Ph2_ACF output for progress of current test. Used to update progress bars

        text: output from Ph2_ACF process. This expects ANSI symbols to have already been removed
        process_index: Index that refers to which Ph2_ACF instance we are looking at
        """
        if "@@@ End of CMSIT miniDAQ @@@" in text:
            self.ProgressingMode = "Summary"
        if self.ProgressingMode == "Perform":
            if "Progress" in text:
                try:
                    progress: Optional[re.Match[str]] = re.search(
                        r"Progress:\s+([0-9]*\.?[0-9]+)%", text
                    )
                    if progress:
                        self.ProgressValue = float(progress)
                    self.runwindow.ResultWidget.ProgressBars[process_index][
                        self.testIndexTracker
                    ].setValue(self.ProgressValue)
                except Exception as e:
                    logger.error("Error while updating progress bar")
                    traceback.print_exc()

    def parse_sensor_temperature(self, text: str):
        if "TEMPSENS_" not in text:
            return
        try:
            output = text.split("[")
            sensor = output[8]
            sensorMeasure = sensor[3:]

            if sensorMeasure != "" or sensorMeasure != "44.086 +/- 1.763 °C":
                temp = float(sensorMeasure.split("+")[0].strip())
                self.tempHistory[self.tempindex] = temp
                if any(
                    num > site_settings.Warning_Threshold for num in self.tempHistory
                ):
                    self.runwindow.updateTempIndicator("orange")
                elif any(
                    num > site_settings.Emergency_Threshold for num in self.tempHistory
                ):
                    self.runwindow.updateTempIndicator("red")
                else:
                    self.runwindow.updateTempIndicator("green")
            else:
                # bad reading
                self.tempHistory[self.tempindex] = 0.0
                if not all(self.tempHistory):
                    self.runwindow.updateTempIndicator("off")
                self.tempindex = self.tempindex + 1 % self.numChips

        except Exception:
            logger.error("Could not update sensor temperature")
            traceback.print_exc()

    def parse_NTC_temperature(self, text: str):
        if "INTERNAL_NTC" not in text:
            return
        try:
            sensor_temperature = (
                text.split("INTERNAL_NTC:")[1].strip().split("C")[0].strip()
            )
            sensor_temperature += " °C"
            sensor_temperature = sensor_temperature.replace("+-", "+/-")

            if sensor_temperature != "" or sensor_temperature != "44.086 +/- 1.763 °C":
                temp = float(sensor_temperature.split("+")[0].strip())
                self.tempHistory[self.tempindex] = temp
                if any(
                    num > site_settings.Warning_Threshold for num in self.tempHistory
                ):
                    self.runwindow.updateTempIndicator("orange")
                elif any(
                    num > site_settings.Emergency_Threshold for num in self.tempHistory
                ):
                    self.runwindow.updateTempIndicator("red")
                else:
                    self.runwindow.updateTempIndicator("green")
            else:
                # bad reading
                self.tempHistory[self.tempindex] = 0.0
                if not all(self.tempHistory):
                    self.runwindow.updateTempIndicator("off")
            self.tempindex = (self.tempindex + 1) % self.numChips

        except:
            logger.error("Failed to read NTC temperature")
            traceback.print_exc()

    def parse_communication_status(self, text):
        match = re.search(r"CMSIT_RD53_([^_]+)", text)
        if match:
            if self.communicationTestModule is not None:
                self.communicationTestResults[self.communicationTestModule] = True
            self.communicationTestModule = match.group(1)

        if self.currentTest == "CommunicationTest":
            if (
                "Error, some data lanes are enabled but inactive, reached maximum number of attempts"
                in alltext
            ):
                if self.communicationTestModule is None:
                    print(
                        "ERROR: Module name not found before CommunicationTest result in test output."
                    )
                    logger.error(
                        "Module name not found before CommunicationTest result in test output."
                    )
                else:
                    self.communicationTestResults[self.communicationTestModule] = False
                    self.communicationTestModule = None
                self.forceContinue(self.firmware[processIndex])
            elif "All enabled data lanes are active" in alltext:
                if self.communicationTestModule is None:
                    print(
                        "ERROR: Module name not found before CommunicationTest result in test output."
                    )
                    logger.error(
                        "Module name not found before CommunicationTest result in test output."
                    )
                else:
                    self.communicationTestResults[self.communicationTestModule] = True
                    self.communicationTestModule = None

    @QtCore.pyqtSlot()
    def on_readyReadStandardOutput_info(self, processIndex: int):
        alltext = (
            self.info_processes[processIndex].readAllStandardOutput().data().decode()
        )
        mode = "a" if os.path.exists(self.outputFile) else "w"
        with open(self.outputFile, mode) as outputfile:
            outputfile.write(alltext)

        for textStr in alltext.split("\n"):
            self.outputString.emit(textStr, self.runwindow.ConsoleViews[processIndex])
