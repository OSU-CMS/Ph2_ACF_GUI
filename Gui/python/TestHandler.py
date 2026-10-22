from PyQt5 import QtCore
from PyQt5.QtCore import pyqtSignal, QObject, QProcess, Qt, QThread, QTimer
from PyQt5.QtGui import QBrush, QColor, QFont
from PyQt5.QtWidgets import (
    QMessageBox,
    QTableWidget,
    QTableWidgetItem,
    QTreeWidget,
    QTreeWidgetItem,
    QHBoxLayout,
    QPlainTextEdit,
    QProgressBar,
    QDialog,
    QVBoxLayout,
    QPushButton,
    QLabel,
)

import os
import functools
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
from Gui.python.icicleCompat import any_output_on
from InnerTrackerTests.TestSequences import CompositeTests_Modules, Test_to_Ph2ACF_Map, OpenBumpTest
from Gui.siteSettings import icicle_instrument_setup, UI_testing, WorkingChannels
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


class _CallableThread(QThread):
    """Run a function on a worker thread, keeping any exception it raises in `error`
    for the GUI thread to report after `finished`."""

    def __init__(self, function):
        super().__init__()
        self._function = function
        self.error = None

    def run(self):
        try:
            self._function()
        except Exception as error:
            logger.error(traceback.format_exc())
            self.error = error


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
    setProgressBarValue = pyqtSignal(QProgressBar, int)  # for updates from worker threads
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

        # GADC SLDO data, keyed as self.<observable><up|down>[moduleName][chip][current]
        self.resetSLDOData()

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
        self.GADCWorker = None
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
        self.setProgressBarValue.connect(self._setProgressBarValue)
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
        # True from the moment a failure is reported until the user answers the dialog
        self._handling_failure = False
        # {fc7 process index: failure reason} for the failure being handled
        self._failed_boards = {}
        # Bumped whenever a failure stops the current run, so stale on_finish /
        # IVCurveFinished calls from that run stop instead of advancing the sequence
        self._test_generation = 0
        # Old QProcess objects kept alive after a stop, since the stop can happen
        # inside one of their own signal handlers
        self._retired_processes = []
        self._injection_delay_cal_edge_by_chip = {}

    def _module_is_enabled(self, module) -> bool:
        module_name = module.getModuleName()
        status = self.statuses.get(module_name, module.getEnabled())
        return str(status) == "1"

    def enabled_modules(self):
        return [module for module in self.modules if self._module_is_enabled(module)]

    def _set_communication_test_result(self, process_index, passed):
        module_names = []
        for module in self.firmware[process_index].getModules():
            if not self._module_is_enabled(module):
                continue
            module_name = module.getModuleName()
            self.communicationTestResults[module_name] = passed
            module_names.append(module_name)

        logger.info(
            "CommunicationTest result for %s modules %s: %s",
            self.firmware[process_index].getBoardName(),
            module_names,
            "pass" if passed else "fail",
        )

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

    def resetSLDOData(self):
        """Clear GADC SLDO measurements and the SLDO file list before a new SLDO scan."""
        for observable in ("VDDD", "VDDA", "VIND", "VINA", "INTERNAL_NTC_ABS"):
            for direction in ("up", "down"):
                setattr(self, observable + direction, {})
                setattr(self, observable + direction + "Error", {})
        self.SLDOfilelist = []

    def GADC_execute_each_step(
        self,
        upOrDown: str,
        total_steps: int,
        physics_seconds: int = site_settings.SLDOScan_GADC["physics seconds"],
    ) -> None:
        """Run one Ph2_ACF physics run per FC7 and store each chip's GADC readings
        under the module it belongs to, at that module's LV current."""
        if self.halt:
            return
        starting_current = site_settings.SLDOScan_GADC[
            self.master.module_in_use.split(" ")[-1].lower()
        ]["starting current"]

        # LV current of each module at this sweep step, read from its own LV channel
        module_dict = self.instruments._module_dict
        module_currents = {}
        for module, key in self._get_module_cluster_keys().items():
            if key is None or not self._module_is_enabled(module):
                continue
            lv = module_dict[key]["lv"]
            voltage = getattr(lv, "voltage")
            current = getattr(lv, "current")
            if current < starting_current:
                continue
            module_currents[module] = current
            logger.info(
                f"Beginning physics test for {module.getModuleName()} at {voltage}V and {current}A"
            )

        runs = []  # (process, fc7_index, output chunks)
        for fc7_index, beboard in enumerate(self.firmware):
            board_modules = [m for m in beboard.getModules() if m in module_currents]
            if not board_modules:
                continue
            for module in board_modules:
                self.outputString.emit(
                    f"Beginning physics test for {module.getModuleName()} at {module_currents[module]}A",
                    self.runwindow.ConsoleViews[fc7_index],
                )

            process = QProcess()
            process.setProcessChannelMode(QtCore.QProcess.MergedChannels)
            process.setWorkingDirectory(os.environ.get("PH2ACF_BASE_DIR") + "/test/")
            chunks = []
            # Bind this run's process/board explicitly; a bare lambda would see the
            # loop's last values by the time output arrives. DirectConnection keeps the
            # handler on the thread running this sweep: SLDOScan calls this from its
            # worker thread, and PyQt would otherwise queue a partial of a TestHandler
            # method to the GUI thread, racing with the parse below.
            process.readyReadStandardOutput.connect(
                functools.partial(self.on_readyReadStandardOutput_GADC, process, fc7_index, chunks),
                Qt.DirectConnection,
            )
            enable_temp = any(
                self.shouldStoreSLDOTemperature(module_currents[m]) for m in board_modules
            )
            self.setSLDOTemperatureMonitoringInXML(fc7_index, enable_temp)
            process.start(
                "CMSITminiDAQ",
                [
                    "-f",
                    f"CMSIT_{beboard.getBoardName()}_{self.currentTest}.xml",
                    "-c",
                    "physics",
                    "-t",
                    str(physics_seconds),
                ],
            )
            runs.append((process, fc7_index, chunks))

        for process, fc7_index, chunks in runs:
            board_name = self.firmware[fc7_index].getBoardName()
            # Poll instead of waiting indefinitely so a Stop can kill the run
            while process.state() != QProcess.NotRunning and not process.waitForFinished(500):
                if self.halt:
                    process.kill()
                    process.waitForFinished(3000)
                    break
                if process.state() == QProcess.NotRunning:
                    logger.error(
                        f"Ph2_ACF physics test on {board_name} didn't excute correctly."
                    )
            # Collect anything still buffered after the process exited
            self.on_readyReadStandardOutput_GADC(process, fc7_index, chunks)
            if self.halt:
                continue
            self.parseGADCOutput(
                "".join(chunks), self.firmware[fc7_index], module_currents, upOrDown
            )

        if self.currentTest == "SLDOScan_GADC":
            self.ProgressValue += 1
            # Signal rather than setValue: this runs on the SLDOScan_GADC worker thread
            for i in range(len(self.firmware)):
                self.setProgressBarValue.emit(
                    self.runwindow.ResultWidget.ProgressBars[i][self.testIndexTracker],
                    int(100 * self.ProgressValue / total_steps),
                )

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
            self.resetSLDOData()

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

            # The sweep takes minutes; run it off the GUI thread so the window
            # (terminal, Stop button) stays responsive. SLDOScanGADCFinished
            # continues on the GUI thread when it is done.
            self.GADCWorker = _CallableThread(
                lambda: self.runSLDOScanGADCSweep(SLDOScan_GADC_dict, total_steps)
            )
            self.GADCWorker.finished.connect(self.SLDOScanGADCFinished)
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("Beginning SLDOScan_GADC", console)
            self.GADCWorker.start()
            return

        if self.instruments:
            if not any_output_on(self.instruments, "lv"):
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
            self.resetSLDOData()
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
            # Checking the status of the HV supply
            if not any_output_on(self.instruments, "hv"):
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
            for module in self.enabled_modules():
                self.communicationTestResults[module.getModuleName()] = None
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
            self._stop_iv_curve()
        if self.SLDOScanHandler:
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("Aborting SLDOScan", console)
            self.SLDOScanHandler.stop()
        if self.GADCWorker is not None:
            # self.halt stops the sweep and kills the running physics run; the
            # worker then switches LV off
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("Aborting SLDOScan_GADC", console)
        if self.trimbitHandler:
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("Aborting TrimbitScan", console)
            self.trimbitHandler.stop()

    def _stop_iv_curve(self):
        """Stop a running IV curve: abort its thread, wait for it, then ramp HV down."""
        handler = self.IVCurveHandler
        if handler is None:
            return
        # Disconnect so a late IVCurveFinished from this run cannot fire
        for signal in (handler.finished, handler.progressSignal, handler.startSignal):
            try:
                signal.disconnect()
            except TypeError:
                pass
        handler.stop()
        self.IVCurveHandler = None

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

    def _modules_in_entry_order(self):
        """Return self.modules in the order they were entered in the GUI.

        self.modules is grouped by board/FMC/port, which can differ from entry order.
        Falls back to self.modules when entry indices are unavailable (e.g. simple mode).
        """
        if all(isinstance(m.getEntryIndex(), int) for m in self.modules):
            return sorted(self.modules, key=lambda m: m.getEntryIndex())
        return list(self.modules)

    def _get_module_cluster_keys(self):
        """Map each module to its instrument cluster key (e.g. '0', '3').

        The i-th module entered in the GUI corresponds to the i-th remaining cluster
        entry (the same pairing used to trim the cluster in
        QtStartWindow._get_used_channels). Modules beyond the cluster size map to None.
        """
        try:
            module_dict = getattr(self.instruments, '_module_dict', {}) or {}
            cluster_keys = sorted(module_dict.keys(), key=lambda k: int(k))
        except Exception:
            cluster_keys = []
        return {
            module: (cluster_keys[i] if i < len(cluster_keys) else None)
            for i, module in enumerate(self._modules_in_entry_order())
        }

    def _get_tec_module_map(self, n_tecs: int = 8):
        """Map each TEC channel (1-based, as a string key) to the module ID in that slot.

        The TEC number is taken from the 'cb' channel of each module's instrument
        cluster entry; if the cluster has no coldbox channel, fall back to the order
        of WorkingChannels in siteConfig. Unoccupied TECs map to None.
        """
        tec_map = {str(ch): None for ch in range(1, n_tecs + 1)}
        module_dict = getattr(self.instruments, '_module_dict', {}) or {}
        cluster_keys = self._get_module_cluster_keys()

        for i, module in enumerate(self._modules_in_entry_order()):
            tec = None
            key = cluster_keys.get(module)
            if key is not None:
                cb_channel = getattr(module_dict[key].get("cb"), "channel", None)
                if isinstance(cb_channel, int):
                    tec = cb_channel
            if tec is None and i < len(WorkingChannels):
                tec = WorkingChannels[i]
            if tec is not None:
                tec_map[str(tec)] = module.getModuleName()
        return tec_map

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
                try:
                    n_tecs = len(temps) if isinstance(temps, list) and temps else 8
                    payload["tec_module_map"] = self._get_tec_module_map(n_tecs)
                except Exception:
                    logger.warning("Could not build TEC-to-module map for Tessie snapshot")
                    payload["tec_module_map"] = None

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

    def saveTest(self, processIndex: int, process: QProcess, report_failure=True):
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
            if report_failure and self.currentTest != "CommunicationTest":
                self._report_failure(
                    processIndex, "Could not save the test output files"
                )

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

        if self.currentTest == "CommunicationTest":
            if (
                "Error, some data lanes are enabled but inactive, reached maximum number of attempts"
                in alltext
            ):
                self._set_communication_test_result(processIndex, False)
                self._report_failure(
                    processIndex, "Data lanes enabled but inactive"
                )
            elif "All enabled data lanes are active" in alltext:
                self._set_communication_test_result(processIndex, True)

        else:
            if "FIFO empty" in alltext:
                self._report_failure(processIndex, "FIFO empty")
            elif "Reached maximum number of attempts" in alltext:
                self._report_failure(
                    processIndex, "Reached maximum number of attempts"
                )

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
                            if (
                                "ThresholdAdjustment_default_to" in self.currentTest
                                and updatedFEKey.startswith("DAC_GDAC_")
                            ):
                                continue
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
    
    def on_readyReadStandardOutput_GADC(self, process: QProcess, fc7_index: int, chunks: list):
        """Stream a GADC physics run's output to its FC7's output.txt and console.
        The text is also kept in chunks so it can be parsed once the run is complete."""
        alltext = process.readAllStandardOutput().data().decode(errors="replace")
        if not alltext:
            return
        chunks.append(alltext)

        try:
            fc7name = self.firmware[fc7_index].getBoardName()
            outpath = os.path.join(self.output_dir, fc7name, "output.txt")
            with open(outpath, "a") as outputfile:
                outputfile.write(alltext)
        except Exception:
            logger.error("Failed writing GADC output to per-FC7 file")
            logger.error(traceback.format_exc())

        for textStr in alltext.split("\n"):
            _, text = parseANSI(textStr.encode("ascii", errors="ignore"))
            self.outputString.emit(
                text.decode("utf-8"), self.runwindow.ConsoleViews[fc7_index]
            )

    def parseGADCOutput(self, alltext, beboard, module_currents, upOrDown):
        """Store GADC readings from one FC7's physics run under the module each chip belongs to.

        Ph2_ACF prints "data for [board/opticalGroup/hybrid/chip = b/o/h/c]" before each
        chip's readings; the XML uses the optical group ID and the module's FMC port as
        the hybrid ID, so (opticalGroup, hybrid) identifies the module on this board.
        Modules not in module_currents (disabled, or below the starting current) are skipped.
        """
        hybrid_to_module = {
            (str(m.getOpticalGroup().getOpticalGroupID()), str(m.getFMCPort())): m
            for m in beboard.getModules()
        }
        module = None
        chip = None
        unknown_hybrids = set()

        for textStr in alltext.split("\n"):
            textStr = re.compile(r"\x1B[@-_][0-?]*[ -/]*[@-~]").sub("", textStr)
            match = re.search(
                r"data for \[board/opticalGroup/hybrid/chip = (\d+)/(\d+)/(\d+)/(\d+)\]",
                textStr,
            )
            if match:
                chip = match.group(4)
                module = hybrid_to_module.get((match.group(2), match.group(3)))
                if module is None:
                    unknown_hybrids.add((match.group(2), match.group(3)))
                continue

            match = re.search(
                r"(\w+):\s*([-+]?\d*\.?\d+)\s*\+/-\s*([-+]?\d*\.?\d+)\s*([A-Za-z]+)",
                textStr,
            )
            if not match:
                continue
            observable = match.group(1)
            if observable not in ("VDDD", "VDDA", "VINA", "VIND", "INTERNAL_NTC_ABS"):
                continue
            if chip is None:
                logger.error(
                    f'Did not receive expected message, "Reading monitored data for '
                    f'[board/opticalGroup/hybrid/chip = ...]", before measurement message "{match.group(0)}"'
                )
                continue
            if module not in module_currents:
                continue

            current = module_currents[module]
            if observable == "INTERNAL_NTC_ABS":
                if not self.shouldStoreSLDOTemperature(current):
                    continue
                multiplier = 1
            else:
                multiplier = site_settings.SLDOScan_GADC["multipliers"][observable[:3]]

            moduleName = module.getModuleName()
            # One value per chip per sweep step, keyed by this module's LV current
            for attr, value in (
                (observable + upOrDown, float(match.group(2)) * multiplier),
                (observable + upOrDown + "Error", float(match.group(3)) * multiplier),
            ):
                getattr(self, attr).setdefault(moduleName, {}).setdefault(chip, {})[current] = value

        if unknown_hybrids:
            logger.warning(
                f"GADC output on {beboard.getBoardName()} had readings for (opticalGroup, hybrid) "
                f"{sorted(unknown_hybrids)} that match no module on this board; they were ignored."
            )

    @QtCore.pyqtSlot()
    def on_finish(self, processIndex: int):
        # While the process is killed:
        # Wait for all processes to finish so FC7s don't get out of sync

        logger.debug("All processes finished")

        # A failure stops every FC7 and the failure dialog decides what happens next,
        # so never advance the sequence from here while that is in progress.
        if self._handling_failure:
            logger.info(
                f"Failure being handled - ignoring on_finish for process {processIndex}"
            )
            return

        generation = self._test_generation

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
            # IVCurveFinished handles the rest of the IV curve; it also checks
            # whether saveTest reported a failure.
            return

        # Save the output ROOT file to output_dir
        logger.debug("About to run saveTest()")
        time.sleep(1)
        self.saveTest(processIndex, self.run_processes[processIndex])

        # saveTest may have reported a failure, which stops this run
        if self.halt or generation != self._test_generation:
            logger.info("Run was stopped during saveTest - aborting on_finish")
            return

        self.saveConfigs(process_index=processIndex)
        if self.currentTest in INJECTION_DELAY_SOURCE_TESTS:
            self._cache_injection_delay_cal_edges_from_output_dir(self.output_dir)

        # Don't continue on sequence until all processes have finished the current test
        self.finished_processes += 1
        expected_processes = (
            self.active_process_count
            if self.active_process_count > 0
            else len(self.run_processes)
        )
        if not self.finished_processes == expected_processes:
            return
        self.finished_processes = 0

        self._complete_current_test()

    def _complete_current_test(self):
        """Grade the current test and move on: the next OpenBumpTest subtest, or the next test."""
        if self._openBumpTest_running:
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
    def makeSLDOPlot(self, total_result: np.ndarray, pin: str, method: str, modules=None):
        """Save an SLDO curve for each of modules (default: every enabled module)."""
        for module in (modules if modules is not None else self.enabled_modules()):
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

    def makeSLDOTemperatureCSV(self, total_result: np.ndarray, chip: str, method: str, modules=None):
        """Save chip temperatures for each of modules (default: every enabled module)."""
        for module in (modules if modules is not None else self.enabled_modules()):
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
        generation = self._test_generation
        for process in self.run_processes:
            logger.debug("Sending command to end monitoring")
            process.write(b"\n")
            process.waitForBytesWritten()
            logger.debug("Command was sent.Waiting for process to finish.")
            process.waitForFinished(-1)
            logger.debug("Process finished")

        # on_finish -> saveTest may have reported a failure while the monitoring
        # processes were finishing; the failure dialog decides what happens next.
        if self._handling_failure or generation != self._test_generation:
            logger.info("IV Curve run was stopped by a failure - not advancing")
            return

        # 3/17/25 : Once HV distributor box arrives, functionality needs to be added for running
        # IVCurve on multiple modules. Once that happens, the loop under this comment can be edited
        # to output the results only to the console of the fc7 that each module is connnected to.
        #for console in self.runwindow.ConsoleViews:
        #    self.outputString.emit(f"Voltages: {measure['voltage']}", console)
        #    self.outputString.emit(f"Currents: {measure['current']}", console)

        # measure is keyed by instrument cluster key, so look up each module's key
        # (assigned in GUI entry order, not the board/FMC/port order of self.modules).
        module_chan_map = self._get_module_cluster_keys()
        if not all(module_chan_map.get(m) in measure for m in self.enabled_modules()):
            channelList = list(measure)
            entry_ordered = self._modules_in_entry_order()
            enabled_entry_ordered = [m for m in entry_ordered if self._module_is_enabled(m)]
            modules_for_mapping = (
                enabled_entry_ordered
                if len(channelList) == len(enabled_entry_ordered)
                else entry_ordered
            )
            module_chan_map = dict(zip(modules_for_mapping, channelList))
            logger.warning(
                f"IV curve channels {channelList} do not match instrument cluster keys; "
                "pairing modules with channels by entry order instead."
            )

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

    def makeGADCSLDOPlots(self):
        """Write each module's GADC SLDO curves and chip temperatures from its own data."""
        for module in self.enabled_modules():
            moduleName = module.getModuleName()

            for datatype in ("VDDD", "VDDA"):
                vin = f"VIN{datatype[-1]}"
                for chip in getattr(self, f"{datatype}up").get(moduleName, {}):
                    rows = []
                    for direction in ("up", "down"):
                        vdd_values = getattr(self, datatype + direction).get(moduleName, {}).get(chip, {})
                        vin_values = getattr(self, vin + direction).get(moduleName, {}).get(chip, {})
                        currents = [c for c in vdd_values if c in vin_values]
                        rows += [
                            currents,
                            [float(vin_values[c]) for c in currents],
                            [float(vdd_values[c]) for c in currents],
                        ]
                    if not rows[0] or len(rows[0]) != len(rows[3]):
                        logger.error(
                            f"Skipping GADC {datatype} plot for {moduleName} ROC{chip}: "
                            f"{len(rows[0])} up and {len(rows[3])} down sweep points."
                        )
                        continue
                    self.makeSLDOPlot(rows, f"{datatype}_ROC{int(chip)}", "GADC", modules=[module])

            for chip in self.INTERNAL_NTC_ABSup.get(moduleName, {}):
                temperature_data = []
                for direction in ("up", "down"):
                    values = getattr(self, "INTERNAL_NTC_ABS" + direction).get(moduleName, {}).get(chip, {})
                    errors = getattr(self, "INTERNAL_NTC_ABS" + direction + "Error").get(moduleName, {}).get(chip, {})
                    temperature_data += [list(values.keys()), list(values.values()), list(errors.values())]
                if not temperature_data[0] or not all(
                    len(row) == len(temperature_data[0]) for row in temperature_data
                ):
                    continue
                self.makeSLDOTemperatureCSV(temperature_data, str(chip), "GADC", modules=[module])

    def _setProgressBarValue(self, bar: QProgressBar, value: int):
        bar.setValue(value)

    def runSLDOScanGADCSweep(self, SLDOScan_GADC_dict, total_steps):
        """Body of SLDOScan_GADC; runs on self.GADCWorker. Stops early when self.halt is set."""
        try:
            starting_voltages = [
                np.abs(getattr(module["hv"], "voltage"))
                for module in self.instruments._module_dict.values()
            ]
            self.instruments.hv_off(
                execute_each_step=lambda: self.ramp_progress_bar(starting_voltages)
            )
            self.instruments.lv_on(
                voltage=SLDOScan_GADC_dict["voltage"],
                current=SLDOScan_GADC_dict["starting current"],
            )
            for direction, target in (
                ("up", SLDOScan_GADC_dict["target current"]),
                ("down", SLDOScan_GADC_dict["starting current"]),
            ):
                if self.halt:
                    break
                self.instruments.lv_sweep(
                    target=target,
                    delay=0.1,
                    set_property="current",
                    measure=True,
                    step_size=SLDOScan_GADC_dict["step size"],
                    execute_each_step=functools.partial(
                        self.GADC_execute_each_step, direction, total_steps
                    ),
                    break_loop=lambda: self.halt,
                )
        finally:
            try:
                self.instruments.lv_off()
            except Exception:
                logger.error(traceback.format_exc())

    def SLDOScanGADCFinished(self):
        """Continue SLDOScan_GADC on the GUI thread once its sweep worker is done."""
        worker, self.GADCWorker = self.GADCWorker, None
        if worker is not None:
            worker.wait()  # finished can arrive before run() has fully returned
        if worker is not None and worker.error is not None:
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit(f"SLDOScan_GADC failed: {worker.error!r}", console)
            if not self.halt:
                self.halt = True
                self.haltSignal.emit(self.halt)
            return
        if self.halt:
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("SLDOScan_GADC stopped", console)
            return

        for i in range(len(self.firmware)):
            self.runwindow.ResultWidget.ProgressBars[i][self.testIndexTracker].setValue(100)
        # GADC plots are made per module in SLDOScanFinished
        self.SLDOScanFinished()

    def SLDOScanFinished(self):
        self.makeGADCSLDOPlots()
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

            # Only this module's CSVs; SLDOfilelist holds every module's files
            module_files = list(dict.fromkeys(
                f for f in self.SLDOfilelist
                if f"_Module_{moduleName}_" in os.path.basename(f)
            ))
            SLDO_CSV_to_ROOT2(
                moduleName, module_canvas_path, module_files, os.path.join(self.output_dir,fc7name)
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

    def _report_failure(self, processIndex, reason):
        """A test failed on one FC7: stop every FC7 and ask the user what to do."""
        board_name = self.firmware[processIndex].getBoardName()
        logger.error(f"{self.currentTest} failed on {board_name}: {reason}")

        if self._handling_failure:
            # Already stopping this run for an earlier failure; just add it to the dialog
            self._failed_boards.setdefault(processIndex, reason)
            return

        self._handling_failure = True
        self._failed_boards = {processIndex: reason}
        self._test_generation += 1

        for console in self.runwindow.ConsoleViews:
            self.outputString.emit(
                f"{self.currentTest} failed on {board_name} ({reason}). Stopping all FC7s.",
                console,
            )
        self._stop_all_fc7s()

        # Show the dialog from the event loop rather than from inside the
        # output/finish handler that detected the failure
        QTimer.singleShot(0, self._show_failure_dialog)

    def _stop_all_fc7s(self):
        """Kill every FC7 process (and a running IV curve) and give each FC7 a fresh QProcess."""
        # IVCurveHandler is kept after the IV curve finishes, so only stop it
        # (which ramps HV down) when the IV curve is the test that failed
        if "IVCurve" in self.currentTest:
            self._stop_iv_curve()

        old_processes = self.run_processes
        self.run_processes = []
        for i, process in enumerate(old_processes):
            # Disconnect signals before killing to prevent unwanted finished signal
            try:
                process.finished.disconnect()
                process.readyReadStandardOutput.disconnect()
            except Exception:
                pass
            if process.state() != QProcess.NotRunning:
                process.kill()
                process.waitForFinished(3000)

            new_process = QProcess()
            new_process.readyReadStandardOutput.connect(
                lambda j=i: self.on_readyReadStandardOutput(j)
            )
            new_process.finished.connect(
                lambda exitCode, exitStatus, j=i: self.finished_run_process(
                    exitCode, exitStatus, j
                )
            )
            self.run_processes.append(new_process)

        # The stop can run inside a signal handler of one of the old processes,
        # so keep them alive until the next stop instead of letting Python delete them now
        self._retired_processes = old_processes
        self.finished_processes = 0

    def _show_failure_dialog(self):
        """One dialog for all FC7s, showing which FC7 failed and which modules each FC7 has."""
        if self.halt:
            # The run was aborted before the dialog could be shown
            self._handling_failure = False
            self._failed_boards = {}
            return

        failed = dict(self._failed_boards)
        test_name = self.currentTest

        dialog = QDialog()
        dialog.setWindowTitle(f"{test_name}: Failure Detected")
        dialog.setMinimumWidth(450)
        layout = QVBoxLayout(dialog)

        failed_names = ", ".join(
            self.firmware[i].getBoardName() for i in sorted(failed)
        )
        summary = QLabel(
            f"<b>{test_name}</b> failed on <b>{failed_names}</b>.<br>"
            "All FC7s have been stopped."
        )
        summary.setWordWrap(True)
        layout.addWidget(summary)

        # FC7s are top-level rows with their modules underneath; failed FC7s are highlighted
        tree = QTreeWidget()
        tree.setColumnCount(2)
        tree.setHeaderLabels(["FC7 / Module", "Status"])
        bold = QFont()
        bold.setBold(True)
        failed_background = QBrush(QColor("#f8d7da"))
        failed_text = QBrush(QColor("#842029"))

        board_module_items = []  # [(board, [(module, item), ...]), ...]
        for i, board in enumerate(self.firmware):
            status = f"FAILED: {failed[i]}" if i in failed else "Stopped (no error)"
            board_item = QTreeWidgetItem(tree, [f"FC7: {board.getBoardName()}", status])
            board_item.setFlags(
                board_item.flags() | Qt.ItemIsUserCheckable | Qt.ItemIsAutoTristate
            )
            board_item.setCheckState(0, Qt.Checked)
            for column in range(2):
                board_item.setFont(column, bold)
                if i in failed:
                    board_item.setBackground(column, failed_background)
                    board_item.setForeground(column, failed_text)

            module_items = []
            for module in board.getModules():
                module_item = QTreeWidgetItem(
                    board_item,
                    [module.getModuleName(), f"FMC port {module.getFMCPort()}"],
                )
                module_item.setFlags(module_item.flags() | Qt.ItemIsUserCheckable)
                module_item.setCheckState(
                    0, Qt.Checked if self._module_is_enabled(module) else Qt.Unchecked
                )
                module_items.append((module, module_item))
            board_module_items.append((board, module_items))

        tree.expandAll()
        tree.resizeColumnToContents(0)
        # Tall enough to show every FC7 and module without scrolling (up to a limit)
        row_count = sum(1 + len(module_items) for _, module_items in board_module_items)
        tree.setMinimumHeight(
            min(
                500,
                tree.header().sizeHint().height()
                + row_count * tree.sizeHintForRow(0)
                + 2 * tree.frameWidth(),
            )
        )
        layout.addWidget(tree)

        help_text = QLabel(
            "Uncheck any modules to leave out, then choose:<br>"
            f"<b>Retry</b>: run {test_name} again on all FC7s.<br>"
            f"<b>Continue</b>: keep the {test_name} results as they are and move on.<br>"
            "<b>Exit</b>: stop the test sequence."
        )
        help_text.setWordWrap(True)
        layout.addWidget(help_text)

        error_label = QLabel()
        error_label.setStyleSheet("color: red;")
        error_label.setWordWrap(True)
        error_label.hide()
        layout.addWidget(error_label)

        button_row = QHBoxLayout()
        exit_button = QPushButton("Exit")
        retry_button = QPushButton("Retry")
        continue_button = QPushButton("Continue")
        for button in (exit_button, retry_button, continue_button):
            button.setAutoDefault(False)
            button_row.addWidget(button)
        layout.addLayout(button_row)

        def apply_module_selection():
            # Every FC7 still runs a process, so each one needs an enabled module
            empty = [
                board.getBoardName()
                for board, module_items in board_module_items
                if not any(item.checkState(0) == Qt.Checked for _, item in module_items)
            ]
            if empty:
                error_label.setText(
                    "Each FC7 needs at least one enabled module: " + ", ".join(empty)
                )
                error_label.show()
                return False
            for _, module_items in board_module_items:
                for module, item in module_items:
                    module.setEnabled("1" if item.checkState(0) == Qt.Checked else "0")
            return True

        dialog.choice = "exit"  # Closing the window counts as Exit

        def choose(choice):
            if choice != "exit" and not apply_module_selection():
                return
            dialog.choice = choice
            dialog.accept()

        exit_button.clicked.connect(lambda: choose("exit"))
        retry_button.clicked.connect(lambda: choose("retry"))
        continue_button.clicked.connect(lambda: choose("continue"))

        dialog.exec_()  # Blocks until the user chooses

        for _, module_items in board_module_items:
            for module, item in module_items:
                self.statuses[module.getModuleName()] = (
                    "1" if item.checkState(0) == Qt.Checked else "0"
                )

        self._handling_failure = False
        self._failed_boards = {}

        if dialog.choice == "retry":
            self._retry_current_test()
        elif dialog.choice == "continue":
            self._continue_after_failure()
        else:
            self.halt = True
            self.haltSignal.emit(self.halt)
            self.starttime = None

    def _retry_current_test(self):
        for console in self.runwindow.ConsoleViews:
            self.outputString.emit(f"Retrying {self.currentTest}...", console)
        for i in range(len(self.firmware)):
            self.runwindow.ResultWidget.runtimes[i][
                self.testIndexTracker
            ].setText("")  # may need to .update()
            self.runwindow.ResultWidget.ProgressBars[i][
                self.testIndexTracker
            ].setValue(
                0
            )  # may need to .update(). Automatically adds "0%" text on Progress bar.
        # Don't change testIndexTracker - we're retrying at the same position
        self.runTest()

    def _continue_after_failure(self):
        for console in self.runwindow.ConsoleViews:
            self.outputString.emit(
                f"Continuing after {self.currentTest} failure...", console
            )
        # Save whatever output the stopped processes left behind, without
        # reporting another failure if some of it is missing
        for i, process in enumerate(self.run_processes):
            self.saveTest(i, process, report_failure=False)
        self._complete_current_test()

    def upload_to_Panthera(self):
        # Only modules that felis has results for can be uploaded. A module disabled
        # before its first graded test (e.g. in the CommunicationTest failure dialog)
        # was never added to felis, and upload_results would raise a KeyError for it.
        modules_to_upload = []
        for i, fc7 in enumerate(self.firmware):
            for module in fc7.getModules():
                module_name = module.getModuleName()
                if module_name in self.felis_instances[i].dict_modules:
                    modules_to_upload.append((i, module_name))
                else:
                    logger.info(
                        f"Skipping Panthera upload for {module_name}: no test results"
                    )
        total = len(modules_to_upload)

        self.updateProgressBar.emit(
            self.runwindow.UploadProgressBar, 0, f"0/{total} uploaded"
        )
        self.runwindow.UploadButton.setDisabled(True)

        counter = 0
        failures = []  # [(module_name, reason), ...]
        # Upload each module separately so one failure doesn't stop the rest
        for i, module_name in modules_to_upload:
            try:
                self.felis_instances[i].set_comment(
                    name_module=module_name, name_test="sequence", comment=self.comment
                )
                status, message = self.felis_instances[i].upload_results(
                    name_module=module_name,
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
                    raise ConnectionError(
                        message or "Panthera rejected the upload, see the log for details"
                    )
            except Exception as err:
                logger.error(f"Failed to upload {module_name} to Panthera")
                logger.error(traceback.format_exc())
                failures.append((module_name, str(err) or type(err).__name__))
                if not self.master.panthera_connected:
                    break  # Every module would fail the same way
                continue

            counter += 1
            self.updateProgressBar.emit(
                self.runwindow.UploadProgressBar,
                100 * counter / total,
                f"{counter}/{total} uploaded",
            )

        if not failures:
            return

        if not self.master.panthera_connected:
            error_message = (
                "Cannot upload test results, you are not signed in to Panthera."
            )
        else:
            error_message = (
                f"Failed to upload {len(failures)} of {total} modules to Panthera:\n"
                + "\n".join(f"{name}: {reason}" for name, reason in failures)
            )
            self.runwindow.UploadButton.setDisabled(False)  # allow a manual retry

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
    
