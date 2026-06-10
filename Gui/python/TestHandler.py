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
from enum import Enum
import shutil
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
)

from Gui.python.ROOTInterface import executeCommandSequence
from felis.felis import Felis
from InnerTrackerTests.Analysis.IVCurve_CSV_to_ROOT import IVCurve_CSV_to_ROOT

from InnerTrackerTests.RootFilesDict import root_files

from InnerTrackerTests.Analysis.SLDO_CSV_to_ROOT import (
    SLDO_CSV_to_ROOT2,
    Trimbit_CSV_to_ROOT,
)


from Gui.QtGUIutils.QtMatplotlibUtils import ScanCanvas
from Gui.QtGUIutils.TessieCoolingApp import TessieCoolingApp

from Gui.python.TestValidator import ResultGrader
from Gui.python.ANSIColoringParser import parseANSI
from Gui.python.IVCurveHandler import IVCurveHandler
from Gui.python.SLDOScanHandler import SLDOCurveHandler
from Gui.python.TrimbitHandler import TrimbitCurveHandler
import Gui.siteSettings as site_settings
from Gui.python.logging_config import get_logger
from Gui.python.CustomizedWidget import chip_iref_db
from InnerTrackerTests.TestSequences import CompositeTests_Modules, Test_to_Ph2ACF_Map, OpenBumpTest
from Gui.siteSettings import icicle_instrument_setup, UI_testing
from InnerTrackerTests.QCSettings import CAL_EDGE_ADDEND


logger = get_logger(__name__)

INJECTION_DELAY_TESTS = {
    "InjectionDelay",
    "InjectionDelay_coupled",
    "InjectionDelay_uncoupled",
}

# Only plain InjectionDelay should define the propagated CAL_EDGE source.
INJECTION_DELAY_SOURCE_TESTS = {
    "InjectionDelay",
}

# Keep CAL_EDGE_FINE_DELAY static for xtalk-specific tests.
XTALK_STATIC_CAL_EDGE_TESTS = {
    "PixelAlive_coupled_xtalk",
    "PixelAlive_highcharge_xtalk",
    "PixelAlive_uncoupled_xtalk",
    "InjectionDelay_coupled",
    "InjectionDelay_uncoupled",
    "GenericDAC-DAC_coupled",
    "GenericDAC-DAC_uncoupled",
}


class DummyInstrumentCluster:
    def __init__(self):
        self._module_dict = {}
        self._instrument_dict = {}
        self.powering_groups = {}

    def get_modules(self):
        return {}

    def status(self):
        return {}

    def __getattr__(self, name):
        def _noop(*args, **kwargs):
            return None

        return _noop


# Setup enum for ProgressigMode. This ensures we don't use any unhandled states on accident
# Dont work about the values
class ProgressMode(Enum):
    NONE = 0
    SUMMARY = 1
    PERFORM = 2
    CONFIGURE = 3


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
        if self.instruments is None:
            self.instruments = DummyInstrumentCluster()
        self.mod_dict = {}
        self.fused_dict_index = [
            -1,
            -1,
        ]  # hybrid_id and chip_number gathered from Ph2_ACF output
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
        self.statuses = {
            module.getModuleName(): module.getEnabled() for module in self.modules
        }

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
        self.INTERNAL_NTC_ABSup = {channel: {} for channel in self.instruments._module_dict}
        self.INTERNAL_NTC_ABSdown = {channel: {} for channel in self.instruments._module_dict}

        self.INTERNAL_NTC_ABSupError = {channel: {} for channel in self.instruments._module_dict}
        self.INTERNAL_NTC_ABSdownError = {channel: {} for channel in self.instruments._module_dict}
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
            logger.error(traceback.format_exc())
            self.test_list = CompositeTests_Modules["Default"][self.info]

        self.module_test_history = {
            module.getModuleName(): {
                test: {"Passed": 0, "Failed": 0} for test in self.test_list
            }
            for module in self.modules
        }
        self.finished_tests = []
        self.Ph2_ACF_ver = os.environ.get("PH2ACF_VERSION")
        print("Using version {0} of Ph2_ACF".format(self.Ph2_ACF_ver))
        #self.firmwareImage = firmware_image[self.ModuleType][self.Ph2_ACF_ver]
        self.firmwareImage = firmware_image[self.ModuleType]
        print("Firmware version is {0}".format(self.firmwareImage))
        self.RunNumber = "-1"
        self.isTDACtuned = False

        self.IVCurveHandler = None
        self.SLDOScanHandler = None
        self.trimbitHandler = None
        self.starttime = None
        self._OBT_starttime = None

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
        self.outputFile = None
        self.errorFile = None
        self.comment = ""
        self.txt_files = txt_files if txt_files != {} else {}

        self.autoSave = False
        self.backSignal = False
        self.halt = False
        self.finishSignal = False
        self.proceedSignal = False

        self.runNext = threading.Event()
        self.testIndexTracker = 0
        self.listWidgetIndex = 0
        self.outputDirQueue = []
        # Fixme: QTimer to be added to update the page automatically

        # Need multiple felis instances to have two separate scratch directories
        # to handle multiple FC7s
        self.felis_instances = []
        felisScratchDirBase = "/home/cmsTkUser/Ph2_ACF_GUI/data/scratch"
        felis_directories = [
            os.path.join(felisScratchDirBase, fc7.getBoardName())
            for fc7 in self.firmware
        ]
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
            logger.error(traceback.format_exc())

        self.felis_instances = [
            Felis(felis_directory, False) for felis_directory in felis_directories
        ]
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

        self.ProgressingMode = [ProgressMode.NONE for _ in self.run_processes]

        self.ProgressValue = 0
        self.IVProgressValue = 0
        self.SLDOProgressValue = 0
        self.runtimeList = []
        self.starttime = None
        self.active_process_count = len(self.run_processes)

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
        self.powergroup = None
        for group_key, group in self.instruments.powering_groups.items():
            self.powergroup = group
        
        self._openBumpTest_running = False
        self._openBumpTest_subtest_index = 0
        self._retrying = False
        self._injection_delay_cal_edge_by_chip = {}

    def _module_is_enabled(self, module) -> bool:
        module_name = module.getModuleName()
        status = self.statuses.get(module_name, module.getEnabled())
        return str(status) == "1"

    def enabled_modules(self):
        return [module for module in self.modules if self._module_is_enabled(module)]

    def finished_run_process(self, _, exitStatus, i):
        logger.info("Inside finished_run_process")
        logger.info("Current exitStatus in finished_run_process: %s", exitStatus)
        
        if exitStatus == QProcess.NormalExit:
            # Ensure that all processes have finished before continuing
            self.on_finish(i)

    def initializeRD53Dict(self):
        self.rd53_file = {}
        for module in self.enabled_modules():
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
        for module in self.enabled_modules():
            ModuleIDs.append(str(module.getModuleName()))
        # output_dir gets set to $DATA_dir/Test_{testname}/Test_Module{ModuleID}_{Test}_{TimeStamp}
        return ConfigureTest(
            testName,
            "_Module".join(ModuleIDs),
            self.output_dir,
            self.input_dir,
        )

    def saveConfigs(self, process_index: int):
        ph2_acf_base_dir = os.environ.get("PH2ACF_BASE_DIR")
        logger.debug(f"{self.rd53_file.keys()=}")
        logger.debug(f"{self.ModuleMap=}")

        current_fc7 = self.firmware[process_index].getBoardName()
        # NOTE: ModuleMap key is of the form "{0}_{1}_{2}".format(beboardId, ogId, moduleId)
        # key used to be the module name and chip ie. SH0101_0_13

        # Modules that are associated with the finished QProcess that called saveConfigs
        finished_modules = {
            name
            for key, name in self.ModuleMap.items()
            if key.split("_")[0] == str(process_index)
        }

        # Module and enabled chip mapping
        module_chips_to_save = [
            chip
            for chip in list(self.rd53_file.keys())
            if chip.split("_")[0] in finished_modules
        ]

        # TODO Add process index to this function and use to format input directory
        for module in module_chips_to_save:
            try:
                shutil.copyfile(
                    f"{ph2_acf_base_dir}/test/{current_fc7}/Run{self.RunNumber}_CMSIT_RD53_{module}.txt",
                    f"{self.output_dir}/{current_fc7}/CMSIT_RD53_{module}_OUT.txt",
                )
            except Exception:
                traceback.print_exc()
                logger.error(
                    f"Failed to copy {ph2_acf_base_dir}/test/{current_fc7}/Run{self.RunNumber}_CMSIT_RD53_{module}.txt {self.output_dir}/{current_fc7}/CMSIT_RD53_{module}_OUT.txt"
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
            logger.warning(traceback.format_exc())

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
        logger.debug(f"{self.input_dir=}")
        logger.debug(f"{self.output_dir=}")

        # NOTE:  This code is to update the mapping of Ph2_ACF txt files
        if self.input_dir == "":
            # Copies file given in rd53[key] to test directory in Ph2_ACF test area as CMSIT_RD53.txt and the output dir.
            SetupRD53ConfigfromFile(self.rd53_file, self.output_dir)
        else:
            logger.debug(f"{self.testIndexTracker=}")
            print(os.listdir(self.input_dir))
            SetupRD53Config(self.input_dir, self.output_dir, self.rd53_file)

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
                            config_file, os.path.join(self.output_dir, firmware.getBoardName()), firmware.getBoardName()
                        )
                    else:
                        logger.warning("No Valid XML configuration file")
                        logger.warning(traceback.format_exc())
                    # QMessageBox.information(None,"Noitce", "Using default XML configuration",QMessageBox.Ok)
            else:
                for firmware in self.firmware:
                    SetupXMLConfigfromFile(
                        self.config_file, os.path.join(self.output_dir, firmware.getBoardName()), firmware.getBoardName()
                    )
        else:
            if self.config_file != "":
                for firmware in self.firmware:
                    SetupXMLConfigfromFile(
                        self.config_file, os.path.join(self.output_dir, firmware.getBoardName()), firmware.getBoardName()
                    )
            else:
                tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
                if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
                    try:
                        os.mkdir(tmpDir)
                        logger.info("Creating " + tmpDir)
                    except OSError:
                        logger.warning("Failed to create " + tmpDir)
                        logger.warning(traceback.format_exc())
                # Create the xml file from the text file
                for firmware in self.firmware:
                    config_file = GenerateXMLConfig(
                        firmware, self.currentTest, tmpDir, self.txt_files, **kwargs
                    )

                    if config_file:
                        SetupXMLConfigfromFile(
                            config_file, os.path.join(self.output_dir, firmware.getBoardName()), firmware.getBoardName()
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
        if UI_testing:
            self._simulate_run()
            return
        if reRun:
            self.halt = False
            self.testIndexTracker = 0
        testName = self.info

        self.input_dir = self.output_dir
        self.output_dir = ""

        if isCompositeTest(testName):
            self.runCompositeTest(testName)
        elif testName == "OpenBumpTest":
            self.runOpenBumpTest()
        elif isSingleTest(testName):
            self.runSingleTest(testName)
        else:
            QMessageBox.information(None, "Warning", "Not a valid test", QMessageBox.Ok)
            return

    def _simulate_run(self):
        tests = self.test_list if isCompositeTest(self.info) else (self.info,)
        for index, test_name in enumerate(tests):
            self.currentTest = test_name
            self.testIndexTracker = index
            for console in getattr(self.runwindow, "ConsoleViews", []):
                self.outputString.emit(
                    f"[SIM] Running {test_name} (no hardware)",
                    console,
                )

            for progress in (0, 25, 50, 75, 100):
                for fw_index in range(len(self.firmware)):
                    bar = self.runwindow.ResultWidget.ProgressBars[fw_index][index]
                    self.updateProgressBar.emit(
                        bar, progress, f"{progress}% (sim)"
                    )
                time.sleep(0.05)

            for fw_index in range(len(self.firmware)):
                runtime = self.runwindow.ResultWidget.runtimes[fw_index][index]
                runtime.setText("0.0s (sim)")

            results = [
                {
                    module.getModuleName(): (
                        True,
                        "Simulated run (no hardware)",
                    )
                }
                for module in self.modules
            ]
            self.finished_tests.append(test_name)
            self.updateValidation.emit(results)
            self.updateFinishedTests.emit(self.finished_tests)
            self.historyRefresh.emit()

        self.stepFinished.emit(True)

    # This loops over all the tests by using the on_finish pyqt decorator defined below
    def runCompositeTest(self, testName):
        logger.info("Inside runCompositeTest")
        if self.halt:
            return
        runTestList = self.test_list
        
        logger.debug("Past the Openbump test conditional block")
        if self.testIndexTracker == len(self.test_list):
            logger.debug("Reset testIndexTracker")
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
        logger.info("The testName is: %s", testName)
        if testName == "OpenBumpTest":
            logger.debug("Running OpenBumpTest")
            self.runOpenBumpTest()
            return
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
    def runOpenBumpTest(self):
        self._openBumpTest_running = True
        self._openBumpTest_subtest_index = 0
        subtest = OpenBumpTest[self._openBumpTest_subtest_index]
        logger.info(f"Running OpenBumpTest subtest: {subtest}")
        self.currentTest = subtest
        self.runSingleTest(subtest)

    def runADC(self):
        if "adc_board" in self.instruments._instrument_dict.keys():
            self.adc_board = self.instruments._instrument_dict["adc_board"]
            logger.info("Running with ADC.")
            self.adc_board.__enter__()

        else:
            logger.error(
                "You do not have instruments required to run a Trimbit scan connected.\nYou must have an Adc Board."
            )

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
            if (
                current
                < site_settings.SLDOScan_GADC[
                    self.master.module_in_use.split(" ")[-1].lower()
                ]["starting current"]
            ):
                continue
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
            enable_temp = self.shouldStoreSLDOTemperature(current)
            self.setSLDOTemperatureMonitoringInXML(fc7_index, enable_temp)
            process.start(
                "CMSITminiDAQ",
                [
                    "-f",
                    f"CMSIT_{self.firmware[fc7_index].getBoardName()}_{self.currentTest}.xml",
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

        if self.currentTest == "SLDOScan_GADC":
            self.ProgressValue += 1
            for i in range(len(self.firmware)):
                self.runwindow.ResultWidget.ProgressBars[i][
                    self.testIndexTracker
                ].setValue(100 * self.ProgressValue / total_steps)

    def runSingleTest(self, testName, nextTest=None):
        self.starttime = time.time()
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

        self.ProgressingMode = [ProgressMode.NONE for _ in self.ProgressingMode]

        self.currentTest = testName

        self.updateOptimizedXMLValues()
        self.configTest()
        if site_settings.cooler == "Tessie":
            # Log Tessie readings (temps/env) from widget cache at the start of each test
            try:
                self._log_tessie_from_widget("start")
            except Exception as e:
                logger.debug(f"Could not log Tessie readings from widget: {e}")

        # Make sure that the GUI is not trying to write to the root directory
        try:
            assert self.output_dir != ""
        except AssertionError:
            logger.exception(
                "Output directory was not formatted correctly, closing GUI to not write to root directory."
            )
            logger.error(traceback.format_exc())
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

                        self.makeSLDOPlot(data, f"{datatype}_ROC{int(chip)}", "GADC")
                        self.makeSLDOPlot(data, f"{datatype}_ROC{int(chip)}", "GADC")
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

        if "SLDOScan" in testName and "GADC" not in testName:
            self.currentTest = testName
            self.configTest()
            self.SLDOScanData = []
            self.SLDOProgressValue = 0
            self.SLDOScanHandler = SLDOCurveHandler(
                self.instruments,
                moduleType=self.ModuleType[5:],
                step_size=site_settings.SLDOScan_probecard[
                    self.master.module_in_use.split(" ")[-1].lower()
                ]["step size"],
                end_current=site_settings.SLDOScan_probecard[
                    self.master.module_in_use.split(" ")[-1].lower()
                ]["target current"],
                starting_current=site_settings.SLDOScan_probecard[
                    self.master.module_in_use.split(" ")[-1].lower()
                ]["starting current"],
                voltage_limit=site_settings.ModuleVoltageMapSLDO[
                    self.master.module_in_use
                ],
                execute_each_step=self.ramp_progress_bar,
                testhandler=self,
            )
            self.SLDOScanHandler.makeSLDOplotSignal.connect(self.makeSLDOPlot)
            self.SLDOScanHandler.finishedSignal.connect(self.SLDOScanFinished)
            self.SLDOScanHandler.progressSignal.connect(self.updateProgress)
            self.SLDOScanHandler.abortSignal.connect(self.urgentStop)
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("Beginning SLDOScan", console)
            self.SLDOScanHandler.SLDOScan()
            return

        # If the HV is not already on, turn it on.
        if (
            self.instruments
            and self.currentTest != "SLDOScan_GADC"
            and self.currentTest != "CommunicationTest"
        ):
            default_hv_voltage = site_settings.icicle_instrument_setup[
                "instrument_dict"
            ]["hv"]["default_voltage"]
            # assumes only 1 HV titled 'hv' in instruments.json
            hv_status = False
            #Checking the status of the HV supply
            for number in self.instruments.get_modules().keys():
                if self.instruments.status()[number]["hv"] == "1":
                    hv_status = True
                    break
            if not hv_status:
                self.instruments.hv_on(voltage=0, delay=0.5, step_size=10, no_lock=True)
                self.powergroup.enable_all()
                print("trying to turn on HV")
                self.powergroup.ramp_hv(
                    voltage=default_hv_voltage,
                    delay=0.3,
                    step_size=10,
                    execute_each_step=lambda: self.ramp_progress_bar(
                        [default_hv_voltage]
                        * len(self.instruments._module_dict.values())
                        ),
                    break_loop=lambda: self.halt,
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
        logger.info(f"{self.output_dir=}")

        # Track how many processes actually start for this run.
        self.active_process_count = 0

        # Create per-FC7 output directories
        # <output_dir>/<fc7>/output.txt
        # An error.txt file was created in the old version but never used.
        for firmware in self.firmware:
            fc7_dir = os.path.join(self.output_dir, firmware.getBoardName())
            try:
                os.makedirs(fc7_dir, exist_ok=True)
                # Ensure files exist (open in append mode then close)
                open(os.path.join(fc7_dir, "output.txt"), "a").close()
                # open(os.path.join(fc7_dir, "error.txt"), "a").close()
            except Exception:
                logger.error(f"Failed creating output dir for {fc7_dir}")
                logger.error(traceback.format_exc())

            # Check if the test was aborted
        if self.halt:
            print("Test aborted. Skipping QProcess setup.")
            return
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
                        f"Running COMMAND: CMSITminiDAQ  -f  CMSIT_{firmware.getBoardName()}_{self.currentTest}.xml -p"
                    ],
                )
        else:
            for process, firmware in zip(self.info_processes, self.firmware):
                process.start(
                    "echo",
                    [
                        "Running COMMAND: CMSITminiDAQ  -f  CMSIT_{0}.xml -c  {1}".format(
                            firmware.getBoardName(),
                            Test_to_Ph2ACF_Map[self.currentTest],
                        )
                    ],
                )

        for process in self.info_processes:
            process.waitForFinished(-1)

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
                    ["-f", f"CMSIT_{firmware.getBoardName()}_{self.currentTest}.xml", "-p"],
                )
                if process.state() != QProcess.NotRunning:
                    self.active_process_count += 1

        elif (
            self.currentTest == "IREF_GADC"
        ):  # FIXME need to add -t so the scan will stop at the end
            for process, firmware in zip(self.run_processes, self.firmware):
                process.start(
                    "CMSITminiDAQ",
                    [
                        "-f",
                        f"CMSIT_{firmware.getBoardName()}_{self.currentTest}.xml",
                        "-c",
                        "{}".format(Test_to_Ph2ACF_Map[self.currentTest]),
                        "-t",
                        "15",
                    ],
                )
                if process.state() != QProcess.NotRunning:
                    self.active_process_count += 1
        elif self.currentTest == "TrimbitScan":
            for process, firmware in zip(self.run_processes, self.firmware):
                process.start(
                    "CMSITminiDAQ",
                    [
                        "-f",
                        f"CMSIT_{firmware.getBoardName()}_{self.currentTest}.xml",
                    ],
                )
                if process.state() != QProcess.NotRunning:
                    self.active_process_count += 1

        elif self.currentTest == "OpenBumpTest" or self.currentTest in OpenBumpTest:
            i = 0
            for process, firmware in zip(self.run_processes, self.firmware):
                process.start(
                    "CMSITminiDAQ",
                    [
                        "-f",
                        f"CMSIT_{firmware.getBoardName()}_{self.currentTest}.xml",
                        "-c",
                        "{}".format(Test_to_Ph2ACF_Map[OpenBumpTest[self._openBumpTest_subtest_index]]),
                    ],
                )
        else:
            i = 0
            for process, firmware in zip(self.run_processes, self.firmware):
                process.start(
                    "CMSITminiDAQ",
                    [
                        "-f",
                        f"CMSIT_{firmware.getBoardName()}_{self.currentTest}.xml",
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
                    self.active_process_count += 1
                    logger.info(
                        f"Process for firmware {self.firmware[i].getBoardName()} started successfully."
                    )
                i += 1

    def abortTest(self):
        self.halt = True
        for process in self.run_processes:
            # Disconnect signals before killing to prevent unwanted finished signal
            try:
                process.finished.disconnect()
                process.readyReadStandardOutput.disconnect()
            except Exception:
                pass
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
            # Disconnect signals before killing to prevent unwanted finished signal
            try:
                process.finished.disconnect()
                process.readyReadStandardOutput.disconnect()
            except Exception:
                pass
            process.kill()
        self.halt = True
        self.haltSignal.emit(self.halt)
        self.starttime = None

    def _find_tessie_widget(self):
        """Try to find a TessieCoolingApp widget in the UI tree."""
        candidates = []
        try:
            candidates.append(getattr(self.master, 'window', None))
        except Exception:
            pass
        candidates.extend([self.master if hasattr(self, 'master') else None, self.runwindow])
        for parent in filter(None, candidates):
            try:
                w = parent.findChild(TessieCoolingApp)
                if w is not None:
                    return w
            except Exception:
                continue
        # Fallback to a direct attribute if provided by the app
        w = getattr(self.master, 'tessie_widget', None)
        return w

    def _log_tessie_from_widget(self, phase: str = "start"):
        """Fetch cached Tessie readings from the TessieCoolingApp widget and persist them.
        Uses widget cache only, no direct instrument queries.

        phase: 'start' or 'end' to control filename and console label.
        - start -> writes tessie_start_temps.json (kept for backward compatibility)
        - end   -> writes tessie_end_values.json
        """
        widget = self._find_tessie_widget()
        temps = None
        rh = None
        dp = None
        setpoints = None
        if widget:
            try:
                if hasattr(widget, 'get_latest_temperatures'):
                    temps = widget.get_latest_temperatures()
            except Exception:
                temps = None
            try:
                if hasattr(widget, 'get_latest_env'):
                    env = widget.get_latest_env()
                    if isinstance(env, tuple):
                        rh, dp = env
            except Exception:
                rh, dp = None, None
            try:
                if hasattr(widget, 'get_latest_setpoints'):
                    setpoints = widget.get_latest_setpoints()
            except Exception:
                setpoints = None

        ts = datetime.now().strftime("%H:%M:%S")
        # Build console message
        label = "start-of-test" if str(phase).lower().startswith("s") else "end-of-test"
        parts = []
        if isinstance(temps, list) and len(temps) == 8:
            parts.append("temps=" + ", ".join(f"{t:.2f}°C" for t in temps))
        if isinstance(rh, (int, float)):
            parts.append(f"RH={rh:.1f}%")
        if isinstance(dp, (int, float)):
            parts.append(f"DP={dp:.2f}°C")
        # Include setpoints if available
        try:
            if isinstance(setpoints, list) and len(setpoints) == 8:
                uniq = {round(float(x), 2) for x in setpoints if isinstance(x, (int, float))}
                if len(uniq) == 1:
                    sval = next(iter(uniq))
                    parts.append(f"set={sval:.2f}°C")
                else:
                    parts.append("set=[" + ", ".join(
                        f"{float(x):.2f}°C" if isinstance(x, (int, float)) else str(x)
                        for x in setpoints
                    ) + "]")
        except Exception:
            pass

        msg = f"[{ts}] Tessie {label} (widget): " + ("; ".join(parts) if parts else "unavailable")

        try:
            for console in getattr(self.runwindow, 'ConsoleViews', []):
                self.outputString.emit(msg, console)
        except Exception:
            pass
        logger.info(msg)

        # Persist snapshot to output dir
        try:
            if getattr(self, 'output_dir', None):
                import json

                # Keep filenames as before for compatibility, but store them under each FC7 board dir.
                if str(phase).lower().startswith("s"):
                    file_name = "tessie_start_temps.json"
                else:
                    file_name = "tessie_end_values.json"

                payload = {
                    "source": "widget",
                    "timestamp": ts,
                    "temperatures": temps,
                    "relative_humidity": rh,
                    "dew_point": dp,
                    "phase": "start" if str(phase).lower().startswith("s") else "end",
                    "temperature_setpoints": setpoints,
                }

                board_dirs = [
                    os.path.join(self.output_dir, firmware.getBoardName())
                    for firmware in self.firmware
                ]
                for board_dir in board_dirs:
                    os.makedirs(board_dir, exist_ok=True)
                    out_path = os.path.join(board_dir, file_name)
                    with open(out_path, "w") as f:
                        json.dump(payload, f, indent=2)
        except Exception:
            pass

    def validateTest(self):
        for process in self.run_processes:
            if process.state() == QProcess.Running:
                return

        self.finished_tests.append(self.currentTest)
        if site_settings.cooler == "Tessie":
            # Snapshot end-of-test Tessie values (from widget cache only)
            try:
                self._log_tessie_from_widget("end")
            except Exception:
                pass
        try:
            passed = []
            results = []
            runNumber = "000000" if self.RunNumber == "-1" else self.RunNumber

            for i, beboard in enumerate(self.firmware):
                boardID = beboard.getBoardID()
                for OG in beboard.getAllOpticalGroups().values():
                    ogID = OG.getOpticalGroupID()
                    for module in OG.getAllModules().values():
                        if not self._module_is_enabled(module):
                            continue
                        print(f"curr test {self.currentTest}")
                        hybridID = module.getFMCPort()
                        module_data = {
                            "boardID": boardID,
                            "ogID": ogID,
                            "hybridID": hybridID,
                            "module": module,
                        }
                        output_dir = os.path.join(
                            self.output_dir, beboard.getBoardName()
                        )
                        result, self.BBanalysis_root_files = ResultGrader(
                            self.felis_instances[i],
                            output_dir,
                            self.currentTest,
                            self.testIndexTracker,
                            runNumber,
                            module_data,
                            self.BBanalysis_root_files,
                            self.info,
                            self.registerKey,
                            self.communicationTestResults,
                        )

                        results.append(result)
                        passed.append(list(result.values())[0][0])

                        self.module_test_history[next(iter(result))][self.currentTest][
                            "Passed" if next(iter(result.values()))[0] else "Failed"
                        ] += 1

                        self.figurelist[module.getModuleName()] = self.collect_plots(
                            module.getModuleName(),
                            felis_instance=self.felis_instances[i],
                        )

            self.updateValidation.emit(results)
            self.updateFinishedTests.emit(
                self.finished_tests
            )  # Obsolete at time of commit: "return all(passed)"
        except Exception:
            logger.error(traceback.format_exc())

    def collect_plots(self, moduleName, felis_instance=None):
        if felis_instance is not None:
            try:
                plot_paths = []
                scratch = os.path.join(felis_instance.path_scratch, moduleName)
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

            else:
                logger.error((traceback.format_exc()))
                return []

    # For root files with the same RunNumber in the PH2ACF directory, this function only copies over to
    # self.output_dir the .root file modified most recently. This will copy over the wrong file if somebody
    # manually edits the .root file in the PH2ACF directory, so there may be a better way to do this
    def copyMostRecentRootFile(self, RunNumber, base_dir, output_dir, test):
        logger.debug(f"{RunNumber=}, {base_dir=}, {output_dir=}, {test=}")
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
            search_pattern = f"{base_dir}/Run{RunNumber}_{name}_Board*.root"
            logger.debug(f"Looking for {search_pattern}")

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

            # When using multiple FC7s root files will get overwritten so need to attach
            # what fc7 the test was run on to file name
            fc7_in_use: str = base_dir.split("/")[-1].replace(".", "_")

            for file in matching_files:
                file_name: str = file.split("/")[-1]
                logger.debug(
                    f"Copying {file} to {output_dir}/{fc7_in_use}_{file_name}"
                )
                shutil.copyfile(file, f"{output_dir}/{fc7_in_use}_{file_name}")

    def saveTest(self, processIndex: int, process: QProcess):
        logger.debug("Inside saveTest")
        if process.state() == QProcess.Running:
            QMessageBox.critical(self, "Error", "Process not finished", QMessageBox.Ok)
            return

        try:
            logger.debug(f"{self.output_dir=}")
            if not os.path.exists(
                os.path.join(
                    self.output_dir, self.firmware[processIndex].getBoardName()
                )
            ):
                logger.debug(f"Created directory for process: {processIndex}")
                os.makedirs(
                    os.path.join(
                        self.output_dir, self.firmware[processIndex].getBoardName()
                    )
                )

            if self.RunNumber == "-1":      
                dest_dir = os.path.join(self.output_dir,
                                         self.firmware[processIndex].getBoardName()
                )

                for file_path in glob.glob(
                    os.path.join(
                        os.environ.get("PH2ACF_BASE_DIR"),
                        "test",
                        "Results",
                        "Run000000*.root"
                    )
                ):
                    try:
                        shutil.copy(file_path, dest_dir)
                    except (shutil.Error, IOError) as e:
                        logger.error(f"Error copying {file_path}: {e}")


            elif "IVCurve" in self.currentTest:
                print("copying MonitorDQM.root file to output directory")

                current_fc7: str = self.firmware[processIndex].getBoardName()

                for file_path in glob.glob(
                    os.path.join(
                        os.environ.get("PH2ACF_BASE_DIR"),
                        "test",
                        "Results",
                        "Run{0}_MonitorDQM_Board_{1}*.root".format(
                            self.RunNumber, self.firmware[processIndex].getBoardID()))
                        ):              
                            shutil.copy(
                                file_path,
                                os.path.join(
                                    self.output_dir,
                                    current_fc7,
                                    os.path.basename(file_path)
                                )
                            )

                shutil.copy(
                    f"{os.environ.get('PH2ACF_BASE_DIR')}/test/Results/Run{self.RunNumber}_CMSIT_{current_fc7}_{self.currentTest}.xml",
                    os.path.join(self.output_dir, current_fc7)
                    )

            elif "IREF_GADC" in self.currentTest:
                print("copying MonitorDQM.root file to output directory")
                current_fc7: str = self.firmware[processIndex].getBoardName()
                os.system(
                    "cp {0}/test/Results/Run{1}_MonitorDQM_Board_{2}*.root {3}/".format( #Chaneged from {0}/test/Results/Run{1}_MonitorDQM_Board_{2}*.root {3}
                        os.environ.get("PH2ACF_BASE_DIR"),
                        self.RunNumber,
                        self.firmware[processIndex].getBoardID(),
                        os.path.join(
                            self.output_dir, current_fc7
                        ),
                    )
                )
                shutil.copy(
                    f"{os.environ.get('PH2ACF_BASE_DIR')}/test/Results/Run{self.RunNumber}_CMSIT_{current_fc7}_{self.currentTest}.xml",
                    os.path.join(self.output_dir, current_fc7)
                    )

            else:
                ph2_acf_base_dir: str | None = os.environ.get("PH2ACF_BASE_DIR")
                if ph2_acf_base_dir is None:
                    raise OSError("PH2ACF_BASE_DIR environmentvariable is not set.")
                current_fc7: str = self.firmware[processIndex].getBoardName()
                self.copyMostRecentRootFile(
                    self.RunNumber,
                    ph2_acf_base_dir + f"/test/{current_fc7}",
                    os.path.join(self.output_dir, current_fc7),
                    self.currentTest,
                )

        except Exception:
            logger.error(traceback.format_exc())
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

        # Write output to the per-FC7 output file so multiple FC7s don't collide
        try:
            fc7name = self.firmware[processIndex].getBoardName()
            outpath = os.path.join(self.output_dir, fc7name, "output.txt")
            mode = "a" if os.path.exists(outpath) else "w"
            with open(outpath, mode) as outputfile:
                outputfile.write(alltext)
        except Exception:
            logger.error("Failed writing FC7 output to per-FC7 file")
            logger.error(traceback.format_exc())
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

                if self._openBumpTest_running:
                    if self._OBT_starttime is None: 
                        self._OBT_starttime = time.time()    
                    self.starttime = self._OBT_starttime

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
                logger.warning(traceback.format_exc())
            if "@@@ End of CMSIT miniDAQ @@@" in textStr:
                self.ProgressingMode[processIndex] = ProgressMode.SUMMARY
            if self.ProgressingMode[processIndex] == ProgressMode.PERFORM:
                if "Progress:" in textStr:
                    try:
                        index = textStr.split().index("Progress:") + 2
                        # self.ProgressValue = float(textStr.split()[index].rstrip("%"))
                        self.ProgressValue = float(
                            re.sub(r"\x1b\[\d+m", "", textStr.split()[index].strip("%"))
                        )
                        # Check if openBumpTest is running
                        if self._openBumpTest_running:
                            scaled_progress_value = (self._openBumpTest_subtest_index * (100/3) + (self.ProgressValue / 3))
                            self.runwindow.ResultWidget.ProgressBars[processIndex][self.testIndexTracker].setValue(scaled_progress_value)

                        else:
                            if self.ProgressValue == 100:
                                self.ProgressingMode[processIndex] = ProgressMode.SUMMARY
                            self.runwindow.ResultWidget.ProgressBars[processIndex][
                                self.testIndexTracker
                            ].setValue(self.ProgressValue)

                    except Exception as e:
                        print(f"Error while updating progress bar {e}")
                        logger.error(traceback.format_exc())
                        pass

                if self.check_for_end_of_test(textStr, processIndex):
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
                        logger.error("Failed due to {0}".format(e))
                        logger.error(traceback.format_exc())
                elif "INTERNAL_NTC" in textStr:
                    try:
                        ansi_pattern = re.compile(r"\x1B[@-_][0-?]*[ -/]*[@-~]")
                        clean_text = ansi_pattern.sub("", textStr)
                        if "INTERNAL_NTC" in clean_text:
                            sensor = (
                                clean_text.split("INTERNAL_NTC_ABS:")[1]
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
                        logger.error("Failed due to {0}".format(e))
                        logger.error(traceback.format_exc())
                text = textStr.encode("ascii")
                _, text = parseANSI(text)
                self.outputString.emit(
                    text.decode("utf-8"), self.runwindow.ConsoleViews[processIndex]
                )
                continue
            # This next block needs to be edited once Ph2ACF bug is fixed.  Remove the Fixme when ready.

            elif self.ProgressingMode[processIndex] == ProgressMode.SUMMARY:
                if self.check_for_end_of_test(textStr, processIndex):
                    self.runwindow.ResultWidget.ProgressBars[processIndex][
                        self.testIndexTracker
                    ].setValue(100)
            elif "@@@ Initializing the Hardware @@@" in textStr:
                self.ProgressingMode[processIndex] = ProgressMode.CONFIGURE
            elif "@@@ Performing" in textStr:
                self.ProgressingMode[processIndex] = ProgressMode.PERFORM
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
                    logger.error(
                        "Module name not found before CommunicationTest result in test output."
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
                    logger.error(
                        "Module name not found before CommunicationTest result in test output."
                    )

                else:
                    self.communicationTestResults[self.communicationTestModule] = True
                    self.communicationTestModule = None

        else:
            if "FIFO empty" in alltext or "Reached maximum number of attempts" in alltext:
                if self.run_processes[processIndex].state() == QProcess.Running:
                    logger.info("process is still running...  Attempting to terminate")
                    self.run_processes[processIndex].terminate()
                    if not self.run_processes[processIndex].waitForFinished(3000):
                        logger.warning("Process would not terminate, so killing it now...")
                        self.run_processes[processIndex].kill()
                self.forceContinue(self.firmware[processIndex])

        self.readingOutput = False

    def updateOptimizedXMLValues(self):
        print("trying to update the xml value")
        try:
            if Test_to_Ph2ACF_Map[self.currentTest] in optimizationTestMap.keys():
                updatedFEKeys = optimizationTestMap[
                    Test_to_Ph2ACF_Map[self.currentTest]
                ]
                for module in self.enabled_modules():
                    chipIDs = [
                        chip.getID()
                        for chip in module.getChips().values()
                        if chip.getStatus()
                    ]

                    hybridID = module.getFMCPort()
                    logger.debug("HybridID {0}".format(hybridID))
                    logger.debug("chipIDs {0}".format(chipIDs))
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
        except Exception:
            logger.error(traceback.format_exc())

        # Ensure CAL_EDGE_FINE_DELAY overrides are only active when intended.
        self._clear_cal_edge_fine_delay_overrides()
        self._apply_cal_edge_fine_delay_offset_after_injection_delay()

    def _clear_cal_edge_fine_delay_overrides(self):
        removed_overrides = 0
        for chip_values in updatedXMLValues.values():
            if isinstance(chip_values, dict):
                if "CAL_EDGE_FINE_DELAY" in chip_values:
                    removed_overrides += 1
                chip_values.pop("CAL_EDGE_FINE_DELAY", None)
        if removed_overrides:
            logger.debug(
                "Cleared %d stale CAL_EDGE_FINE_DELAY override(s) before test '%s'",
                removed_overrides,
                self.currentTest,
            )

    def _is_post_injection_delay_test(self) -> bool:
        if not isCompositeTest(self.info):
            logger.debug(
                "Skipping CAL_EDGE_FINE_DELAY offset for test '%s': not a composite sequence",
                self.currentTest,
            )
            return False
        previous_tests = self.test_list[: self.testIndexTracker]
        has_injection_delay_before = any(
            test in INJECTION_DELAY_TESTS for test in previous_tests
        )
        if not has_injection_delay_before:
            logger.debug(
                "Skipping CAL_EDGE_FINE_DELAY offset for test '%s': no InjectionDelay test found before index %d",
                self.currentTest,
                self.testIndexTracker,
            )
        return has_injection_delay_before

    def _read_cal_edge_fine_delay_from_txt(self, module_name: str, module_id: str, chip_id: str):
        ph2_acf_base_dir = os.environ.get("PH2ACF_BASE_DIR")
        if not ph2_acf_base_dir:
            return None

        txt_path = os.path.join(
            ph2_acf_base_dir,
            "test",
            f"CMSIT_RD53_{module_name}_{module_id}_{chip_id}.txt",
        )
        if not os.path.isfile(txt_path):
            logger.debug(
                "CAL_EDGE source txt not found for module '%s' hybrid '%s' chip '%s': %s",
                module_name,
                module_id,
                chip_id,
                txt_path,
            )
            return None

        try:
            with open(txt_path, "r", encoding="utf-8") as txt_file:
                for line in txt_file:
                    if "CAL_EDGE_FINE_DELAY" not in line:
                        continue
                    match = re.search(r"CAL_EDGE_FINE_DELAY\s+0x[0-9A-Fa-f]+\s+(0x[0-9A-Fa-f]+)", line)
                    if match:
                        hex_value = match.group(1)
                        decimal_value = int(hex_value, 16)
                        logger.debug(
                            "Read CAL_EDGE_FINE_DELAY=%s (0x%s) from %s",
                            decimal_value,
                            hex_value[2:],
                            txt_path,
                        )
                        return decimal_value
            logger.debug(
                "CAL_EDGE_FINE_DELAY not present in source txt for module '%s' hybrid '%s' chip '%s': %s",
                module_name,
                module_id,
                chip_id,
                txt_path,
            )
        except OSError:
            logger.warning("Failed reading CAL_EDGE_FINE_DELAY from %s", txt_path)
            logger.warning(traceback.format_exc())
        return None

    def _cache_injection_delay_cal_edges_from_output_dir(self, output_dir: str):
        if not output_dir or not os.path.isdir(output_dir):
            return

        cached_values = {}
        out_pattern = os.path.join(output_dir, "*", "CMSIT_RD53_*_OUT.txt")
        out_files = glob.glob(out_pattern)
        if not out_files:
            return

        for out_file in out_files:
            basename = os.path.basename(out_file)
            file_match = re.match(r"CMSIT_RD53_(.+)_([^_]+)_([^_]+)_OUT\.txt$", basename)
            if not file_match:
                continue
            module_id = file_match.group(2)
            chip_id = file_match.group(3)
            chip_key = f"{module_id}/{chip_id}"

            try:
                with open(out_file, "r", encoding="utf-8") as txt_file:
                    for line in txt_file:
                        if "CAL_EDGE_FINE_DELAY" not in line:
                            continue
                        value_match = re.search(
                            r"CAL_EDGE_FINE_DELAY\s+0x[0-9A-Fa-f]+\s+(0x[0-9A-Fa-f]+)",
                            line,
                        )
                        if value_match:
                            cached_values[chip_key] = int(value_match.group(1), 16)
                            break
            except OSError:
                logger.warning(
                    "Failed reading InjectionDelay CAL_EDGE cache source file: %s",
                    out_file,
                )
                logger.warning(traceback.format_exc())

        if cached_values:
            self._injection_delay_cal_edge_by_chip = cached_values
            logger.debug(
                "Cached InjectionDelay CAL_EDGE_FINE_DELAY for %d chip(s) from '%s'",
                len(cached_values),
                output_dir,
            )

    def _apply_cal_edge_fine_delay_offset_after_injection_delay(self):
        if self.currentTest in XTALK_STATIC_CAL_EDGE_TESTS:
            logger.debug(
                "Keeping static CAL_EDGE_FINE_DELAY for xtalk/static test '%s'",
                self.currentTest,
            )
            return
        if not self._is_post_injection_delay_test():
            return

        logger.debug(
            "Applying CAL_EDGE_FINE_DELAY offset for post-InjectionDelay test '%s' (addend=%d)",
            self.currentTest,
            CAL_EDGE_ADDEND,
        )

        applied_overrides = 0

        for module in self.enabled_modules():
            hybrid_id = module.getFMCPort()
            module_name = module.getModuleName()
            module_id = module.getFMCPort()

            for chip in module.getChips().values():
                if not chip.getStatus():
                    continue

                chip_id = chip.getID()
                chip_key = f"{hybrid_id}/{chip_id}"
                cal_edge = self._injection_delay_cal_edge_by_chip.get(chip_key)
                if cal_edge is None:
                    cal_edge = self._read_cal_edge_fine_delay_from_txt(
                        module_name=str(module_name),
                        module_id=str(module_id),
                        chip_id=str(chip_id),
                    )
                if cal_edge is None:
                    continue

                adjusted_cal_edge = cal_edge + CAL_EDGE_ADDEND
                if chip_key not in updatedXMLValues:
                    updatedXMLValues[chip_key] = {}
                updatedXMLValues[chip_key]["CAL_EDGE_FINE_DELAY"] = str(adjusted_cal_edge)
                applied_overrides += 1
                logger.debug(
                    "Set CAL_EDGE_FINE_DELAY override for test '%s' chip '%s': base=%d addend=%d final=%d",
                    self.currentTest,
                    chip_key,
                    cal_edge,
                    CAL_EDGE_ADDEND,
                    adjusted_cal_edge,
                )

        if applied_overrides == 0:
            logger.debug(
                "No CAL_EDGE_FINE_DELAY overrides were applied for test '%s'",
                self.currentTest,
            )
        else:
            logger.debug(
                "Applied %d CAL_EDGE_FINE_DELAY override(s) for test '%s'",
                applied_overrides,
                self.currentTest,
            )

    def check_for_end_of_test(self, textStr, processIndex=0):
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
        elif (
            "IREF_GADC" == self.currentTest
            and self.ProgressingMode[processIndex] == ProgressMode.SUMMARY
        ):
            return True
        return False

    # Reads data that is normally printed to the terminal and saves it to the output file
    @QtCore.pyqtSlot()
    def on_readyReadStandardOutput_info(self, processIndex: int):
        alltext = (
            self.info_processes[processIndex].readAllStandardOutput().data().decode()
        )
        try:
            fc7name = self.firmware[processIndex].getBoardName()
            outpath = os.path.join(self.output_dir, fc7name, "output.txt")
            mode = "a" if os.path.exists(outpath) else "w"
            with open(outpath, mode) as outputfile:
                outputfile.write(alltext)
        except Exception:
            logger.error("Failed writing FC7 info output to per-FC7 file")
            logger.error(traceback.format_exc())
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
        try:
            fc7name = self.firmware[fc7_index].getBoardName()
            outpath = os.path.join(self.output_dir, fc7name, "output.txt")
            with open(outpath, "a") as outputfile:
                outputfile.write(alltext)
        except Exception:
            logger.error("Failed writing VDDsweep output to per-FC7 file")
            logger.error(traceback.format_exc())
        textline = alltext.split("\n")
        for textStr in textline:
            try:
                text = textStr.encode("ascii")
                _, text = parseANSI(text)

                self.outputString.emit(
                    text.decode("utf-8"), self.runwindow.ConsoleViews[fc7_index]
                )
            except Exception:
                logger.error("Error emitting console output")
                logger.error(traceback.format_exc())
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
    def shouldStoreSLDOTemperature(self, current):
        module_key = self.master.module_in_use.split(" ")[-1].lower()
        cfg = site_settings.SLDOScan_GADC[module_key]

        return (
            np.isclose(current, cfg["starting current"])
            or np.isclose(current, cfg["target current"])
        )
    
    @QtCore.pyqtSlot()
    def on_readyReadStandardOutput_GADC(
        self, process: QProcess, fc7_index: int, upOrDown: str, current, channel
    ):
        if self.readingOutput:
            print("Thread competition detected")
            return
        self.readingOutput = True

        alltext = process.readAllStandardOutput().data().decode()

        try:
            fc7name = self.firmware[fc7_index].getBoardName()
            outpath = os.path.join(self.output_dir, fc7name, "output.txt")
            mode = "a" if os.path.exists(outpath) else "w"
            logger.debug(f"{mode=}")
            with open(outpath, mode) as outputfile:
                outputfile.write(alltext)
        except Exception:
            logger.error("Failed writing GADC output to per-FC7 file")
            logger.error(traceback.format_exc())
        textline = alltext.split("\n")

        for textStr in textline:
            text = textStr.encode("ascii")
            _, text = parseANSI(text)
            self.outputString.emit(
                text.decode("utf-8"), self.runwindow.ConsoleViews[fc7_index]
            )

            self.runwindow.ConsoleViews[fc7_index].update()

            textStr = re.compile(r"\x1B[@-_][0-?]*[ -/]*[@-~]").sub("", textStr)
            match = re.search(
                r"data for \[board/opticalGroup/hybrid/chip = (\d+)/(\d+)/(\d+)/(\d+)\]",
                textStr,
            )
            if match:
                self.GADC_meas_chip = match.group(4)
            else:
                #match = re.search(r"(\w+):\s*([\d.]+)\s*\+/-\s*([\d.]+)\s*V", textStr)
                match = re.search(
                    r"(\w+):\s*([-+]?\d*\.?\d+)\s*\+/-\s*([-+]?\d*\.?\d+)\s*([A-Za-z]+)",
                    textStr,
                )
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
                        elif match.group(1) == "INTERNAL_NTC_ABS":
                            if not self.shouldStoreSLDOTemperature(current):
                                continue

                            if (
                                self.GADC_meas_chip
                                not in getattr(self, match.group(1) + upOrDown)[channel]
                            ):
                                getattr(self, match.group(1) + upOrDown)[channel][
                                    self.GADC_meas_chip
                                ] = {}

                            getattr(self, match.group(1) + upOrDown)[channel][
                                self.GADC_meas_chip
                            ][current] = float(match.group(2))

                            if (
                                self.GADC_meas_chip
                                not in getattr(self, match.group(1) + upOrDown + "Error")[channel]
                            ):
                                getattr(self, match.group(1) + upOrDown + "Error")[channel][
                                    self.GADC_meas_chip
                                ] = {}

                            getattr(self, match.group(1) + upOrDown + "Error")[channel][
                                self.GADC_meas_chip
                            ][current] = float(match.group(3))
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

        logger.debug("All processes finished")
        
        # If this is a retried test completing, clear the retry flag so it can proceed normally
        if self._retrying:
            self._retrying = False
            logger.info("Retry test completing - clearing retry flag")

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
            # Check if retry was triggered during saveTest
            if self._retrying:
                logger.info("Retry initiated during IVCurve saveTest - aborting on_finish")
                return
            return

        # Save the output ROOT file to output_dir
        logger.debug("About to run saveTest()")
        time.sleep(1)
        self.saveTest(processIndex, self.run_processes[processIndex])
        
        # Check if retry was triggered during saveTest
        if self._retrying:
            logger.info("Retry initiated during saveTest - aborting on_finish")
            return

        self.saveConfigs(process_index=processIndex)
        if self.currentTest in INJECTION_DELAY_SOURCE_TESTS:
            self._cache_injection_delay_cal_edges_from_output_dir(self.output_dir)
        # Don't continue on sequence until all processes have finished the current test
        if self._openBumpTest_running:
            self.finished_processes += 1
            expected_processes = (
                self.active_process_count
                if self.active_process_count > 0
                else len(self.run_processes)
            )
            if not self.finished_processes == expected_processes:
                return
            self.finished_processes = 0
            self._openBumpTest_subtest_index += 1
            if self._openBumpTest_subtest_index < len(OpenBumpTest):
                subtest = OpenBumpTest[self._openBumpTest_subtest_index]
                logger.info(f"Running next OpenBumpTest subtest: {subtest}")
                self.currentTest = subtest
                self.input_dir = self.output_dir  # Chain the output of the last test as input for the next
                #self.output_dir = "" # Reset output directory to force creation of a new one
                self.runSingleTest(subtest)
                return  # Skip normal finish routine
            else:
                # Finished all subtests
                self.currentTest = "OpenBumpTest"
                logger.info("All OpenBumpTest subtests finished. Validating...")
                print(f"Current openbumptest subtest index = {self._openBumpTest_subtest_index}")
                self._openBumpTest_running = False

        # Ensure that all processes have finished before continuing
        else:
            self.finished_processes += 1
            expected_processes = (
                self.active_process_count
                if self.active_process_count > 0
                else len(self.run_processes)
            )
            if not self.finished_processes == expected_processes:
                return

            self.finished_processes = 0

        # validate the results
        logger.debug("About to run validateTest()")
        self.validateTest()

        logger.debug("testIndexTracker before increment: %i", self.testIndexTracker)
        self.testIndexTracker += 1

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
                logger.info("index == len.self.test_list")
                self.powerSignal.emit()
                EnableReRun = True

                if self.info == "FWD-RVS Bias" or self.info == "CrossTalk":
                    self.bumpbond_analysis()

                if (
                    len(self.BBanalysis_root_files) > 0
                    and "analyze" in self.currentTest
                ):
                    for fc7_index, beboard in enumerate(self.firmware):
                        boardID = beboard.getBoardID()
                        for OG in beboard.getAllOpticalGroups().values():
                            ogID = OG.getOpticalGroupID()
                            for module in OG.getAllModules().values():
                                if not self._module_is_enabled(module):
                                    continue
                                hybridID = module.getFMCPort()
                                module_data = {
                                    "boardID": boardID,
                                    "ogID": ogID,
                                    "hybridID": hybridID,
                                    "module": module,
                                }
                                index -= 1
                                print(
                                    f"self.BBanalysis_root_files: {self.BBanalysis_root_files}")
                                self.felis_instances[fc7_index].set_result(
                                    self.BBanalysis_root_files,
                                    module_data["module"].getModuleName(),
                                    f"{index:02d}_CrossTalk",
                                    "crosstalk",
                                )
                                self.figurelist[module.getModuleName()] = (
                                    self.collect_plots(
                                        module.getModuleName(),
                                        felis_instance=self.felis_instances[fc7_index],
                                    )
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
        if self.starttime is not None:
            self.currentTime = time.time()
            runningTime = self.currentTime - self.starttime
            for i, firmware in enumerate(self.firmware):
                self.runwindow.ResultWidget.runtimes[i][self.testIndexTracker].setText(
                    "{0} s".format(round(runningTime, 1))
                )

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
                #self.runwindow.ResultWidget.ProgressBars[i][
                #    self.testIndexTracker
                #].setValue(self.SLDOProgressValue)

    def setSLDOTemperatureMonitoringInXML(self, fc7_index, enable):
        import xml.etree.ElementTree as ET

        xml_path = os.path.join(
            os.environ.get("PH2ACF_BASE_DIR"),
            "test",
            f"CMSIT_{self.firmware[fc7_index].getBoardName()}_{self.currentTest}.xml",
        )

        tree = ET.parse(xml_path)
        root = tree.getroot()

        enable_value = "1" if enable else "0"
        found = False

        for element in root.iter("MonitoringElement"):
            if (
                element.get("device") == "RD53"
                and element.get("register") == "INTERNAL_NTC_ABS"
            ):
                element.set("enable", enable_value)
                found = True

        if not found:
            logger.warning(
                f"INTERNAL_NTC_ABS MonitoringElement not found in {xml_path}"
            )
            return

        tree.write(xml_path)
    def makeSLDOPlot(self, total_result: np.ndarray, pin: str, method: str):
        for module in self.enabled_modules():
            moduleName = module.getModuleName()
            fc7name = module.getOpticalGroup().getBeBoard().getBoardName()
            filename = "{0}/SLDOCurve_Module_{1}_{2}_{3}.svg".format(
                os.path.join(self.output_dir, fc7name), moduleName, pin, method
            )
            csvfilename = "{0}/SLDOCurve_Module_{1}_{2}_{3}.csv".format(
                os.path.join(self.output_dir, fc7name), moduleName, pin, method
            )
            self.SLDOfilelist.append(csvfilename)
            # The pin is passed here, so we can use that as the key in the chipmap dict from settings.py
            total_result_stacked = np.vstack(total_result)
            os.makedirs(os.path.dirname(csvfilename), exist_ok=True)
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

    def makeSLDOTemperatureCSV(self, total_result: np.ndarray, chip: str, method: str):
        for module in self.enabled_modules():
            moduleName = module.getModuleName()
            fc7name = module.getOpticalGroup().getBeBoard().getBoardName()

            csvfilename = "{0}/SLDOTemperature_Module_{1}_INTERNAL_NTC_ABS_ROC{2}_{3}.csv".format(
                os.path.join(self.output_dir, fc7name), moduleName, chip, method
            )

            self.SLDOfilelist.append(csvfilename)

            total_result_stacked = np.vstack(total_result)
            os.makedirs(os.path.dirname(csvfilename), exist_ok=True)
            np.savetxt(csvfilename, total_result_stacked, delimiter=",")
    
    def makeTrimbitScanPlots(self, trimbit_dict, pin_mapping):
        """
        Plots measurement vs trimbit for each pin from a dictionary:
        trimbit_dict: {pin: [(trimbit, value), ...], ...}
        Returns a list of CSV filenames created.
        """
        csvfiles = []
        for module in self.enabled_modules():
            moduleName = module.getModuleName()
            for pin, name in pin_mapping.items():
                if pin in trimbit_dict:
                    data = np.array(trimbit_dict[pin], dtype=object)
                    if data.size == 0:
                        continue
                    trimbits, values = data[:, 0], data[:, 1]

                    # Filter out invalid data
                    trimbits = np.array(trimbits, dtype=float)
                    values = np.array(values, dtype=float)
                    print(f"Trimbits: {trimbits}")
                    print(f"Values: {values}")
                    valid_indices = ~np.isnan(values)
                    trimbits = trimbits[valid_indices]
                    values = values[valid_indices]

                    # Save to CSV
                    csvfilename = (
                        f"{self.output_dir}/TrimbitCurve_Module_{moduleName}_{name}.csv"
                    )
                    np.savetxt(
                        csvfilename,
                        np.column_stack([trimbits, values]),
                        delimiter=",",
                        header="Trimbit,Measurement",
                        comments="",
                    )
                    csvfiles.append(csvfilename)
        return csvfiles

    def IVCurveFinished(self, test: str, measure: dict):
        # Get the current timestamp
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        logger.debug("Inside IVCurveFinished")
        for process in self.run_processes:
            logger.debug("Sending command to end monitoring")
            process.write(b"\n")
            process.waitForBytesWritten()
            logger.debug("Command was sent.Waiting for process to finish.")
            process.waitForFinished(-1)
            logger.debug("Process finished")

        # 3/17/25 : Once HV distributor box arrives, functionality needs to be added for running
        # IVCurve on multiple modules. Once that happens, the loop under this comment can be edited
        # to output the results only to the console of the fc7 that each module is connnected to.
        #for console in self.runwindow.ConsoleViews:
        #    self.outputString.emit(f"Voltages: {measure['voltage']}", console)
        #    self.outputString.emit(f"Currents: {measure['current']}", console)

        # We should zip self.modules with the instrument cluster channels
        # to match the modules ids with the correct channels.
        channelList = []
        for channel in measure:
            channelList.append(channel)
        modules_for_mapping = (
            self.enabled_modules()
            if len(channelList) == len(self.enabled_modules())
            else self.modules
        )
        module_chan_map = dict(zip(modules_for_mapping, channelList))

        for module in self.enabled_modules():
            ogId = module.getOpticalGroup().getOpticalGroupID()
            beboardId = module.getOpticalGroup().getBeBoard().getBoardID()
            fc7name = module.getOpticalGroup().getBeBoard().getBoardName()
            moduleName = module.getModuleName()
            hybridId = module.getFMCPort()

            self.IVCurveResult = ScanCanvas(
                self,
                xlabel="Voltage (V)",
                ylabel="I (A)",
                X=measure[module_chan_map[module]]["voltage"],
                Y=measure[module_chan_map[module]]["current"],
                invert=True,
            )

            csvfilename = "{0}/IVCurve_Module_{1}_{2}.csv".format(
                os.path.join(self.output_dir, fc7name), moduleName, timestamp
            )
            # Some power supplies give outputs as a two dimensional array
            # This breaks np.savetxt. The second element of the array should be empty
            # either way, therefore, we will just flatten the array getting rid of the
            # second dimension. NOTE: If we do want measurements from multiple HV
            # channels this will need to reevaluated.

            # Convert to numpy array to give us access to flatten() and ndim
            # This should actually allow measure to be a nested dictionary where the 
            # keys are the instrument cluster channel names.
            voltages = np.array(measure[module_chan_map[module]]["voltage"])
            current = np.array(measure[module_chan_map[module]]["current"])

            # If the voltages are 2D+, then flatten.
            if voltages.ndim > 1:
                voltages = voltages.flatten()
                current = current.flatten()

            os.makedirs(os.path.dirname(csvfilename), exist_ok=True)
            np.savetxt(csvfilename, (voltages, current), delimiter=",")
            module_canvas_path = "Detector/Board_{boardID}/OpticalGroup_{ogID}/Hybrid_{hybridID}/".format(
                boardID=beboardId, ogID=ogId, hybridID=hybridId
            )

            IVCurve_CSV_to_ROOT(
                moduleName, module_canvas_path, csvfilename, os.path.join(self.output_dir,fc7name)
            )

            filename = "{0}/IVCurve_Module_{1}_{2}.svg".format(
                os.path.join(self.output_dir,fc7name), moduleName, timestamp

            )
            # filename2 = "IVCurve_Module_{0}_{1}.svg".format(moduleName, timestamp)
            self.IVCurveResult.saveToSVG(filename)
            # self.IVCurveResult.saveToSVG(filename2)

            self.figurelist[moduleName] = [filename]

        for i, firmware in enumerate(self.firmware):
            self.runwindow.ResultWidget.ProgressBars[i][
                self.testIndexTracker
            ].setValue(100)

        self.validateTest()

        step = "IVCurve"

        self.testIndexTracker += 1

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
        for channel in self.instruments._module_dict:
            for chip in self.INTERNAL_NTC_ABSup[channel]:
                if (
                    chip not in self.INTERNAL_NTC_ABSdown[channel]
                    or chip not in self.INTERNAL_NTC_ABSupError[channel]
                    or chip not in self.INTERNAL_NTC_ABSdownError[channel]
                ):
                    continue

                temperature_data = [
                    list(self.INTERNAL_NTC_ABSup[channel][chip].keys()),
                    list(self.INTERNAL_NTC_ABSup[channel][chip].values()),
                    list(self.INTERNAL_NTC_ABSupError[channel][chip].values()),
                    list(self.INTERNAL_NTC_ABSdown[channel][chip].keys()),
                    list(self.INTERNAL_NTC_ABSdown[channel][chip].values()),
                    list(self.INTERNAL_NTC_ABSdownError[channel][chip].values()),
                ]
                if not all(len(row) == len(temperature_data[0]) for row in temperature_data):
                    continue
                self.makeSLDOTemperatureCSV(temperature_data, str(chip), "GADC")
        for module in self.enabled_modules():
            ogId = module.getOpticalGroup().getOpticalGroupID()
            beboardId = module.getOpticalGroup().getBeBoard().getBoardID()
            fc7name = module.getOpticalGroup().getBeBoard().getBoardName()
            moduleName = module.getModuleName()
            hybridId = module.getFMCPort()
            module_canvas_path = (
                "Detector/Board_{boardID}/OpticalGroup_{ogID}/Hybrid_{hybridID}".format(
                    boardID=beboardId, ogID=ogId, hybridID=hybridId
                )
            )

            SLDO_CSV_to_ROOT2(
                moduleName, module_canvas_path, self.SLDOfilelist, os.path.join(self.output_dir,fc7name)
            )

        self.validateTest()
        self.testIndexTracker += 1

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
        for module in self.enabled_modules():
            ogId = module.getOpticalGroup().getOpticalGroupID()
            beboardId = module.getOpticalGroup().getBeBoard().getBoardID()
            fc7name = module.getOpticalGroup().getBeBoard().getBoardName()
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
                moduleName, module_canvas_path, csvfiles, os.path.join(self.output_dir, fc7name)
            )

        self.validateTest()
        self.testIndexTracker += 1

        EnableReRun = False
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
        # Set retrying flag immediately to prevent on_finish from running while dialog is shown
        self._retrying = True
        
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
                    # Disconnect signals before killing to prevent unwanted finished signal
                    try:
                        process.finished.disconnect()
                        process.readyReadStandardOutput.disconnect()
                    except Exception:
                        pass
                    process.kill()
                self.halt = True
                self.haltSignal.emit(self.halt)
                self.starttime = None

            for row in range(self.force_continue_window.table.rowCount()):
                module_name = self.force_continue_window.table.item(row, 0).text()
                enabled = (
                    self.force_continue_window.table.item(row, 1).checkState()
                    == Qt.Checked
                )
                self.statuses[module_name] = "1" if enabled else "0"

            event.accept()

        def handle_retry():
            if check_enabledModules():
                # Set retrying flag to prevent on_finish from executing
                self._retrying = True
                
                for i, process in enumerate(self.run_processes):
                    # Disconnect signals then kill processes
                    try:
                        process.finished.disconnect()
                        process.readyReadStandardOutput.disconnect()
                        logger.info(f"Disconnected signals from old process {i}")
                    except Exception as e:
                        logger.debug(f"Could not disconnect signals from process {i}: {e}")
                    
                    if process.state() == QProcess.Running:
                        process.kill()
                        process.waitForFinished(-1)
                    
                    # Create new QProcess objects
                    self.run_processes[i] = QProcess()
                    self.run_processes[i].readyReadStandardOutput.connect(
                        lambda j=i: self.on_readyReadStandardOutput(j)
                    )
                    self.run_processes[i].finished.connect(
                        lambda exitCode, exitStatus, j=i: self.finished_run_process(
                            exitCode, exitStatus, j
                        )
                    )
                
                # Reset finished_processes counter
                self.finished_processes = 0
                
                for console in self.runwindow.ConsoleViews:
                    self.outputString.emit(f"Retrying {self.currentTest}...",console)
                for i in range(len(self.firmware)):
                    self.runwindow.ResultWidget.runtimes[i][
                        self.testIndexTracker
                    ].setText("")  # may need to .update()
                    self.runwindow.ResultWidget.ProgressBars[i][
                        self.testIndexTracker
                    ].setValue(
                        0
                    )  # may need to .update(). Automatically adds "0%" text on Progress bar.
                # Don't decrement testIndexTracker - we're retrying at the same position
                self.force_continue_window.close()
                
                # Restart the test - _retrying will be cleared when the retry completes
                self.runTest()

        def handle_continue():
            if check_enabledModules():
                # Clear retrying flag to allow normal on_finish execution
                self._retrying = False
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
            for i, fc7 in enumerate(self.firmware):
                for module in fc7.getModules():
                    status, message = self.felis_instances[i].set_comment( name_module=module.getModuleName(),name_test="sequence", comment=self.comment)
                    status, message = self.felis_instances[i].upload_results(
                        name_module=module.getModuleName(),
                        username=self.master.username,
                        userpass=self.master.password,
                        type_sequence=self.info,
                        version_ph2acf=os.environ.get("PH2ACF_VERSION"),
                        version_testStationSoftware="OSU_GUI-" + os.environ.get(
                            "PH2_ACF_GUI_VERSION"
                        ),
                        version_innertrackertests=os.environ.get("INNER_TRACKER_TESTS_VERSION"),
                    )
                    if not status:
                        raise ConnectionError(message)

                    counter += 1
                    self.updateProgressBar.emit(
                        self.runwindow.UploadProgressBar,
                        100 * counter / len(self.modules),
                        f"{counter}/{len(self.modules)} uploaded",
                    )

        except ConnectionError:
            logger.error(traceback.format_exc())
            self.master.errorMessageBoxSignal.emit(traceback.format_exc())

        except Exception:
            error_message = (
                "Cannot upload test results, you are not signed in to Panthera."
            )
            if self.master.panthera_connected:
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

        for module in self.enabled_modules():
            ogId = module.getOpticalGroup().getOpticalGroupID()
            boardId = module.getOpticalGroup().getBeBoard().getBoardID()
            hybridId = module.getFMCPort()
            for chipID in module.getEnabledChips().keys():
                commands.append(
                    command_template.format(boardId, ogId, hybridId, chipID)
                )
        executeCommandSequence(commands)
    