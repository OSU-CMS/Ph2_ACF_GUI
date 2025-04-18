import traceback
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
from InnerTrackerTests.Analysis.SLDO_CSV_to_ROOT import SLDO_CSV_to_ROOT


from Gui.QtGUIutils.QtMatplotlibUtils import ScanCanvas

from Gui.python.TestValidator import ResultGrader
from Gui.python.ANSIColoringParser import parseANSI
from Gui.python.IVCurveHandler import IVCurveHandler
from Gui.python.SLDOScanHandler import SLDOCurveHandler
import Gui.siteSettings as site_settings
from Gui.python.logging_config import logger
from InnerTrackerTests.TestSequences import CompositeTests, Test_to_Ph2ACF_Map


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

    def __init__(self, runwindow, master, info, firmware):
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

        self.finished_tests = []
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
        self.Ph2_ACF_ver = os.environ.get("Ph2_ACF_VERSION")
        print("Using version {0} of Ph2_ACF".format(self.Ph2_ACF_ver))
        self.firmwareImage = firmware_image[self.ModuleType][self.Ph2_ACF_ver]
        print("Firmware version is {0}".format(self.firmwareImage))
        self.RunNumber = "-1"
        self.isTDACtuned = False

        self.IVCurveHandler = None
        self.SLDOScanHandler = None

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

        felisScratchDir = "/home/cmsTkUser/Ph2_ACF_GUI/data/scratch"
        if not os.path.isdir(felisScratchDir):
            try:
                os.makedirs(felisScratchDir)
                logger.info("New Felis scratch directory created.")
            except OSError as e:
                logger.error(f"Error making Felis scratch directory: {e.strerror}")

        self.felis = Felis("/home/cmsTkUser/Ph2_ACF_GUI/data/scratch", False)
        self.grades = []

        self.figurelist = {}

        self.run_processes = [QProcess() for _ in self.firmware]
        for i, process in enumerate(self.run_processes):
            process.readyReadStandardOutput.connect(
                lambda j=i: self.on_readyReadStandardOutput(j)
            )
            process.finished.connect(lambda exitCode, exitStatus, j=i: self.finished_run_process(exitCode, exitStatus, j))

        self.readingOutput = False
        self.ProgressingMode = "None"
        self.ProgressValue = 0
        self.IVProgressValue = 0
        self.SLDOProgressValue = 0
        self.runtimeList = []
        self.starttime = None

        self.info_processes = [QProcess() for _ in self.firmware]
        for i, process in enumerate(self.info_processes):
            process.readyReadStandardOutput.connect(
                lambda j=i: self.on_readyReadStandardOutput_info(j)
            )

        ##---Adding firmware setting-----
        self.fw_processes =[QProcess() for _ in self.firmware]
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

    def finished_run_process(self, _, exitStatus, i):
        if exitStatus == QProcess.NormalExit:
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

    def configTest(self):
        # Gets the run number by reading from the RunNumber.txt file.
        try:
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
            testName = CompositeTests[self.info][0]
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
        if self.input_dir == "":
            # Copies file given in rd53[key] to test directory in Ph2_ACF test area as CMSIT_RD53.txt and the output dir.
            SetupRD53ConfigfromFile(self.rd53_file, self.output_dir)
        else:
            SetupRD53Config(self.input_dir, self.output_dir, self.rd53_file)

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
                    config_file = GenerateXMLConfig(firmware, self.currentTest, tmpDir)

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
                    config_file = GenerateXMLConfig(firmware, self.currentTest, tmpDir)

                    if config_file:
                        SetupXMLConfigfromFile(
                            config_file, self.output_dir, firmware.getBoardName()
                        )
                    else:
                        logger.warning("No Valid XML configuration file")

        self.initializeRD53Dict()
        self.config_file = ""
        return

    def saveConfigs(self):
        for key in self.rd53_file.keys():
            try:
                os.system(
                    "cp {0}/test/CMSIT_RD53_{1}.txt {2}/CMSIT_RD53_{1}_OUT.txt".format(
                        os.environ.get("PH2ACF_BASE_DIR"), key, self.output_dir
                    )
                )
            except OSError:
                print(
                    "Failed to copy {0}/test/CMSIT_RD53_{1}.txt {2}/CMSIT_RD53_{1}_OUT.txt".format(
                        os.environ.get("PH2ACF_BASE_DIR"), key, self.output_dir
                    )
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
        if self.halt:
            return
        runTestList = CompositeTests[self.info]

        if self.testIndexTracker == len(CompositeTests[self.info]):
            self.testIndexTracker = 0
            return
        testName = runTestList[self.testIndexTracker]
        if self.testIndexTracker + 1 < len(runTestList):  # Check if there is a next test
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

    def runSingleTest(self, testName, nextTest = None):
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
                self.runwindow.ResultWidget.ProgressBars[i][self.testIndexTracker].setValue(100)
            return

        print("Executing Single Step test...")
        for console in self.runwindow.ConsoleViews:
            self.outputString.emit("Executing Single Step test...", console)

        if self.instruments:
            lv_on = False
            for number in self.instruments.get_modules().keys():
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
            self.SLDOfilelist = []
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
        if self.instruments:
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
                            execute_each_step=lambda: self.ramp_progress_bar(starting_voltages)
                        )
                        self.instruments.hv_on_module(
                            module=mod_dict[number],
                            voltage=site_settings.forward_bias_voltage,
                            delay=0.3,
                            step_size=10,
                            execute_each_step=lambda: self.ramp_progress_bar(
                            [site_settings.forward_bias_voltage] * len(self.instruments._module_dict.values())
                            )
                        )
                    else:
                        self.instruments.hv_on_module(
                            module=mod_dict[number],
                            voltage=site_settings.forward_bias_voltage,
                            delay=0.3,
                            step_size=10,
                            execute_each_step=lambda: self.ramp_progress_bar(
                            [site_settings.forward_bias_voltage] * len(self.instruments._module_dict.values())
                            )
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
                            [default_hv_voltage] * len(self.instruments._module_dict.values())
                            )
                    )

        self.tempHistory = [0.0] * self.numChips
        self.tempindex = 0

        self.starttime = None
        self.ProgressingMode = "None"
        self.currentTest = testName

        self.updateOptimizedXMLValues()
        self.configTest()

        self.outputFile = self.output_dir + "/output.txt"
        self.errorFile = self.output_dir + "/error.txt"

        # Make sure that the GUI is not trying to write to the root directory
        try:
            assert self.output_dir != ""
        except AssertionError:
            logger.exception(
                "Output directory was not formatted correctly, closing GUI to not write to root directory."
            )
            raise

        if os.path.exists(self.outputFile):
            self.outputfile = open(self.outputFile, "a")
        else:
            self.outputfile = open(self.outputFile, "w")
        self.setupQProcess()

    def setupQProcess(self):
        self.tempHistory = [0.0] * self.numChips
        self.tempindex = 0
        self.outputFile = self.output_dir + "/output.txt"
        self.errorFile = self.output_dir + "/error.txt"
        if os.path.exists(self.outputFile):
            self.outputfile = open(self.outputFile, "a")
        else:
            self.outputfile = open(self.outputFile, "w")

        for process in self.info_processes:
            process.setProcessChannelMode(QtCore.QProcess.MergedChannels)
            process.setWorkingDirectory(os.environ.get("PH2ACF_BASE_DIR") + "/test/")

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
                        "Running COMMAND: CMSITminiDAQ  -f  CMSIT_{0}.xml  -c  {1}".format(
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
            process.setWorkingDirectory(
                os.environ.get("PH2ACF_BASE_DIR") + "/test/"
            )

        if self.currentTest == "CommunicationTest":
            for process, firmware in zip(self.run_processes, self.firmware):
                process.start(
                    "CMSITminiDAQ",
                    ["-f", f"CMSIT_{firmware.getBoardName()}.xml", "-p"],
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

    def abortTest(self):
        self.halt = True
        for process in self.run_processes:
            process.kill()

        self.haltSignal.emit(self.halt)

        self.starttime = None
        if self.IVCurveHandler:
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("Aborting IVCurve", console)
            self.IVCurveHandler.stop()
        if self.SLDOScanHandler:
            for console in self.runwindow.ConsoleViews:
                self.outputString.emit("Aborting SLDOScan", console)
            self.SLDOScanHandler.stop()

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
                        )

                        results.append(result)
                        passed.append(list(result.values())[0][0])

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
            os.system(f"cp {latest_file} {output_dir}/")

    def saveTest(self, processIndex: int):
        if self.run_processes[processIndex].state() == QProcess.Running:
            QMessageBox.critical(self, "Error", "Process not finished", QMessageBox.Ok)
            return

        try:
            if self.RunNumber == "-1":
                os.system(
                    "cp {0}/test/Results/Run000000*.root {1}/".format(
                        os.environ.get("PH2ACF_BASE_DIR"), self.output_dir
                    )
                )

            elif "IVCurve" in self.currentTest:
                os.system(
                    "cp {0}/test/Results/Run{1}_MonitorDQM.root {2}/".format(
                        os.environ.get("PH2ACF_BASE_DIR"),
                        self.RunNumber,
                        self.output_dir,
                    )
                )
            else:
                self.copyMostRecentRootFile(
                    self.RunNumber,
                    os.environ.get("PH2ACF_BASE_DIR") + "/test/Results",
                    self.output_dir,
                    self.currentTest,
                )

        except Exception as e:
            logger.error(e)
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
        self.outputfile.write(alltext)
        textline = alltext.split("\n")

        for textStr in textline:
            import re

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
                    self.runwindow.ResultWidget.runtimes[processIndex][self.testIndexTracker].setText(
                        "{0} s".format(round(runningTime, 1))
                    )
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
                        self.runwindow.ResultWidget.ProgressBars[processIndex][self.testIndexTracker].setValue(self.ProgressValue)
                        ##Added because of Ph2_ACF bug:

                    except Exception as e:
                        print(f"Error while updating progress bar {e}")
                        pass

                if self.check_for_end_of_test(textStr):
                    self.runwindow.ResultWidget.ProgressBars[processIndex][self.testIndexTracker].setValue(100)
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
                    self.runwindow.ResultWidget.ProgressBars[processIndex][self.testIndexTracker].setValue(100)
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

    def check_for_end_of_test(self, textStr):
        # function to support the quick fix in on_readyReadStandardOutput() where
        # the progress bar doesn't always reach 100%.
        currentTest = Test_to_Ph2ACF_Map[self.currentTest]
        if currentTest in ["thradj", "thrmin"] and "Global threshold for" in textStr:
            return True
        elif currentTest in ["threq"] and "Best VCAL_HIGH" in textStr:
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
        return False

    # Reads data that is normally printed to the terminal and saves it to the output file
    @QtCore.pyqtSlot()
    def on_readyReadStandardOutput_info(self, processIndex: int):
        if os.path.exists(self.outputFile):
            outputfile = open(self.outputFile, "a")
        else:
            outputfile = open(self.outputFile, "w")

        alltext = (
            self.info_processes[processIndex].readAllStandardOutput().data().decode()
        )
        outputfile.write(alltext)
        outputfile.close()
        textline = alltext.split("\n")

        for textStr in textline:
            self.outputString.emit(textStr, self.runwindow.ConsoleViews[processIndex])

    @QtCore.pyqtSlot()
    def on_finish(self, processIndex: int):
        self.outputfile.close()
        # While the process is killed:

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
            self.saveTest(processIndex)
            return

        self.saveConfigs()

        # Save the output ROOT file to output_dir
        self.saveTest(processIndex)
        self.testIndexTracker += 1

        # validate the results
        self.validateTest()

        EnableReRun = self.onFinalTest(self.testIndexTracker)
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
                CompositeTests[self.info]
            ):  # Checks that this was the last test in the sequence.
                self.powerSignal.emit()
                EnableReRun = True
                
                if self.info == "FWD-RVS Bias" or self.info == "CrossTalk":
                    self.bumpbond_analysis()

                if len(self.BBanalysis_root_files) > 0:
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

                                self.felis.set_result(
                                    self.BBanalysis_root_files,
                                    module_data["module"].getModuleName(),
                                    f"{index:02d}_PixelAlive",
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

    def updateProgress(self, measurementType, stepSize):
        if measurementType == "IVCurve":
            self.IVProgressValue += stepSize / 2.0
            for i, firmware in enumerate(self.firmware):
                self.runwindow.ResultWidget.ProgressBars[i][self.testIndexTracker].setValue(
                    self.IVProgressValue
                )
            self.ramp_progress_bar(
                [
                    site_settings.IVcurve_range[self.currentTest]
                    if site_settings.IVcurve_range[self.currentTest] < 0
                    else 80
                ]
                * len(self.instruments._module_dict.values())
            )
        if "SLDO" in measurementType:
            self.SLDOProgressValue += stepSize
            for i, firmware in enumerate(self.firmware):
                self.runwindow.ResultWidget.ProgressBars[i][self.testIndexTracker].setValue(
                    self.SLDOProgressValue
                )

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
            np.savetxt(
                csvfilename, (measure["voltage"], measure["current"]), delimiter=","
            )
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

        EnableReRun = False

        # Will send signal to turn off power supply after composite or single tests are run
        if isCompositeTest(self.info):
            if self.testIndexTracker == len(CompositeTests[self.info]):
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

            if self.testIndexTracker == len(CompositeTests[self.info]):
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
                for i, _ in enumerate(self.firmware):
                    self.runwindow.ResultWidget.runtimes[i][self.testIndexTracker].setText(
                        ""
                    )  # may need to .update()
                    self.runwindow.ResultWidget.ProgressBars[i][self.testIndexTracker].setValue(
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