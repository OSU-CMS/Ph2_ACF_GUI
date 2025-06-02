from PyQt5.QtCore import pyqtSignal, Qt, QSize
from PyQt5.QtGui import QPixmap, QColor, QImage
from PyQt5.QtWidgets import (
    QAbstractItemView,
    QCheckBox,
    QGridLayout,
    QGroupBox,
    QLabel,
    QPlainTextEdit,
    QPushButton,
    QTableWidget,
    QTableWidgetItem,
    QHBoxLayout,
    QWidget,
    QMessageBox,
    QSplitter,
    QProgressBar,
    QApplication,
)

import os
import numpy as np
import threading
import Gui.siteSettings as site_settings

from Gui.GUIutils.guiUtils import isCompositeTest
from Gui.QtGUIutils.Loading import LoadingThread, LoadingWheel

from Gui.QtGUIutils.QtCustomizeWindow import QtCustomizeWindow

# from Gui.QtGUIutils.QtTableWidget import *
# from Gui.QtGUIutils.QtMatplotlibUtils import *
from Gui.python.ResultTreeWidget import ResultTreeWidget
from Gui.python.TestHandler import TestHandler
from Gui.python.logging_config import logger
from InnerTrackerTests.TestSequences import CompositeTests


class QtRunWindow(QWidget):
    resized = pyqtSignal()

    def __init__(self, master, info, firmware):
        super(QtRunWindow, self).__init__()
        self.master = master
        self.master.globalStop.connect(self.urgentStop)

        # self.LogoGroupBox = self.master.LogoGroupBox
        self.firmware = firmware
        self.info = info

        # Removing for sequencefix
        #        if "AllScan_Tuning" in self.info:
        #            runTestList = pretuningList
        #            runTestList.extend(tuningList * len(defaultTargetThr))
        #            runTestList.extend(posttuningList)
        #            CompositeList.update({"AllScan_Tuning": runTestList})

        self.ModuleMap = dict()
        self.ModuleType = self.firmware[0].getModuleData()["type"]

        self.RunNumber = "-1"

        # Add TestProcedureHandler
        self.testHandler = TestHandler(self, master, info, firmware)
        if not site_settings.manual_powersupply_control:
            assert self.master.instruments is not None, logger.error(
                "Unable to setup instruments"
            )
            self.testHandler.powerSignal.connect(self.onPowerSignal)

        self.GroupBoxSeg = [1, 10, 1]
        self.HorizontalSeg = [3, 5]
        self.VerticalSegCol0 = [1, 3]
        self.VerticalSegCol1 = [2, 2]
        self.DisplayH = self.height() * 3.0 / 7
        self.DisplayW = self.width() * 3.0 / 7

        self.processingFlag = False
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

        self.backSignal = False
        self.haltSignal = False
        self.finishSignal = False
        self.proceedSignal = False

        self.runNext = threading.Event()
        self.testIndexTracker = -1
        self.listWidgetIndex = 0
        self.outputDirQueue = []
        # Fixme: QTimer to be added to update the page automatically
        self.grades = []
        self.modulestatus = []
        self.finished_tests = []
        self.autoSave = False

        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)

        self.ledMap = {
            "off": QPixmap.fromImage(
                QImage("icons/led-off.png").scaled(
                    QSize(60, 30), Qt.KeepAspectRatio, Qt.SmoothTransformation
                )
            ),
            "green": QPixmap.fromImage(
                QImage("icons/led-green-on.png").scaled(
                    QSize(60, 30), Qt.KeepAspectRatio, Qt.SmoothTransformation
                )
            ),
            "orange": QPixmap.fromImage(
                QImage("icons/led-amber-on.png").scaled(
                    QSize(60, 30), Qt.KeepAspectRatio, Qt.SmoothTransformation
                )
            ),
            "red": QPixmap.fromImage(
                QImage("icons/led-red-on.png").scaled(
                    QSize(60, 30), Qt.KeepAspectRatio, Qt.SmoothTransformation
                )
            ),
        }

        self.setLoginUI()
        # self.initializeRD53Dict()
        self.createHeadLine()
        self.createApp()
        self.createMain()
        self.occupied()

        self.resized.connect(self.rescaleImage)

    def onPowerSignal(self):
        starting_voltages = [
            np.abs(getattr(module["hv"], "voltage"))
            for module in self.master.instruments._module_dict.values()
        ]
        self.master.instruments.off(
            hv_delay=0.3,
            hv_step_size=10,
            measure=False,
            execute_each_step=lambda: self.testHandler.ramp_progress_bar(
                starting_voltages
            ),
        )

    def setLoginUI(self):
        X = self.master.dimension.width() / 10
        Y = self.master.dimension.height() / 10
        Width = self.master.dimension.width() * 8.0 / 10
        Height = self.master.dimension.height() * 8.0 / 10
        self.setGeometry(X, Y, Width, Height)
        self.setWindowTitle("Run Control Page")
        self.DisplayH = self.height() * 3.0 / 7
        self.DisplayW = self.width() * 3.0 / 7
        self.show()

    def createHeadLine(self):
        self.HeadBox = QGroupBox()

        self.HeadLayout = QHBoxLayout()

        HeadLabel = QLabel('<font size="4"> Test: {0} </font>'.format(self.info))
        HeadLabel.setMaximumHeight(30)

        colorString = "color: green" if self.master.panthera_connected else "color: red"
        StatusLabel = QLabel()
        StatusLabel.setText(self.master.operator_name)
        StatusLabel.setStyleSheet(f"{colorString}; font-size: 14px")

        self.HeadLayout.addWidget(HeadLabel)
        self.HeadLayout.addStretch(1)
        self.HeadLayout.addWidget(StatusLabel)

        self.HeadBox.setLayout(self.HeadLayout)

        self.mainLayout.addWidget(self.HeadBox, 0, 0, self.GroupBoxSeg[0], 1)

    def destroyHeadLine(self):
        self.HeadBox.deleteLater()
        self.mainLayout.removeWidget(self.HeadBox)

    def createMain(self):
        self.testIndexTracker = 0
        self.MainBodyBox = QGroupBox()

        mainbodylayout = QHBoxLayout()

        # Splitters
        MainSplitter = QSplitter(Qt.Horizontal)
        LeftColSplitter = QSplitter(Qt.Vertical)
        RightColSplitter = QSplitter(Qt.Vertical)

        # Group Box for controller
        ControllerBox = QGroupBox()
        ControllerSP = ControllerBox.sizePolicy()
        ControllerSP.setVerticalStretch(self.VerticalSegCol0[0])
        ControllerBox.setSizePolicy(ControllerSP)

        self.ControlLayout = QGridLayout()

        self.CustomizedButton = QPushButton("&Customize...")
        self.CustomizedButton.clicked.connect(self.customizeTest)
        self.ResetButton = QPushButton("&Reset")
        self.ResetButton.clicked.connect(self.resetConfigTest)
        self.RunButton = QPushButton("&Run")
        self.RunButton.setDefault(True)
        self.RunButton.clicked.connect(self.resetConfigTest)
        self.RunButton.clicked.connect(self.initialTest)
        self.RunButton.clicked.connect(lambda: self.RunButton.setDisabled(True))
        self.AbortButton = QPushButton("&Abort")
        self.AbortButton.clicked.connect(self.abortTest)
        self.saveCheckBox = QCheckBox("&auto-save to Panthera")
        self.saveCheckBox.setMaximumHeight(30)
        if self.master.panthera_connected:
            self.saveCheckBox.setChecked(self.testHandler.autoSave)
            self.saveCheckBox.clicked.connect(self.setAutoSave)
        else:
            self.testHandler.autoSave = False
            self.saveCheckBox.setChecked(False)
            self.saveCheckBox.setDisabled(True)

        if self.master.expertMode:
            self.ControlLayout.addWidget(self.RunButton, 0, 0, 1, 1)
            self.ControlLayout.addWidget(self.AbortButton, 0, 1, 1, 1)
            self.ControlLayout.addWidget(self.ResetButton, 0, 2, 1, 1)
            self.ControlLayout.addWidget(self.saveCheckBox, 1, 0, 1, 1)

        else:
            pass

        ControllerBox.setLayout(self.ControlLayout)

        # Group Box for terminal display
        TerminalBox = QGroupBox("&Terminal")
        TerminalSP = TerminalBox.sizePolicy()
        TerminalSP.setVerticalStretch(self.VerticalSegCol0[1])
        TerminalBox.setSizePolicy(TerminalSP)
        TerminalBox.setMinimumWidth(400)

        ConsoleLayout = QGridLayout()

        self.ConsoleViews = [QPlainTextEdit() for i in range(len(self.firmware))]
        for i in range(len(self.ConsoleViews)):
            self.ConsoleViews[i].setStyleSheet(
                "QTextEdit { background-color: rgb(10, 10, 10); color : white; }"
            )
            self.ConsoleViews[i].ensureCursorVisible()
            self.ConsoleViews[i].setReadOnly(True)
            ConsoleLayout.addWidget(QLabel(self.firmware[i].getBoardName()), 0, i)
            ConsoleLayout.addWidget(self.ConsoleViews[i], 1, i)

        TerminalBox.setLayout(ConsoleLayout)

        # Group Box for output display
        OutputBox = QGroupBox("&Result")
        OutputBoxSP = OutputBox.sizePolicy()
        OutputBoxSP.setVerticalStretch(self.VerticalSegCol1[0])
        OutputBox.setSizePolicy(OutputBoxSP)

        OutputLayout = QGridLayout()
        self.ResultWidget = ResultTreeWidget(
            self.info, self.DisplayW, self.DisplayH, self.master, self.firmware
        )
        OutputLayout.addWidget(self.ResultWidget, 0, 0, 1, 1)
        OutputBox.setLayout(OutputLayout)

        # Group Box for history
        self.HistoryBox = QGroupBox("&History")
        HistoryBoxSP = self.HistoryBox.sizePolicy()
        HistoryBoxSP.setVerticalStretch(self.VerticalSegCol1[1])
        self.HistoryBox.setSizePolicy(HistoryBoxSP)

        self.HistoryLayout = QGridLayout()
        # self.StatusCanvas = RunStatusCanvas(parent=self,width=5, height=4, dpi=100)
        self.StatusTable = QTableWidget()
        self.header = ["TestName"]
        for key in self.testHandler.rd53_file.keys():
            ModuleID = key.split("_")[0]
            if f"Module{ModuleID}" not in self.header:
                self.header.append("Module{}".format(ModuleID))
        self.StatusTable.setColumnCount(len(self.header))
        self.StatusTable.setHorizontalHeaderLabels(self.header)
        self.StatusTable.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.StatusTable.itemClicked.connect(self.displayTestResultPopup)
        self.HistoryLayout.addWidget(self.StatusTable)
        self.HistoryBox.setLayout(self.HistoryLayout)

        self.TempBox = QGroupBox()
        TempBoxSP = self.TempBox.sizePolicy()
        TempBoxSP.setVerticalStretch(self.VerticalSegCol1[1])
        self.TempBox.setSizePolicy(TempBoxSP)
        self.TempLayout = QGridLayout()
        TempLabel = QLabel("Temperature Status:")
        self.tempIndicator = QLabel()
        self.tempIndicator.setPixmap(self.ledMap["off"])

        self.TempLayout.addWidget(TempLabel, 1, 0, 1, 1)
        self.TempLayout.addWidget(self.tempIndicator, 1, 1, 1, 1)
        self.TempBox.setLayout(self.TempLayout)

        self.RampBox = QGroupBox()
        self.RampLayout = QGridLayout()
        self.RampProgressBars = [QProgressBar()] * len(
            self.master.instruments._module_dict.values()
        )
        RampProgressLabels = [QLabel("Bias Voltage:")] * len(
            self.master.instruments._module_dict.values()
        )
        for label in RampProgressLabels:
            label.setStyleSheet("font-weight: bold;")

        for i in range(len(self.master.instruments._module_dict.values())):
            self.RampLayout.addWidget(RampProgressLabels[i], i, 0, 1, 1)
            self.RampLayout.addWidget(self.RampProgressBars[i], i, 1, 1, 1)
        self.RampBox.setLayout(self.RampLayout)

        LeftColSplitter.addWidget(ControllerBox)
        LeftColSplitter.addWidget(TerminalBox)
        LeftColSplitter.addWidget(self.RampBox)
        LeftColSplitter.addWidget(self.TempBox)
        RightColSplitter.addWidget(OutputBox)
        RightColSplitter.addWidget(self.HistoryBox)
        RightColSplitter.addWidget(self.AppOption)

        LeftColSplitterSP = LeftColSplitter.sizePolicy()
        LeftColSplitterSP.setHorizontalStretch(self.HorizontalSeg[0])
        LeftColSplitter.setSizePolicy(LeftColSplitterSP)

        RightColSplitterSP = RightColSplitter.sizePolicy()
        RightColSplitterSP.setHorizontalStretch(self.HorizontalSeg[1])
        RightColSplitter.setSizePolicy(RightColSplitterSP)

        MainSplitter.addWidget(LeftColSplitter)
        MainSplitter.addWidget(RightColSplitter)

        mainbodylayout.addWidget(MainSplitter)

        self.MainBodyBox.setLayout(mainbodylayout)
        self.mainLayout.addWidget(
            self.MainBodyBox, sum(self.GroupBoxSeg[0:1]), 0, self.GroupBoxSeg[1], 1
        )

    def updateTempIndicator(self, color: str):
        self.tempIndicator.setPixmap(self.ledMap[color])
        if color == "red":
            self.abortTest()

    def destroyMain(self):
        self.MainBodyBox.deleteLater()
        self.mainLayout.removeWidget(self.MainBodyBox)

    def upload_to_Panthera_starter(self):
        self.UploadProgressBar = QProgressBar()
        self.UploadWheel = LoadingWheel()
        self.UploadProgressBar.setFormat(f"0/{len(self.testHandler.modules)} uploaded")
        self.StartLayout.insertWidget(1, self.UploadProgressBar)
        self.StartLayout.insertWidget(1, self.UploadWheel)
        self.AppOption.repaint()

        self.Panthera_thread = LoadingThread(self.testHandler.upload_to_Panthera, 50)
        self.Panthera_thread.finished.connect(self.UploadWheel.close)
        self.Panthera_thread.timer.timeout.connect(self.UploadWheel.update_spinner)
        self.Panthera_thread.timer.start()
        self.Panthera_thread.start()  # Start the thread

    def createApp(self):
        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()

        self.UploadButton = QPushButton("&Upload Results")
        self.UploadButton.clicked.connect(self.upload_to_Panthera_starter)
        self.UploadButton.setDisabled(True)

        self.BackButton = QPushButton("&Back")
        self.BackButton.clicked.connect(self.sendBackSignal)
        self.BackButton.clicked.connect(self.closeWindow)
        self.BackButton.clicked.connect(self.creatStartWindow)

        self.FinishButton = QPushButton("&Finish")
        self.FinishButton.setDefault(True)
        self.FinishButton.clicked.connect(self.closeWindow)

        self.StartLayout.addStretch(1)

        self.StartLayout.addWidget(self.UploadButton)

        self.StartLayout.addWidget(self.BackButton)
        self.StartLayout.addWidget(self.FinishButton)
        self.AppOption.setLayout(self.StartLayout)

        self.LogoGroupBox = QGroupBox("")
        self.LogoGroupBox.setCheckable(False)
        self.LogoGroupBox.setMaximumHeight(100)

        self.LogoLayout = QHBoxLayout()
        OSULogoLabel = QLabel()
        OSUimage = QImage("icons/osuicon.jpg").scaled(
            QSize(200, 60), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        OSUpixmap = QPixmap.fromImage(OSUimage)
        OSULogoLabel.setPixmap(OSUpixmap)
        CMSLogoLabel = QLabel()
        CMSimage = QImage("icons/cmsicon.png").scaled(
            QSize(200, 60), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        CMSpixmap = QPixmap.fromImage(CMSimage)
        CMSLogoLabel.setPixmap(CMSpixmap)
        self.LogoLayout.addWidget(OSULogoLabel)
        self.LogoLayout.addStretch(1)
        self.LogoLayout.addWidget(CMSLogoLabel)

        self.LogoGroupBox.setLayout(self.LogoLayout)

        self.mainLayout.addWidget(
            self.LogoGroupBox, sum(self.GroupBoxSeg[0:3]), 0, self.GroupBoxSeg[2], 1
        )

    def destroyApp(self):
        self.AppOption.deleteLater()

    def closeWindow(self):
        self.close()

    def creatStartWindow(self):
        if self.backSignal and self.master.expertMode:
            self.master.openNewTest()

    def occupied(self):
        self.master.ProcessingTest = True

    def release(self):
        self.testHandler.abortTest()
        self.master.ProcessingTest = False
        if self.master.expertMode:
            self.master.NewTestButton.setDisabled(False)
            self.master.LogoutButton.setDisabled(False)
            self.master.ExitButton.setDisabled(False)
        else:
            self.master.SimpleMain.RunButton.setDisabled(False)
            self.master.SimpleMain.StopButton.setDisabled(True)

    def refreshHistory(self):
        # self.dataList = getLocalRemoteTests(self.connection, self.info[0])
        # self.proxy = QtTableWidget(self.dataList)
        # self.view.setModel(self.proxy)
        # self.view.setEditTriggers(QAbstractItemView.NoEditTriggers)
        # self.view.update()
        print("attempting to update status in history")
        self.HistoryLayout.removeWidget(self.StatusTable)
        self.StatusTable.setRowCount(0)
        for test, test_results in zip(self.finished_tests, self.modulestatus):
            row = self.StatusTable.rowCount()
            self.StatusTable.setRowCount(row + 1)
            if isCompositeTest(self.info):
                self.StatusTable.setItem(row, 0, QTableWidgetItem(test))
            else:
                self.StatusTable.setItem(row, 0, QTableWidgetItem(self.info))
            for module_result in test_results:
                moduleName = list(module_result.keys())[0]
                status = "Pass" if module_result[moduleName][0] else "Failed"
                moduleID = f"Module{moduleName}"
                if moduleID in self.header:
                    columnID = self.header.index(moduleID)
                    self.StatusTable.setItem(row, columnID, QTableWidgetItem(status))
                    if status == "Pass":
                        self.StatusTable.item(row, columnID).setBackground(
                            QColor(Qt.green)
                        )
                    elif status == "Failed":
                        self.StatusTable.item(row, columnID).setBackground(
                            QColor(Qt.red)
                        )

        self.StatusTable.resizeColumnsToContents()
        self.HistoryLayout.addWidget(self.StatusTable)

    def displayTestResultPopup(self, item):
        try:
            row = item.row()  # row = index, they are aligned in refreshHistory()
            col = item.column()
            message = self.modulestatus[row][self.header.index(self.header[col]) - 1][
                self.header[col].lstrip("Module")
            ][1]

            msg_box = QMessageBox()
            msg_box.setWindowTitle("Additional Information")
            msg_box.setText(message)
            msg_box.exec_()
        except KeyError as e:
            if e.args[0] != "TestName":
                raise e

    def sendBackSignal(self):
        self.backSignal = True

    def sendProceedSignal(self):
        self.testHandler.proceedSignal = True
        # self.runNext.set()

    def customizeTest(self):
        print("Customize configuration")
        self.CustomizedButton.setDisabled(True)
        self.ResetButton.setDisabled(True)
        self.RunButton.setDisabled(True)
        self.CustomizedWindow = QtCustomizeWindow(self, self.testHandler.rd53_file)
        self.CustomizedButton.setDisabled(False)
        self.ResetButton.setDisabled(False)
        self.RunButton.setDisabled(False)

    def resetConfigTest(self):
        self.testHandler.resetConfigTest()

    def initialTest(self):
        isReRun = False
        if "Re" in self.RunButton.text():
            isReRun = True
            self.grades = []
            if isCompositeTest(self.info):
                for index in range(len(CompositeTests[self.info])):
                    self.ResultWidget.ProgressBar[index].setValue(0)
                    self.ResultWidget.runtime[index].setText("")
            else:
                self.ResultWidget.ProgressBar[0].setValue(0)
                self.ResultWidget.runtime[0].setText("")
        self.ResetButton.setDisabled(True)
        self.testHandler.runTest(isReRun)

    def abortTest(self):
        reply = QMessageBox.question(
            None,
            "Abort",
            "Are you sure to abort?",
            QMessageBox.No | QMessageBox.Yes,
            QMessageBox.No,
        )

        if reply == QMessageBox.Yes:
            self.testHandler.abortTest()
        else:
            return

    def urgentStop(self):
        self.testHandler.urgentStop()

    #######################################################################
    ##  For result display
    #######################################################################
    def clickedOutputItem(self, qmodelindex):
        # Fixme: Extract the info from ROOT file
        item = self.ListWidget.currentItem()
        referName = item.text().split("_")[0]
        if referName in [
            "GainScan",
            "Latency",
            "NoiseScan",
            "PixelAlive",
            "SCurveScan",
            "ThresholdEqualization",
            "GainOptimization",
            "ThresholdMinimization",
            "InjectionDelay",
        ]:
            self.ReferView = QPixmap(
                os.environ.get("GUI_dir") + "/Gui/test_plots/{0}.png".format(referName)
            ).scaled(
                QSize(self.DisplayW, self.DisplayH),
                Qt.KeepAspectRatio,
                Qt.SmoothTransformation,
            )
            self.ReferLabel.setPixmap(self.ReferView)

    #######################################################################
    ##  For real-time terminal display
    #######################################################################

    def updateConsoleInfo(self, text: str, console: QPlainTextEdit):
        textCursor = console.textCursor()
        console.setTextCursor(textCursor)
        console.appendHtml(text)

    def finish(self, EnableReRun):
        self.RunButton.setDisabled(True)
        self.RunButton.setText("&Continue")
        self.finishSignal = True

        if EnableReRun:
            self.RunButton.setText("&Re-run")
            self.RunButton.setDisabled(False)
            if self.master.panthera_connected:
                self.UploadButton.setDisabled(self.testHandler.autoSave)

    def updateResult(self, newResult):
        if self.master.expertMode:
            self.ResultWidget.updateResult(newResult)
        else:
            step, displayDict = newResult
            self.ResultWidget.updateDisplayList(step, displayDict)

    def updateIVResult(self, newResult):
        if self.master.expertMode:
            self.ResultWidget.updateIVResult(newResult)
        else:
            step, displayDict = newResult
            self.ResultWidget.updateDisplayList(step, displayDict)

    def updateSLDOResult(self, newResult):
        if self.master.expertMode:
            self.ResultWidget.updateSLDOResult(newResult)
        else:
            step, displayDict = newResult
            self.ResultWidget.updateDisplayList(step, displayDict)

    def updateValidation(self, results: list):
        try:
            self.modulestatus.append(results)
        except Exception as err:
            logger.error(err)

    def updateFinishedTests(self, tests: list):
        try:
            self.finished_tests = tests
        except Exception as err:
            logger.error(err)

    def updateProgressBar(self, bar: QProgressBar, value: int, text: str):
        bar.setFormat(text)
        bar.setValue(value)
        QApplication.processEvents()  # So the "window not responding" popup doesn't appear
        # QApplication.processEvents() is unideal though. It would have been better to
        # run tests in a QThread, so we don't have to keep pinging the GUI.
        # But ion finna make that happen.

    #######################################################################
    ##  For real-time terminal display
    #######################################################################

    def refresh(self):
        self.destroyHeadLine()
        self.createHeadLine()
        self.destroyApp()
        self.createApp()

    def resizeEvent(self, event):
        self.resized.emit()
        return super(QtRunWindow, self).resizeEvent(event)

    def rescaleImage(self):
        self.DisplayH = self.height() * 3.0 / 7
        self.DisplayW = self.width() * 3.0 / 7
        self.ResultWidget.resizeImage(self.DisplayW, self.DisplayH)

    def setAutoSave(self):
        if self.testHandler.autoSave:
            self.testHandler.autoSave = False
        else:
            self.testHandler.autoSave = True
        self.saveCheckBox.setChecked(self.testHandler.autoSave)

    def closeEvent(self, event):
        if self.processingFlag:
            event.ignore()

        else:
            reply = QMessageBox.question(
                self,
                "Window Close",
                "Are you sure you want to quit the test?",
                QMessageBox.No | QMessageBox.Yes,
                QMessageBox.No,
            )

            if reply == QMessageBox.Yes:
                self.release()
                if self.master.instruments:
                    starting_voltages = [
                        np.abs(getattr(module["hv"], "voltage"))
                        for module in self.master.instruments._module_dict.values()
                    ]
                    self.master.instruments.off(
                        hv_delay=0.3,
                        hv_step_size=10,
                        execute_each_step=lambda: self.testHandler.ramp_progress_bar(
                            starting_voltages
                        ),
                    )
                else:
                    QMessageBox.information(
                        self,
                        "Info",
                        "You must turn off instruments manually",
                        QMessageBox.Ok,
                    )
                event.accept()
            else:
                self.backSignal = False
                event.ignore()
