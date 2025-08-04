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
import traceback
import threading
import time
import re
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
from InnerTrackerTests.Analysis.SLDO_CSV_to_ROOT import (
    SLDO_CSV_to_ROOT,
    Trimbit_CSV_to_ROOT,
)


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
    haltSignal = pyqtSignal(object)  # Used to initiate runwindow.finish()
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
        self.mod_dict = {}
        self.fused_dict_index = [-1, -1]
        self.FWisPresent = False
        self.FWisLoaded = False
        self.master.globalStop.connect(self.urgentStop)
        self.runwindow = runwindow
        self.firmware = firmware
        self.info = info  # This is the name of the test sequence or just the name of the test if it is a single test
        self.ModuleMap = dict()

        self.modules = [
            module for beboard in self.firmware for module in beboard.getModules()
        ]

        self.GADC_meas_chip = None
        self.VDDDup = {channel: {} for channel in self.instruments._module_dict}
        self.VDDDdown = {channel: {} for channel in self.instruments._module_dict}
        self.VDDAdown = {channel: {} for channel in self.instruments._module_dict}
        self.VDDAup = {channel: {} for channel in self.instruments._module_dict}
        self.VINDup = {channel: {} for channel in self.instruments._module_dict}
        self.VINDdown = {channel: {} for channel in self.instruments._module_dict}
        self.VINAdown = {channel: {} for channel in self.instruments._module_dict}
        self.VINAup = {channel: {} for channel in self.instruments._module_dict}

        self.VDDDupError = {channel: {} for channel in self.instruments._module_dict}
        self.VDDDdownError = {channel: {} for channel in self.instruments._module_dict}
        self.VDDAdownError = {channel: {} for channel in self.instruments._module_dict}
        self.VDDAupError = {channel: {} for channel in self.instruments._module_dict}
        self.VINDupError = {channel: {} for channel in self.instruments._module_dict}
        self.VINDdownError = {channel: {} for channel in self.instruments._module_dict}
        self.VINAdownError = {channel: {} for channel in self.instruments._module_dict}
        self.VINAupError = {channel: {} for channel in self.instruments._module_dict}

        self.SLDOfilelist = []

        self.BBanalysis_root_files = []

        self.numChips = len(
            [
                chipID
                for module in self.modules
                for chipID in module.getEnabledChips().keys()
            ]
        )

        self.ModuleType = self.runwindow.ModuleType
        if "CROC" in self.ModuleType:
            self.boardType = "RD53B"
            self.moduleVersion = self.firmware[0].getModuleData()[
                "version"
            ]  # module types/versions should be identical for all modules
            self.hdiVersion = self.firmware[0].getModuleData()["hdiVersion"]
        else:
            self.boardType = "RD53A"
            self.moduleVersion = ""

        self.registerKey = "{0}_HDIv{1}".format(
            self.ModuleType.replace(" ", "_"), self.hdiVersion
        )

        # If the module is not one of the module types in CompositeTests_Modules, use the default test list
        try:
            self.test_list = (
                CompositeTests_Modules[self.registerKey][self.info]
                if isCompositeTest(self.info)
                else (self.info,)
            )
        except KeyError:
            logger.error(
                f"Test {self.info} not found in CompositeTests_Modules for ModuleType {self.registerKey}."
            )
            self.test_list = CompositeTests_Modules["Default"][self.info]

        self.module_test_history = {
            module.getModuleName(): {
                test: {"Passed": 0, "Failed": 0} for test in self.test_list
            }
            for module in self.modules
        }
        self.finished_tests = []
        self.Ph2_ACF_ver = os.environ.get("Ph2_ACF_VERSION")
        print("Using version {0} of Ph2_ACF".format(self.Ph2_ACF_ver))
        self.firmwareImage = firmware_image[self.ModuleType][self.Ph2_ACF_ver]
        print("Firmware version is {0}".format(self.firmwareImage))
        self.RunNumber = "-1"
        self.isTDACtuned = False

        self.IVCurveHandler = None
        self.SLDOScanHandler = None
        self.trimbitHandler = None

        self.processingFlag = False
        self.ProgresBarList = []
        self.input_dir = ""
        self.output_dir = ""
        self.config_file = (
            ""  # os.environ.get('GUI_dir')+ConfigFiles.get(self.calibration, "None")
        )
        self.rd53_file = {}
        self.grade = -1
        self.currentTest = ""
        self.outputFile = ""
        self.errorFile = ""
        self.comment = ""
        self.txt_files = txt_files if txt_files != {} else {}

        self.autoSave = False
        self.backSignal = False
        self.halt = False
        self.finishSignal = False
        self.proceedSignal = False

        self.runNext = threading.Event()
        self.testIndexTracker = 0
        self.testsAttempted = 0
        self.listWidgetIndex = 0
        self.outputDirQueue = []
        # Fixme: QTimer to be added to update the page automatically

        # Need multiple felis instances to have two separate scratch directories
        # to handle multiple FC7s 
        self.felis_instances = [] 
        felisScratchDirBase = "/home/cmsTkUser/Ph2_ACF_GUI/data/scratch"
        felis_directories = [os.path.join(felisScratchDirBase, fc7.getBoardName()) for fc7 in self.firmware]
        try:
            # Create a unique scratch directory for each FC7
            for directory in felis_directories:
                os.makedirs(directory)
                logger.info("New Felis scratch directory created.")

        except FileExistsError:
            # If directory already exists, fantastic.
            logger.debug("The scratch directory already exists. Continuing")
            pass

        except OSError as e:
            logger.error(f"Error making Felis scratch directory: {e.strerror}")
            
        self.felis_instances = [Felis(felis_directory, False) for felis_directory in felis_directories] 
        self.grades = []

        self.figurelist = {}

        self.run_processes = [QProcess() for _ in self.firmware]
        for i, process in enumerate(self.run_processes):
            process.readyReadStandardOutput.connect(
                lambda j=i: self.on_readyReadStandardOutput(j)
            )
            process.finished.connect(
                lambda exitCode, exitStatus, j=i: self.finished_run_process(
                    exitCode, exitStatus, j
                )
            )
        self.finished_processes = 0
        self.readingOutput = False
        self.ProgressingMode = "None"
        self.ProgressValue = 0
        self.IVProgressValue = 0
        self.SLDOProgressValue = 0
        self.runtimeList = []
        self.starttime = None

        self.communicationTestResults = {
            module.getModuleName(): None for module in self.modules
        }
        self.communicationTestModule = None

        self.info_processes = [QProcess() for _ in self.firmware]
        for i, process in enumerate(self.info_processes):
            process.readyReadStandardOutput.connect(
                lambda j=i: self.on_readyReadStandardOutput_info(j)
            )

        ##---Adding firmware setting-----
        self.fw_processes = [QProcess() for _ in self.firmware]
        for i, process in enumerate(self.fw_processes):
            process.readyReadStandardOutput.connect(
                lambda j=i: self.on_readyReadStandardOutput_info(j)
            )

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

        self.finished_tests = []

        self.initializeRD53Dict()
        self.iref_match_status = {
            module.getModuleName(): True for module in self.modules
        }  # Initialize all to True

    def finished_run_process(self, _, exitStatus, i):
        logger.info("Inside finsihed_run_process")
        logger.info("Current exitStatus in finished_run_process: %s", exitStatus)
        if exitStatus == QProcess.NormalExit:
            # Ensure that all processes have finished before continuing
            self.finished_processes += 1
            if self.finished_processes == len(self.run_processes):
                self.finished_processes = 0
                self.on_finish(i)

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
                    "{0}_{1}_{2}".format(
                        moduleName, moduleId, ModuleLaneMap[moduleType][i]
                    )
                ] = None
            fwPath = "{0}_{1}_{2}".format(beboardId, ogId, moduleId)
            self.ModuleMap[fwPath] = moduleName
            print("module map is {0}:{1}".format(fwPath, self.ModuleMap[fwPath]))

    def config_output_dir(self, testName):
        ModuleIDs = []
        for module in self.modules:
            ModuleIDs.append(str(module.getModuleName()))
        # output_dir gets set to $DATA_dir/Test_{testname}/Test_Module{ModuleID}_{Test}_{TimeStamp}
        return ConfigureTest(
            testName,
            "_Module".join(ModuleIDs),
            self.output_dir,
            self.input_dir,
        )

    def configTest(self, **kwargs):
        # Gets the run number by reading from the RunNumber.txt file.
        try:
            # NOTE: All processes are going to have the same run number as written, there is no race condition though.
            RunNumberFileName = (
                os.environ.get("PH2ACF_BASE_DIR") + "/test/RunNumber.txt"
            )
            if os.path.isfile(RunNumberFileName):
                runNumberFile = open(RunNumberFileName, "r")
                runNumberText = runNumberFile.readlines()
                self.RunNumber = runNumberText[0].split("\n")[0]
                logger.info("RunNumber: {}".format(self.RunNumber))
        except OSError:
            logger.warning("Failed to retrieve RunNumber due to OSError")

        # If currentTest is not set check if it's a compositeTest and if so set testname accordingly, otherwise set it based off the test set in info[1]
        if self.currentTest == "" and isCompositeTest(self.info):
            testName = self.test_list[0]
        elif self.currentTest is None:
            testName = self.info
        else:
            testName = self.currentTest
        self.output_dir, self.input_dir = self.config_output_dir(testName)

        # The default place to get the config file is in /settings/RD53Files/CMSIT_RD53.txt
        # FIXME Fix rd53_file[key] so that it reads the correct txt file depending on what module is connected. -> Done!

        for key in self.rd53_file.keys():
            if self.rd53_file[key] is None:
                self.rd53_file[key] = os.environ.get(
                    "PH2ACF_BASE_DIR"
                ) + "/settings/RD53Files/CMSIT_{0}{1}.txt".format(
                    self.boardType, self.moduleVersion
                )
                print("Getting config file {0}".format(self.rd53_file[key]))

        # At first there should be no input_dir and we should be grabbing the default txt files.
        # After the first test, we should see values or input_dir and output_dir signifiying that the txt files are being updated.
        logger.info(f"{self.input_dir=}")
        logger.info(f"{self.output_dir=}")

        # NOTE:  This code is to update the mapping of Ph2_ACF txt files
        if self.input_dir == "":
            # Copies file given in rd53[key] to test directory in Ph2_ACF test area as CMSIT_RD53.txt and the output dir.
            SetupRD53ConfigfromFile(self.rd53_file, self.output_dir)
        else:
            logger.info(f"{self.testIndexTracker=}")
            print(os.listdir(self.input_dir))
            SetupRD53Config(self.input_dir, self.output_dir, self.rd53_file)

        logger.info(f"{self.config_file=}")

        # NOTE: This code block is used to generate the XML configuration files
        if self.input_dir == "":
            # If no config file(xml file) is given create the XML file and place it into a .tmp directory
            # Create the directory to store the xml file
            if self.config_file == "":
                tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
                if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
                    try:
                        os.mkdir(tmpDir)
                        logger.info("Creating " + tmpDir)
                    except OSError:
                        logger.warning("Failed to create " + tmpDir)
                # Create the xml file from the text file
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
                    # QMessageBox.information(None,"Noitce", "Using default XML configuration",QMessageBox.Ok)
            else:
                for firmware in self.firmware:
                    SetupXMLConfigfromFile(
                        self.config_file, self.output_dir, firmware.getBoardName()
                    )
        else:
            if self.config_file != "":
                for firmware in self.firmware:
                    SetupXMLConfigfromFile(
                        self.config_file, self.output_dir, firmware.getBoardName()
                    )
            else:
                tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
                if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
                    try:
                        os.mkdir(tmpDir)
                        logger.info("Creating " + tmpDir)
                    except OSError:
                        logger.warning("Failed to create " + tmpDir)
                # Create the xml file from the text file
                for firmware in self.firmware:
                    config_file = GenerateXMLConfig(
                        firmware, self.currentTest, tmpDir, self.txt_files
                    )

                    if config_file:
                        SetupXMLConfigfromFile(
                            config_file, self.output_dir, firmware.getBoardName()
                        )
                    else:
                        logger.warning("No Valid XML configuration file")

        self.initializeRD53Dict()
        self.config_file = ""
        return

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
            return

    # This loops over all the tests by using the on_finish pyqt decorator defined below
    def runCompositeTest(self, testName):
        logger.info("Inside runCompositeTest")
        if self.halt:
            return
        runTestList = self.test_list

        if self.testIndexTracker == len(self.test_list):
            logger.info("Reset testIndexTracker")
            # self.testIndexTracker = 0
            # self.testsAttempted = 0
            return
        testName = runTestList[self.testIndexTracker]
        if self.testIndexTracker + 1 < len(
            runTestList
        ):  # Check if there is a next test
            nextTest = runTestList[self.testIndexTracker + 1]
        else:
            nextTest = None
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
                "You do not have instruments required to run a Trimbit scan connected.\nYou must have an Adc Board."
            )

    # def run_VDDsweep(self,total_steps:int = 16, chip = 12, fc7_index : int = 0) -> None:
    #     VDDsweep_process = QProcess()
    #     self.outputString.emit("Running VDD sweep test", self.runwindow.ConsoleViews[fc7_index])

    #     VDDsweep_process.setProcessChannelMode(QtCore.QProcess.MergedChannels)
    #     VDDsweep_process.setWorkingDirectory(
    #         os.environ.get("PH2ACF_BASE_DIR") + "/test/"
    #         )

    #     VDDsweep_process.readyReadStandardOutput.connect(
    #             lambda: self.on_readyReadStandardOutput_VDDsweep(VDDsweep_process, fc7_index)
    #         )

    #     VDDsweep_process.start(
    #         "CMSITminiDAQ",
    #         ["-f", f"CMSIT_{self.firmware[fc7_index].getBoardName()}.xml"],
    #         )

    #     if VDDsweep_process.state() != QProcess.NotRunning:
    #         result = VDDsweep_process.waitForFinished(-1) #waits indefinitely
    #         if not result:
    #             logger.error(f"Ph2_ACF physics test on {self.firmware[fc7_index].getBoardName()} didn't excute correctly.")
    #             VDDsweep_process.kill()
    #     add_trim = self.measureADC(chip)
    #     self.ProgressValue += 1
    #     for i in range(len(self.firmware)):
    #         self.runwindow.ResultWidget.ProgressBars[i][self.testIndexTracker].setValue(100 * self.ProgressValue / total_steps)
    #     return add_trim

    def GADC_execute_each_step(
        self,
        upOrDown: str,
        total_steps: int,
        physics_seconds: int = site_settings.SLDOScan_GADC["physics seconds"],
        fc7_index: int = 0,
    ) -> None:
        GADC_processes = [
            QProcess() for _ in self.instruments._module_dict
        ]  # loops through channels
        for i, process in enumerate(GADC_processes):
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
            process.setWorkingDirectory(os.environ.get("PH2ACF_BASE_DIR") + "/test/")
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

        for process, firmware in zip(GADC_processes, self.firmware):
            if process.state() != QProcess.NotRunning:
                result = process.waitForFinished(-1)  # waits indefinitely
                if not result:
                    logger.error(
                        f"Ph2_ACF physics test on {firmware.getBoardName()} didn't excute correctly."
                    )
                    process.kill()

        self.ProgressValue += 1
        for i in range(len(self.firmware)):
            self.runwindow.ResultWidget.ProgressBars[i][self.testIndexTracker].setValue(
                100 * self.ProgressValue / total_steps
            )

    def runSingleTest(self, testName, nextTest=None):
        logger.info(f"Text files used for xml generation: {self.txt_files}")
        if "analyze" in testName.lower():
            self.output_dir, self.input_dir = self.config_output_dir(testName)
            self.currentTest = testName

            EnableReRun = self.onFinalTest(self.testIndexTracker + 1)
            self.stepFinished.emit(EnableReRun)

            if self.master.expertMode:
                self.updateResult.emit(self.output_dir)
            else:
                step = "{}:{}".format(self.testIndexTracker, self.currentTest)
                self.updateResult.emit((step, self.figurelist))

            for i in range(len(self.firmware)):
                self.runwindow.ResultWidget.ProgressBars[i][
                    self.testIndexTracker
                ].setValue(100)
            return

        print("Executing Single Step test...")
        for console in self.runwindow.ConsoleViews:
            self.outputString.emit("Executing Single Step test...", console)

        self.starttime = None
        self.ProgressingMode = "None"
        self.currentTest = testName

        self.updateOptimizedXMLValues()
        self.configTest()

        # Make sure that the GUI is not trying to write to the root directory
        try:
            assert self.output_dir != ""
        except AssertionError:
            logger.exception(
                "Output directory was not formatted correctly, closing GUI to not write to root directory."
            )
            raise

        # if os.path.exists(self.outputFile):
        #     self.outputfile = open(self.outputFile, "a")
        # else:
        #     self.outputfile = open(self.outputFile, "w")

        if testName == "SLDOScan_GADC":
            starting_voltages = [
                np.abs(getattr(module["hv"], "voltage"))
                for module in self.instruments._module_dict.values()
            ]
            self.instruments.hv_off(
                execute_each_step=lambda: self.ramp_progress_bar(starting_voltages)
            )

            if "1x2" in self.ModuleType.lower():
                SLDOScan_GADC_dict = site_settings.SLDOScan_GADC["1x2"]
            elif "quad" in self.ModuleType.lower():
                SLDOScan_GADC_dict = site_settings.SLDOScan_GADC["quad"]
            else:
                SLDOScan_GADC_dict = site_settings.SLDOScan_GADC["1x2"]
                logger.error(
                    'Module type does not contain "1x2" or "quad". Running SLDOScan_GADC as 1x2.'
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
                self.runwindow.ResultWidget.ProgressBars[i][
                    self.testIndexTracker
                ].setValue(0)

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
                execute_each_step=lambda: self.GADC_execute_each_step(
                    "up", total_steps
                ),
            )

            down_sweep = self.instruments.lv_sweep(
                target=SLDOScan_GADC_dict["starting current"],
                delay=0.1,
                set_property="current",
                measure=True,
                step_size=SLDOScan_GADC_dict["step size"],
                execute_each_step=lambda: self.GADC_execute_each_step(
                    "down", total_steps
                ),
            )

            self.instruments.lv_off()

            for i in range(len(self.firmware)):
                self.runwindow.ResultWidget.ProgressBars[i][
                    self.testIndexTracker
                ].setValue(100)

            for datatype in ("VDDD", "VDDA"):
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
                                for i in getattr(self, f"{datatype}up")[channel][
                                    chip
                                ].values()
                            ],
                            [sweep_step[-1] for sweep_step in down_sweep[0][1]],
                            [
                                float(i)
                                for i in getattr(self, f"VIN{datatype[-1]}down")[
                                    channel
                                ][chip].values()
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
                        self.makeSLDOPlot(data, f"{datatype}_ROC{int(chip)}")
            self.SLDOScanFinished()
            return

        if self.instruments:
            lv_on = False
            for number in self.instruments.get_modules().keys():
                print(self.instruments.status()[number]["lv"])
                if self.instruments.status()[number]["lv"]:
                    lv_on = True
                    break
            if not lv_on:
                self.instruments.lv_on(
                    voltage=site_settings.ModuleVoltageMapSLDO[
                        self.master.module_in_use
                    ],
                    current=site_settings.ModuleCurrentMap[self.master.module_in_use],
                )

        if "IVCurve" in testName:
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
            return

        if testName == "SLDOScan":
            self.currentTest = testName
            self.configTest()
            self.SLDOScanData = []
            self.SLDOProgressValue = 0
            self.SLDOScanHandler = SLDOCurveHandler(
                self.instruments,
                moduleType=self.ModuleType[5:],
                end_current=site_settings.ModuleCurrentMap[self.master.module_in_use],
                voltage_limit=site_settings.ModuleVoltageMapSLDO[
                    self.master.module_in_use
                ],
                execute_each_step=self.ramp_progress_bar,
            )
            self.SLDOScanHandler.makeplotSignal.connect(self.makeSLDOPlot)
            self.SLDOScanHandler.finishedSignal.connect(self.SLDOScanFinished)
            self.SLDOScanHandler.progressSignal.connect(self.updateProgress)
            self.SLDOScanHandler.abortSignal.connect(self.urgentStop)
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("Beginning SLDOScan", console)
            self.SLDOScanHandler.SLDOScan()
            return

        # If the HV is not already on, turn it on.
        if self.instruments and self.currentTest != "SLDOScan_GADC":
            default_hv_voltage = site_settings.icicle_instrument_setup[
                "instrument_dict"
            ]["hv"]["default_voltage"]
            # assumes only 1 HV titled 'hv' in instruments.json
            hv_on_module = False
            mod_dict = self.instruments.get_modules()
            for number in self.instruments.get_modules().keys():
                if self.instruments.status()[number]["hv"] == "1":
                    hv_on_module = True
                    break

                if testName == "SCurveScan_2100_FWD":
                    if hv_on_module:
                        starting_voltages = [
                            np.abs(getattr(module["hv"], "voltage"))
                            for module in self.instruments._module_dict.values()
                        ]
                        self.instruments.hv_off(
                            execute_each_step=lambda: self.ramp_progress_bar(
                                starting_voltages
                            )
                        )
                        self.instruments.hv_on_module(
                            module=mod_dict[number],
                            voltage=site_settings.forward_bias_voltage,
                            delay=0.3,
                            step_size=10,
                            execute_each_step=lambda: self.ramp_progress_bar(
                                [site_settings.forward_bias_voltage]
                                * len(self.instruments._module_dict.values())
                            ),
                        )
                    else:
                        self.instruments.hv_on_module(
                            module=mod_dict[number],
                            voltage=site_settings.forward_bias_voltage,
                            delay=0.3,
                            step_size=10,
                            execute_each_step=lambda: self.ramp_progress_bar(
                                [site_settings.forward_bias_voltage]
                                * len(self.instruments._module_dict.values())
                            ),
                        )
                    testName = "SCurveScan_2100"
                    hv_on_module = True
                if not hv_on_module:
                    self.instruments.hv_on_module(
                        module=mod_dict[number],
                        voltage=default_hv_voltage,
                        delay=0.3,
                        step_size=10,
                        execute_each_step=lambda: self.ramp_progress_bar(
                            [default_hv_voltage]
                            * len(self.instruments._module_dict.values())
                        ),
                    )

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
            self.setupQProcess()

    def storeTrimbitResults(self, adcmeasurements, pin_mapping):
        self.ADCmeasurements = adcmeasurements
        self.pin_mapping = pin_mapping

    def setupQProcess(self):
        self.tempHistory = [0.0] * self.numChips
        self.tempindex = 0

        # NOTE: This may cause issues as I believe both instances of Ph2_ACF will write to the same place.
        logger.info(f"{self.output_dir=}")
        self.outputFile = self.output_dir + "/output.txt"
        self.errorFile = self.output_dir + "/error.txt"
        # if os.path.exists(self.outputFile):
        #     self.outputfile = open(self.outputFile, "a")
        # else:
        #     self.outputfile = open(self.outputFile, "w")

        for process in self.info_processes:
            process.setProcessChannelMode(QtCore.QProcess.MergedChannels)
            process.setWorkingDirectory(os.environ.get("PH2ACF_BASE_DIR") + "/test/")

        """
        if self.currentTest == ["exampletest"]:               #for tests needing -c
            for process, firmware in zip(self.info_processes, self.firmware):
                process.start(
                    "echo",
                    [
                        "Running COMMAND: CMSITminiDAQ  -f  CMSIT_{0}.xml  -c  {1}".format(
                            firmware.getBoardName(),
                            Test_to_Ph2ACF_Map[self.currentTest],
                        )
                    ],
                )        
                    """
        if self.currentTest == "CommunicationTest":
            for process, firmware in zip(self.info_processes, self.firmware):
                process.start(
                    "echo",
                    [
                        f"Running COMMAND: CMSITminiDAQ  -f  CMSIT_{firmware.getBoardName()}.xml  -p"
                    ],
                )
        else:
            for process, firmware in zip(self.info_processes, self.firmware):
                process.start(
                    "echo",
                    [
                        "Running COMMAND: CMSITminiDAQ  -f  CMSIT_{0}.xml  -k -c  {1}".format(
                            firmware.getBoardName(),
                            Test_to_Ph2ACF_Map[self.currentTest],
                        )
                    ],
                )

        for process in self.info_processes:
            process.waitForFinished()

        for process in self.run_processes:
            process.setProcessChannelMode(QtCore.QProcess.MergedChannels)
            process.setWorkingDirectory(os.environ.get("PH2ACF_BASE_DIR") + "/test/")

        for process in self.fw_processes:
            process.setProcessChannelMode(QtCore.QProcess.MergedChannels)
            process.setWorkingDirectory(os.environ.get("PH2ACF_BASE_DIR") + "/test/")

        if self.currentTest == "CommunicationTest":
            for process, firmware in zip(self.run_processes, self.firmware):
                process.start(
                    "CMSITminiDAQ",
                    ["-f", f"CMSIT_{firmware.getBoardName()}.xml", "-p"],
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
                    "CMSITminiDAQ",
                    [
                        "-f",
                        f"CMSIT_{firmware.getBoardName()}.xml",
                    ],
                )
        else:
            i = 0
            for process, firmware in zip(self.run_processes, self.firmware):
                process.start(
                    "CMSITminiDAQ",
                    [
                        "-f",
                        f"CMSIT_{firmware.getBoardName()}.xml",
                        # "-k",
                        "-c",
                        "{}".format(Test_to_Ph2ACF_Map[self.currentTest]),
                    ],
                )
                # Check if the process is running
                if process.state() == QProcess.NotRunning:
                    logger.error(
                        f"Process for firmware {self.firmware[i].getBoardName()} failed to start."
                    )
                else:
                    logger.info(
                        f"Process for firmware {self.firmware[i].getBoardName()} started successfully."
                    )
                i += 1

    def abortTest(self):
        self.halt = True
        for process in self.run_processes:
            process.kill()

        self.haltSignal.emit(self.halt)

        self.starttime = None
        self.testsAttempted = 0  # reset when aborted for fresh restart
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

            print("IREF MATCH STATUS BEFORE VALIDATION:", self.iref_match_status)
            print("MODULE NAMES:", [module.getModuleName() for module in self.modules])

            for i, beboard in enumerate(self.firmware):
                boardID = beboard.getBoardID()
                for OG in beboard.getAllOpticalGroups().values():
                    ogID = OG.getOpticalGroupID()
                    for module in OG.getAllModules().values():
                        print(f"curr test {self.currentTest}")
                        hybridID = module.getFMCPort()
                        module_data = {
                            "boardID": boardID,
                            "ogID": ogID,
                            "hybridID": hybridID,
                            "module": module,
                        }
                        result, self.BBanalysis_root_files = ResultGrader(
                            self.felis_instances[i],
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
                            self.iref_match_status,  # Pass iref_match_status for IREF validation
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
            self.updateFinishedTests.emit(
                self.finished_tests
            )  # Obsolete at time of commit: "return all(passed)"
        except Exception as err:
            logger.error(err)

    def collect_plots(self, moduleName):
        try:
            plot_paths = []
            scratch = os.path.join(self.felis.path_scratch, moduleName)
            test = "{0:02d}_{1}".format(self.testIndexTracker, self.currentTest)
            directory = os.path.join(scratch, test)

            for filename in os.listdir(directory):
                if filename.lower().endswith(".svg") or filename.lower().endswith(
                    ".png"
                ):
                    plot_paths.append(os.path.join(directory, filename))

            return plot_paths
        except Exception as e:
            if "IVCurve" in self.currentTest or "SLDOScan" in self.currentTest:
                if moduleName in self.figurelist.keys():
                    return self.figurelist[moduleName]
                else:
                    return []
            else:
                print("testHandler.collect_plots Exception:", repr(e))
                return []

    # For root files with the same RunNumber in the PH2ACF directory, this function only copies over to
    # self.output_dir the .root file modified most recently. This will copy over the wrong file if somebody
    # manually edits the .root file in the PH2ACF directory, so there may be a better way to do this
    def copyMostRecentRootFile(self, RunNumber, base_dir, output_dir, test):
        logger.debug("Inside copyMostRecentRootFile()")
        files = root_files[test] if test in root_files.keys() else (test,)
        for name in files:
            name = name.split("_")[0]
            if "SCurveScan" in name:
                name = "SCurve"
            elif "GainScan" in name:
                name = "Gain"
            elif "Threshold" in name:
                name = name.replace("Threshold", "Thr")

            # Construct the search pattern for files
            search_pattern = f"{base_dir}/Run{RunNumber}_{name}.root"
            logger.debug(f"Looking for {search_pattern}")
            print(f"Looking for {search_pattern}")
            print("Search pattern exists?:", os.path.exists(search_pattern))

            # Find all matching files
            matching_files = glob.glob(search_pattern)

            if len(matching_files) == 0:
                raise Exception(
                    f"Failed to copy root file to output directory. \
Module disconnection detected because Ph2_ACF didn't \
create {search_pattern}"
                )

            # Sort files by modification time (newest first)
            latest_file = max(matching_files, key=os.path.getmtime)

            if os.path.getsize(latest_file) == 0:
                raise Exception(
                    f"Failed to copy root file to output directory. \
Module disconnection detected because {latest_file} \
created by Ph2_ACF is empty."
                )

            # Copy the most recent file to the output directory
            logger.debug("About to copy inside copyMostRecentROOTFile")

            # When using multiple FC7s root files will get overwritten so need to attach
            # what fc7 the test was run on to file name
            fc7_in_use: str = base_dir.split("/")[-1].replace(".", "_")
            file_name: str = latest_file.split("/")[-1]
            os.system(f"cp {latest_file} {output_dir}/{fc7_in_use}_{file_name}")

    def saveTest(self, processIndex: int, process: QProcess):
        logger.debug("Inside saveTest")
        if process.state() == QProcess.Running:
            QMessageBox.critical(self, "Error", "Process not finished", QMessageBox.Ok)
            return

        try:
            if self.RunNumber == "-1":
                os.system(
                    "cp {0}/test/Results/Run000000*.root {1}/".format(
                        os.environ.get("PH2ACF_BASE_DIR"), self.output_dir
                    )
                )

            elif "IVCurve" in self.currentTest or "IREF_GADC" in self.currentTest:
                print("copying MonitorDQM.root file to output directory")
                os.system(
                    "cp {0}/test/Results/Run{1}_MonitorDQM.root {2}/".format(
                        os.environ.get("PH2ACF_BASE_DIR"),
                        self.RunNumber,
                        self.output_dir,
                    )
                )
            else:
                for fc7 in self.firmware:
                    self.copyMostRecentRootFile(
                        self.RunNumber,
                        os.environ.get("PH2ACF_BASE_DIR")
                        + f"/test/{fc7.getBoardName()}",
                        self.output_dir,
                        self.currentTest,
                    )

        except Exception as e:
            logger.error(e)
            traceback.print_exc()
            if self.currentTest != "CommunicationTest":
                self.forceContinue(self.firmware[processIndex])

    #######################################################################
    ##  For real-time terminal display
    #######################################################################

    @QtCore.pyqtSlot()
    def on_readyReadStandardOutput(self, processIndex: int):
        if self.readingOutput:
            print("Thread competition detected")
            return
        self.readingOutput = True

        alltext = (
            self.run_processes[processIndex].readAllStandardOutput().data().decode()
        )

        mode = "a" if os.path.exists(self.outputFile) else "w"
        with open(self.outputFile, mode) as outputfile:
            outputfile.write(alltext)
        textline = alltext.split("\n")

        for textStr in textline:
            try:
                if "Configuring chips of hybrid" in textStr:
                    ansi_escape = re.compile(r"\x1b\[.*?m")
                    clean_text = ansi_escape.sub("", textStr)
                    hybrid_id = clean_text.split("hybrid: ")[-1].strip()
                    self.mod_dict[hybrid_id] = {}
                    self.fused_dict_index[0] = hybrid_id

                if "Configuring RD53" in textStr:
                    ansi_escape = re.compile(r"\x1b\[.*?m")
                    clean_text = ansi_escape.sub("", textStr)
                    chip_number = clean_text.split("RD53: ")[-1].strip()
                    self.fused_dict_index[1] = chip_number
                    # print(f"Clean_text: {clean_text}")
                    print(f"Chip Number: {chip_number}")

                if "Wire bonded Iref" in textStr:
                    ansi_escape = re.compile(r"\x1b\[.*?m")
                    clean_text = ansi_escape.sub("", textStr)
                    iref_value = clean_text.split("Iref = ")[-1].strip()
                    self.mod_dict[self.fused_dict_index[0]][
                        self.fused_dict_index[1]
                    ] = iref_value
                    print(f"IREF Value: {iref_value}")
                    chip_id = self.fused_dict_index[1]
                    module_name = self.modules[0].getModuleName()
                    db_iref = chip_iref_db.get(str(chip_id))
                    # Initialize module status to True if not set
                    if module_name not in self.iref_match_status:
                        self.iref_match_status[module_name] = True
                    # Compare with database value
                    if db_iref is not None:
                        if db_iref != iref_value:
                            print(
                                f"Mismatch: IREF for chip {chip_id} (database: {db_iref}, module: {iref_value})"
                            )
                            self.iref_match_status[module_name] = (
                                False  # Mark module as failed
                            )
                    else:
                        print(f"No database IREF found for chip {chip_id}")
                        self.iref_match_status[module_name] = (
                            False  # Mark as failed if no DB entry
                        )

                if "Fused ID" in textStr:
                    ansi_escape = re.compile(r"\x1b\[.*?m")
                    clean_text = ansi_escape.sub("", textStr)
                    fuse_id = clean_text.split("Fused ID: ")[-1].strip()
                    self.mod_dict[self.fused_dict_index[0]][
                        self.fused_dict_index[1]
                    ] = fuse_id

                if self.starttime is not None:
                    self.currentTime = time.time()
                    runningTime = self.currentTime - self.starttime
                    self.runwindow.ResultWidget.runtimes[processIndex][
                        self.testIndexTracker
                    ].setText("{0} s".format(round(runningTime, 1)))
                else:
                    self.starttime = time.time()
                    self.currentTime = self.starttime

            except Exception as err:
                logger.info("Error occures while parsing running time, {0}".format(err))
            if "@@@ End of CMSIT miniDAQ @@@" in textStr:
                self.ProgressingMode = "Summary"
            if self.ProgressingMode == "Perform":
                if "Progress:" in textStr:
                    try:
                        index = textStr.split().index("Progress:") + 2
                        # self.ProgressValue = float(textStr.split()[index].rstrip("%"))
                        self.ProgressValue = float(
                            re.sub(r"\x1b\[\d+m", "", textStr.split()[index].strip("%"))
                        )
                        if self.ProgressValue == 100:
                            self.ProgressingMode = "Summary"
                        self.runwindow.ResultWidget.ProgressBars[processIndex][
                            self.testIndexTracker
                        ].setValue(self.ProgressValue)
                        ##Added because of Ph2_ACF bug:

                    except Exception as e:
                        print(f"Error while updating progress bar {e}")
                        pass

                if self.check_for_end_of_test(textStr):
                    self.runwindow.ResultWidget.ProgressBars[processIndex][
                        self.testIndexTracker
                    ].setValue(100)
                elif "TEMPSENS_" in textStr:
                    try:
                        output = textStr.split("[")
                        sensor = output[8]
                        sensorMeasure = sensor[3:]

                        if (
                            sensorMeasure != ""
                            or sensorMeasure != "44.086 +/- 1.763 °C"
                        ):
                            temp = float(sensorMeasure.split("+")[0].strip())
                            self.tempHistory[self.tempindex] = temp
                            if any(
                                num > site_settings.Warning_Threshold
                                for num in self.tempHistory
                            ):
                                self.runwindow.updateTempIndicator("orange")
                            elif any(
                                num > site_settings.Emergency_Threshold
                                for num in self.tempHistory
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

                    except Exception as e:
                        print("Failed due to {0}".format(e))
                elif "INTERNAL_NTC" in textStr:
                    try:
                        ansi_pattern = re.compile(r"\x1B[@-_][0-?]*[ -/]*[@-~]")
                        clean_text = ansi_pattern.sub("", textStr)
                        if "INTERNAL_NTC" in clean_text:
                            sensor = (
                                clean_text.split("INTERNAL_NTC:")[1]
                                .strip()
                                .split("C")[0]
                                .strip()
                            )
                            sensorMeasure0 = re.sub(r"[^\d\.\+\- ]", "", sensor)
                            sensorMeasure0 += " °C"
                            sensorMeasure = sensorMeasure0.replace("+-", "+/-")

                            if (
                                sensorMeasure != ""
                                or sensorMeasure != "44.086 +/- 1.763 °C"
                            ):
                                temp = float(sensorMeasure.split("+")[0].strip())
                                self.tempHistory[self.tempindex] = temp
                                if any(
                                    num > site_settings.Warning_Threshold
                                    for num in self.tempHistory
                                ):
                                    self.runwindow.updateTempIndicator("orange")
                                elif any(
                                    num > site_settings.Emergency_Threshold
                                    for num in self.tempHistory
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

                    except Exception as e:
                        print("Failed due to {0}".format(e))
                text = textStr.encode("ascii")
                _, text = parseANSI(text)
                self.outputString.emit(
                    text.decode("utf-8"), self.runwindow.ConsoleViews[processIndex]
                )
                continue
            # This next block needs to be edited once Ph2ACF bug is fixed.  Remove the Fixme when ready.

            elif self.ProgressingMode == "Summary":
                if self.check_for_end_of_test(textStr):
                    self.runwindow.ResultWidget.ProgressBars[processIndex][
                        self.testIndexTracker
                    ].setValue(100)
            elif "@@@ Initializing the Hardware @@@" in textStr:
                self.ProgressingMode = "Configure"
            elif "@@@ Performing" in textStr:
                self.ProgressingMode = "Perform"
                self.outputString.emit(
                    '<b><span style="color:#ff0000;"> Performing the {} test </span></b>'.format(
                        self.currentTest
                    ),
                    self.runwindow.ConsoleViews[processIndex],
                )

            text = textStr.encode("ascii")
            _, text = parseANSI(text)
            self.outputString.emit(
                text.decode("utf-8"), self.runwindow.ConsoleViews[processIndex]
            )

        match = re.search(r"CMSIT_RD53_([^_]+)", alltext)
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

        self.readingOutput = False

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
                    logger.info("HybridID {0}".format(hybridID))
                    logger.info("chipIDs {0}".format(chipIDs))
                    isCROC = "CROC" in module.getModuleType()
                    for chipID in chipIDs:
                        updatedXMLValues[f"{hybridID}/{chipID}"] = {}
                        for updatedFEKey in updatedFEKeys:
                            if isCROC:
                                if updatedFEKey in [
                                    "LATENCY_CONFIG",
                                    "Vthreshold_LIN",
                                ]:  # registers not on CROC modules
                                    continue
                            elif not isCROC:
                                if updatedFEKey in [
                                    "TriggerConfig",
                                    "DAC_GDAC_",
                                    "CAL_EDGE_FINE_DELAY",
                                ]:  # registers only on CROC modules
                                    continue
                            updatedXMLValues[f"{hybridID}/{chipID}"][updatedFEKey] = ""
        except Exception as err:
            logger.error(f"Failed to update, {err}")

    def check_for_end_of_test(self, textStr):
        # function to support the quick fix in on_readyReadStandardOutput() where
        # the progress bar doesn't always reach 100%.
        currentTest = Test_to_Ph2ACF_Map[self.currentTest]
        if currentTest in ["thradj", "thrmin"] and "Global threshold for" in textStr:
            return True
        elif currentTest in ["gainopt"] and "Krummenacher Current" in textStr:
            return True
        elif currentTest in ["injdelay"]:
            if "New latency dac" in textStr:
                return True
            elif "New injection delay" in textStr:
                return True
        elif "CommunicationTest" == self.currentTest:
            return True
        elif "IREF_GADC" == self.currentTest and self.ProgressingMode == "Summary":
            return True
        return False

    # Reads data that is normally printed to the terminal and saves it to the output file
    @QtCore.pyqtSlot()
    def on_readyReadStandardOutput_info(self, processIndex: int):
        alltext = (
            self.info_processes[processIndex].readAllStandardOutput().data().decode()
        )

        mode = "a" if os.path.exists(self.outputFile) else "w"
        with open(self.outputFile, mode) as outputfile:
            outputfile.write(alltext)
        textline = alltext.split("\n")

        for textStr in textline:
            self.outputString.emit(textStr, self.runwindow.ConsoleViews[processIndex])

    @QtCore.pyqtSlot()
    def on_readyReadStandardOutput_VDDsweep(self, process, fc7_index):
        """
        Slot to handle VDDsweep process output, parse ANSI, and emit to console.
        Safe to use in TestHandler as long as updateConsoleInfo does not emit outputString.
        """
        if getattr(self, "readingOutput", False):
            print("Thread competition detected")
            return
        self.readingOutput = True

        alltext = process.readAllStandardOutput().data().decode()
        if hasattr(self, "outputfile") and self.outputfile:
            self.outputfile.write(alltext)
        textline = alltext.split("\n")
        for textStr in textline:
            try:
                text = textStr.encode("ascii")
                _, text = parseANSI(text)
                self.outputString.emit(
                    text.decode("utf-8"), self.runwindow.ConsoleViews[fc7_index]
                )
            except Exception as e:
                print(f"Error emitting console output: {e}")
        self.readingOutput = False

    # @QtCore.pyqtSlot()
    # def on_readyReadStandardOutput_VDDsweep(self, process:QProcess, fc7_index:int):
    #     if self.readingOutput:
    #         print("Thread competition detected")
    #         return
    #     self.readingOutput = True

    #     alltext = (
    #         process.readAllStandardOutput().data().decode()
    #     )
    #     self.outputfile.write(alltext)
    #     textline = alltext.split("\n")

    #     for textStr in textline:
    #         text = textStr.encode("ascii")
    #         _, text = parseANSI(text)
    #         self.outputString.emit(
    #             text.decode("utf-8"), self.runwindow.ConsoleViews[fc7_index]
    #         )
    #         self.runwindow.ConsoleViews[fc7_index].repaint()

    #     self.readingOutput = False

    @QtCore.pyqtSlot()
    def on_readyReadStandardOutput_GADC(
        self, process: QProcess, fc7_index: int, upOrDown: str, current, channel
    ):
        if self.readingOutput:
            print("Thread competition detected")
            return
        self.readingOutput = True

        alltext = process.readAllStandardOutput().data().decode()

        with open(self.outputFile, mode) as outputfile:
            outputfile.write(alltext)
        textline = alltext.split("\n")

        for textStr in textline:
            text = textStr.encode("ascii")
            _, text = parseANSI(text)
            self.outputString.emit(
                text.decode("utf-8"), self.runwindow.ConsoleViews[fc7_index]
            )
            self.runwindow.ConsoleViews[fc7_index].repaint()
            # .repaint() should not be necessary - indicates a larger problem in the PyQt workflow.

            textStr = re.compile(r"\x1B[@-_][0-?]*[ -/]*[@-~]").sub("", textStr)
            match = re.search(
                r"data for \[board/opticalGroup/hybrid/chip = (\d+)/(\d+)/(\d+)/(\d+)\]",
                textStr,
            )
            if match:
                self.GADC_meas_chip = match.group(4)
            else:
                match = re.search(r"(\w+):\s*([\d.]+)\s*\+/-\s*([\d.]+)\s*V", textStr)
                if match:
                    if self.GADC_meas_chip is not None:
                        if match.group(1) in ("VDDD", "VDDA", "VINA", "VIND"):
                            multiplier = site_settings.SLDOScan_GADC["multipliers"][
                                match.group(1)[:3]
                            ]

                            if (
                                self.GADC_meas_chip
                                not in getattr(self, match.group(1) + upOrDown)[channel]
                            ):
                                getattr(self, match.group(1) + upOrDown)[channel][
                                    self.GADC_meas_chip
                                ] = {}
                            getattr(self, match.group(1) + upOrDown)[channel][
                                self.GADC_meas_chip
                            ][current] = (
                                float(match.group(2)) * multiplier
                            )  # This line enforces that it only logs one VDDD or VDDA value per sweep step

                            if (
                                self.GADC_meas_chip
                                not in getattr(
                                    self, match.group(1) + upOrDown + "Error"
                                )[channel]
                            ):
                                getattr(self, match.group(1) + upOrDown + "Error")[
                                    channel
                                ][self.GADC_meas_chip] = {}
                            getattr(self, match.group(1) + upOrDown + "Error")[channel][
                                self.GADC_meas_chip
                            ][current] = (
                                float(match.group(3)) * multiplier
                            )  # This line enforces that it only logs one VDDD or VDDA value per sweep step

                    else:
                        print(
                            f'Error: Did not receive expected message, "Reading monitored data for \
                        [board/opticalGroup/hybrid/chip = ...]", before measurement message "{match.group(0)}"'
                        )
                        logger.error(
                            f'Did not receive expected message, "Reading monitored data for \
                        [board/opticalGroup/hybrid/chip = ...]", before measurement message "{match.group(0)}"'
                        )

        self.readingOutput = False

    @QtCore.pyqtSlot()
    def on_finish(self, processIndex: int):
        # While the process is killed:
        # Wait for all processes to finish so FC7s don't get out of sync
        # May be a source of stalling with the -1 which waits indefinitely

        logger.debug("All processes finished")

        if self.halt:
            self.haltSignal.emit(True)
            return

        if self.run_processes[processIndex].state() == QProcess.Running:
            logger.info(
                "process is still running...  Attempting to terminate before next test."
            )
            self.run_processes[processIndex].terminate()
            if not self.run_processes[processIndex].waitForFinished(3000):
                logger.warning("Process would not terminate, so killing it now...")
                self.run_processes[processIndex].kill()

        if "IVCurve" in self.currentTest:
            self.saveTest(processIndex, self.run_processes[processIndex])
            return

        # Save the output ROOT file to output_dir
        logger.debug("About to run saveTest()")
        time.sleep(1)
        self.saveTest(processIndex, self.run_processes[processIndex])

        # validate the results
        logger.debug("About to run validateTest()")
        self.validateTest()

        logger.debug("testIndexTracker before increment: %i", self.testIndexTracker)
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
        logger.info("Inside onFinalTest")
        for process in self.run_processes:
            if process.state() == QProcess.Running:
                return

        EnableReRun = False
        # Will send signal to turn off power supply after composite or single tests are run
        if isCompositeTest(self.info):
            if index == len(
                self.test_list
            ):  # Checks that this was the last test in the sequence.
                logger.info("index == len.self.test_list")
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
                                    self.collect_plots(module.getModuleName(), beboard.getName())
                                )
                if self.autoSave:
                    self.runwindow.upload_to_Panthera_starter()

        elif isSingleTest(self.info):
            EnableReRun = True
            self.powerSignal.emit()
            if self.autoSave:
                self.runwindow.upload_to_Panthera_starter()

        return EnableReRun

    def updateProgress(self, measurementType, stepSize):
        if measurementType == "IVCurve":
            self.IVProgressValue += stepSize / 2.0
            for i, firmware in enumerate(self.firmware):
                self.runwindow.ResultWidget.ProgressBars[i][
                    self.testIndexTracker
                ].setValue(self.IVProgressValue)
            self.ramp_progress_bar(
                [
                    site_settings.IVcurve_range[self.currentTest]
                    if site_settings.IVcurve_range[self.currentTest] < 0
                    else 80
                ]
                * len(self.instruments._module_dict.values())
            )
        elif "SLDO" in measurementType:
            self.SLDOProgressValue += stepSize
            for i, firmware in enumerate(self.firmware):
                self.runwindow.ResultWidget.ProgressBars[i][
                    self.testIndexTracker
                ].setValue(self.SLDOProgressValue)
        elif measurementType == "TrimbitScan":
            for i in range(len(self.firmware)):
                self.runwindow.ResultWidget.ProgressBars[i][
                    self.testIndexTracker
                ].setValue(stepSize)

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

    def makeTrimbitScanPlots(self, trimbit_dict, pin_mapping):
        """
        Plots measurement vs trimbit for each pin from a dictionary:
        trimbit_dict: {pin: [(trimbit, value), ...], ...}
        Returns a list of CSV filenames created.
        """
        csvfiles = []
        for module in self.modules:
            moduleName = module.getModuleName()
            for pin, name in pin_mapping.items():
                data = trimbit_dict.get(pin, [])
                if not data:
                    continue  # Skip pins with no data
                trimbits, values = zip(*data)
                svgfilename = "{0}/TrimbitCurve_Module_{1}_{2}.svg".format(
                    self.output_dir, moduleName, name
                )
                csvfilename = "{0}/TrimbitCurve_Module_{1}_{2}.csv".format(
                    self.output_dir, moduleName, name
                )
                np.savetxt(
                    csvfilename,
                    np.column_stack([trimbits, values]),
                    delimiter=",",
                    header="Trimbit,Measurement",
                    comments="",
                )
                csvfiles.append(csvfilename)
                plt.figure()
                plt.plot(trimbits, values, "-o", label=name)
                plt.xlabel("Trimbit")
                plt.ylabel("Measurement (V)")
                plt.title(f"Trimbit Scan for {name}")
                plt.grid(True)
                plt.legend()
                plt.savefig(svgfilename)
                plt.close()
                self.figurelist.setdefault(name, []).append(svgfilename)
        return csvfiles

    def IVCurveFinished(self, test: str, measure: dict):
        # Get the current timestamp
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        for process in self.run_processes:
            process.write(b"\n")
            process.waitForBytesWritten()
            process.waitForFinished()

        # 3/17/25 : Once HV distributor box arrives, functionality needs to be added for running
        # IVCurve on multiple modules. Once that happens, the loop under this comment can be edited
        # to output the results only to the console of the fc7 that each module is connnected to.
        for console in self.runwindow.ConsoleViews:
            self.outputString.emit(f"Voltages: {measure['voltage']}", console)
            self.outputString.emit(f"Currents: {measure['current']}", console)

        for module in self.modules:
            ogId = module.getOpticalGroup().getOpticalGroupID()
            beboardId = module.getOpticalGroup().getBeBoard().getBoardID()
            moduleName = module.getModuleName()
            hybridId = module.getFMCPort()

            self.IVCurveResult = ScanCanvas(
                self,
                xlabel="Voltage (V)",
                ylabel="I (A)",
                X=measure["voltage"],
                Y=measure["current"],
                invert=True,
            )

            csvfilename = "{0}/IVCurve_Module_{1}_{2}.csv".format(
                self.output_dir, moduleName, timestamp
            )

            # Some power supplies give outputs as a two dimensional array
            # This breaks np.savetxt. The second element of the array should be empty
            # either way, therefore, we will just flatten the array getting rid of the
            # second dimension. NOTE: If we do want measurements from multiple HV
            # channels this will need to reevaluated.

            # Convert to numpy array to give us access to flatten() and ndim
            voltages = np.array(measure["voltage"])
            current = np.array(measure["current"])

            # If the voltages are 2D+, then flatten.
            if voltages.ndim > 1:
                voltages = voltages.flatten()
                current = current.flatten()

            np.savetxt(csvfilename, (voltages, current), delimiter=",")
            module_canvas_path = "Detector/Board_{boardID}/OpticalGroup_{ogID}/Hybrid_{hybridID}/".format(
                boardID=beboardId, ogID=ogId, hybridID=hybridId
            )

            IVCurve_CSV_to_ROOT(
                moduleName, module_canvas_path, csvfilename, self.output_dir
            )

            filename = "{0}/IVCurve_Module_{1}_{2}.svg".format(
                self.output_dir, moduleName, timestamp
            )
            # filename2 = "IVCurve_Module_{0}_{1}.svg".format(moduleName, timestamp)
            self.IVCurveResult.saveToSVG(filename)
            # self.IVCurveResult.saveToSVG(filename2)

            self.figurelist[moduleName] = [filename]

        self.validateTest()

        step = "IVCurve"

        self.testIndexTracker += 1
        self.testsAttempted += 1

        EnableReRun = False

        # Will send signal to turn off power supply after composite or single tests are run
        if isCompositeTest(self.info):
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
            self.updateIVResult.emit(self.output_dir)
        else:
            self.updateIVResult.emit(
                (step, self.figurelist)
            )  ##Add else statement to add signal in simple mode

        if isCompositeTest(self.info):
            self.runTest()

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

    def TrimbitScanFinished(self):
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

            # Generate CSVs and get the list
            csvfiles = self.makeTrimbitScanPlots(self.ADCmeasurements, self.pin_mapping)
            Trimbit_CSV_to_ROOT(
                moduleName, module_canvas_path, csvfiles, self.output_dir
            )

        self.validateTest()
        self.testIndexTracker += 1
        self.testsAttempted += 1

        if isCompositeTest(self.info):
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
            self.updateResult.emit(self.output_dir)
        else:
            self.updateResult.emit(
                ("SLDOScan", self.figurelist)
            )  ##Add else statement to add signal in simple mode

        if isCompositeTest(self.info):
            self.runTest()

    def interactiveCheck(self, plot):
        pass

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

        def check_enabledModules():  # Not sure if this is maximally efficient
            for row in range(len(fc7modules)):
                if (
                    self.force_continue_window.table.item(row, 1).checkState()
                    == Qt.Checked
                ):
                    for i, module in enumerate(fc7modules):
                        if (
                            self.force_continue_window.table.item(i, 1).checkState()
                            == Qt.Checked
                        ):
                            module.setEnabled("1")
                        else:
                            module.setEnabled("0")

                    self.force_continue_window.abort = False
                    return True

            if not hasattr(self.force_continue_window, "label"):
                self.force_continue_window.label = QLabel(
                    "At least one module must be enabled."
                )
                self.force_continue_window.label.setStyleSheet("color: red;")
                self.force_continue_window.layout.insertWidget(
                    3, self.force_continue_window.label
                )
            return False

        # Define button handlers
        def handle_close(event):
            if self.force_continue_window.abort:
                for process in self.run_processes:
                    process.kill()
                self.halt = True
                self.haltSignal.emit(self.halt)
                self.starttime = None

            for row in range(self.force_continue_window.table.rowCount()):
                if not self.force_continue_window.table.item(row, 1).checkState():
                    self.statuses[
                        self.force_continue_window.table.item(row, 0).text()
                    ] = "0"

            event.accept()

        def handle_retry():
            if check_enabledModules():
                self.outputString.emit(f"Retrying {self.currentTest}...")
                for i in range(len(self.firmware)):
                    self.runwindow.ResultWidget.runtimes[i][
                        self.testIndexTracker
                    ].setText("")  # may need to .update()
                    self.runwindow.ResultWidget.ProgressBars[i][
                        self.testIndexTracker
                    ].setValue(
                        0
                    )  # may need to .update(). Automatically adds "0%" text on Progress bar.
                self.testIndexTracker -= 1
                self.force_continue_window.close()

        def handle_continue():
            if check_enabledModules():
                self.force_continue_window.close()

        # Connect buttons to handlers
        exit_button.clicked.connect(lambda: self.force_continue_window.close())
        retry_button.clicked.connect(handle_retry)
        continue_button.clicked.connect(handle_continue)

        self.force_continue_window.closeEvent = handle_close
        self.force_continue_window.exec_()  # This will block until the window is closed

    def upload_to_Panthera(self):
        self.updateProgressBar.emit(
            self.runwindow.UploadProgressBar, 0, f"{0}/{len(self.modules)} uploaded"
        )
        try:
            self.runwindow.UploadButton.setDisabled(True)
            counter = 0

            for module in self.modules:
                status, message = self.felis.upload_results(
                    module.getModuleName(),
                    self.master.username,
                    self.master.password,
                    type_sequence=self.info,
                    version_ph2acf=os.environ.get("PH2ACF_VERSION"),
                    version_testStationSoftware=os.environ.get("PH2_ACF_GUI_VERSION"),
                )
                if not status:
                    raise ConnectionError(message)

                counter += 1
                self.updateProgressBar.emit(
                    self.runwindow.UploadProgressBar,
                    100 * counter / len(self.modules),
                    f"{counter}/{len(self.modules)} uploaded",
                )

        except ConnectionError as e:
            error_message = repr(e)
            logger.error(error_message)
            self.master.errorMessageBoxSignal.emit(error_message)

        except Exception:
            if not self.master.panthera_connected:
                error_message = (
                    "Cannot upload test results, you are not signed in to Panthera."
                )
            else:
                error_message = "Failed to upload to Panthera."
                self.runwindow.UploadButton.setDisabled(False)
                if self.autoSave:
                    self.runwindow.UploadButton.setDisabled(
                        False
                    )  # if autosave fails, allow manual

            logger.error(error_message)
            self.master.errorMessageBoxSignal.emit(error_message)

    def bumpbond_analysis(self):
        runNumber = "000000" if self.RunNumber == "-1" else self.RunNumber
        commands = [
            ".L /home/cmsTkUser/Ph2_ACF_GUI/InnerTrackerTests/Analysis/bumpbond_analysis.cpp"
        ]
        command_template = ""

        if self.info == "FWD-RVS Bias":
            process = subprocess.run(
                'find /home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test/Results -type f -name "*SCurve.root"',
                shell=True,
                stdout=subprocess.PIPE,
            )
            all_root_files = sorted(
                process.stdout.decode("utf-8").rstrip("\n").split("\n")
            )
            relevant_root_files = all_root_files[-2:]

            save_file = f"Run{runNumber}_FWDRVS-Bias.root"
            commands.append(f'createROOTFile("{self.output_dir}/{save_file}")')
            command_template = (
                f'bias("{relevant_root_files[0]}", "{relevant_root_files[1]}", "{self.output_dir}/{save_file}", '
                + "const_cast<int*>(std::array<int, 4>{{{0}, {1}, {2}, {3}}}.data()))"
            )

        elif self.info == "Crosstalk":
            process = subprocess.run(
                'find /home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF/test/Results -type f -name "*PixelAlive.root"',
                shell=True,
                stdout=subprocess.PIPE,
            )
            all_root_files = sorted(
                process.stdout.decode("utf-8").rstrip("\n").split("\n")
            )
            relevant_root_files = all_root_files[-3:]

            save_file = f"Run{runNumber}_Crosstalk.root"
            commands.append(f'createROOTFile("{self.output_dir}/{save_file}")')
            command_template = (
                f'xtalk("{relevant_root_files[0]}", "{relevant_root_files[1]}", "{relevant_root_files[2]}", "{self.output_dir}/{save_file}",'
                + "const_cast<int*>(std::array<int, 4>{{{0}, {1}, {2}, {3}}}.data()))"
            )

        for beboard in self.firmware:
            boardID = beboard.getBoardID()
            for OG in beboard.getAllOpticalGroups().values():
                ogID = OG.getOpticalGroupID()
                for module in OG.getAllModules().values():
                    hybridID = module.getFMCPort()
                    for chipID in module.getEnabledChips().keys():
                        commands.append(
                            command_template.format(boardID, ogID, hybridID, chipID)
                        )
        executeCommandSequence(commands)
