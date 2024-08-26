from PyQt5.QtCore import pyqtSignal, Qt, QSize
from PyQt5.QtGui import QPixmap, QColor, QImage
from PyQt5.QtWidgets import (
    QAbstractItemView,
    QCheckBox,
    QDialog,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPlainTextEdit,
    QPushButton,
    QTableWidget,
    QTableWidgetItem,
    QHBoxLayout,
    QWidget,
    QMessageBox,
    QSplitter,
)

import os
import threading
import time
import logging

from Gui.GUIutils.DBConnection import checkDBConnection
from Gui.GUIutils.guiUtils import isActive, isCompositeTest

# from Gui.QtGUIutils.QtStartWindow import *
from Gui.QtGUIutils.QtCustomizeWindow import QtCustomizeWindow
#from Gui.QtGUIutils.QtTableWidget import *
#from Gui.QtGUIutils.QtMatplotlibUtils import *
from Gui.QtGUIutils.QtLoginDialog import QtLoginDialog
from Gui.python.ResultTreeWidget import ResultTreeWidget
#from Gui.python.TestValidator import *
#from Gui.python.ANSIColoringParser import *
from Gui.python.TestHandler import TestHandler
from Gui.GUIutils.settings import ModuleLaneMap
from Gui.python.logging_config import logger
from InnerTrackerTests.TestSequences import CompositeTests, Test_to_Ph2ACF_Map


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
        self.ModuleType = self.firmware[0].getModuleData()['type']

        self.RunNumber = "-1"

        # Add TestProcedureHandler
        self.testHandler = TestHandler(self, master, info, firmware)
        assert self.master.instruments is not None, logger.error("Unable to setup instruments")
        self.testHandler.powerSignal.connect(
            lambda: self.master.instruments.off(
                hv_delay=0.3, hv_step_size=10, measure=False
            )
        )

        self.GroupBoxSeg = [1, 10, 1]
        self.HorizontalSeg = [3, 5]
        self.VerticalSegCol0 = [1, 3]
        self.VerticalSegCol1 = [2, 2]
        self.DisplayH = self.height() * 3.0 / 7
        self.DisplayW = self.width() * 3.0 / 7

        self.processingFlag = False
        self.ProgressBarList = []
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
        self.autoSave = False

        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)

        self.ledMap = {
            "off": QPixmap.fromImage(QImage("icons/led-off.png").scaled(
                QSize(60, 30), Qt.KeepAspectRatio, Qt.SmoothTransformation)
            ),
            "green": QPixmap.fromImage(QImage("icons/led-green-on.png").scaled(
                QSize(60, 30), Qt.KeepAspectRatio, Qt.SmoothTransformation)
            ),
            "orange": QPixmap.fromImage(QImage("icons/led-amber-on.png").scaled(
                QSize(60, 30), Qt.KeepAspectRatio, Qt.SmoothTransformation)
            ),
            "red": QPixmap.fromImage(QImage("icons/led-red-on.png").scaled(
                QSize(60, 30), Qt.KeepAspectRatio, Qt.SmoothTransformation)
            ),
        }
        
        self.setLoginUI()
        # self.initializeRD53Dict()
        self.createHeadLine()
        self.createMain()
        self.createApp()
        self.occupied()

        self.resized.connect(self.rescaleImage)

        # added from Bowen
        #self.j = 0
        # stepWiseGlobalValue[0]['TargetThr'] = defaultTargetThr[0]
        # if len(runTestList)>1:
        #for i in range(len(runTestList)):
        #    if runTestList[i] == "ThresholdAdjustment":
        #        self.j += 1
        #    if self.j == 0:
        #        stepWiseGlobalValue[i]["TargetThr"] = defaultTargetThr[self.j]
        #    else:
        #        stepWiseGlobalValue[i]["TargetThr"] = defaultTargetThr[self.j - 1]

        #logger.info(stepWiseGlobalValue)

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

        HeadLabel = QLabel(
            '<font size="4"> Test: {0} </font>'.format(self.info)
        )
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

        kMinimumWidth = 120
        kMaximumWidth = 150
        kMinimumHeight = 30
        kMaximumHeight = 80

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
        # self.RunButton.clicked.connect(self.runTest)
        # self.ContinueButton = QPushButton("&Continue")
        # self.ContinueButton.clicked.connect(self.sendProceedSignal)
        self.AbortButton = QPushButton("&Abort")
        self.AbortButton.clicked.connect(self.abortTest)
        # self.SaveButton = QPushButton("&Save")
        # self.SaveButton.clicked.connect(self.saveTest)
        self.saveCheckBox = QCheckBox("&auto-save to Panthera")
        self.saveCheckBox.setMaximumHeight(30)
        if self.master.panthera_connected:
            self.saveCheckBox.setChecked(self.testHandler.autoSave)
            self.saveCheckBox.clicked.connect(self.setAutoSave)
        else:
            self.testHandler.autoSave = False
            self.saveCheckBox.setChecked(False)
            self.saveCheckBox.setDisabled(True)
        # if not isActive(self.connection):
        #     self.saveCheckBox.setChecked(False)
        #     self.testHandler.autoSave = False
        #     self.saveCheckBox.setDisabled(True)
        
        ##### previous layout ##########
        """

		self.ControlLayout.addWidget(self.CustomizedButton,0,0,1,2)
		self.ControlLayout.addWidget(self.ResetButton,0,2,1,1)
		self.ControlLayout.addWidget(self.RunButton,1,0,1,1)
		self.ControlLayout.addWidget(self.AbortButton,1,1,1,1)
		self.ControlLayout.addWidget(self.saveCheckBox,1,2,1,1)
		"""
        if self.master.expertMode == True:
            self.ControlLayout.addWidget(self.RunButton, 0, 0, 1, 1)
            self.ControlLayout.addWidget(self.AbortButton, 0, 1, 1, 1)
            self.ControlLayout.addWidget(self.ResetButton, 0, 2, 1, 1)
            self.ControlLayout.addWidget(self.saveCheckBox, 1, 0, 1, 1)

        else:
            pass
        # 	self.ControlLayout.addWidget(self.RunButton,0,0,1,1)
        # 	self.ControlLayout.addWidget(self.AbortButton,0,1,1,1)
        # 	self.saveCheckBox.setDisabled(True)
        # 	self.ControlLayout.addWidget(self.saveCheckBox,0,2,1,1)

        ControllerBox.setLayout(self.ControlLayout)

        # Group Box for ternimal display
        TerminalBox = QGroupBox("&Terminal")
        TerminalSP = TerminalBox.sizePolicy()
        TerminalSP.setVerticalStretch(self.VerticalSegCol0[1])
        TerminalBox.setSizePolicy(TerminalSP)
        TerminalBox.setMinimumWidth(400)

        ConsoleLayout = QGridLayout()

        self.ConsoleView = QPlainTextEdit()
        self.ConsoleView.setStyleSheet(
            "QTextEdit { background-color: rgb(10, 10, 10); color : white; }"
        )
        # self.ConsoleView.setCenterOnScroll(True)
        self.ConsoleView.ensureCursorVisible()
        self.ConsoleView.setReadOnly(True)

        ConsoleLayout.addWidget(self.ConsoleView)
        TerminalBox.setLayout(ConsoleLayout)

        # Group Box for output display
        OutputBox = QGroupBox("&Result")
        OutputBoxSP = OutputBox.sizePolicy()
        OutputBoxSP.setVerticalStretch(self.VerticalSegCol1[0])
        OutputBox.setSizePolicy(OutputBoxSP)

        OutputLayout = QGridLayout()
        self.ResultWidget = ResultTreeWidget(
            self.info, self.DisplayW, self.DisplayH, self.master
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

        LeftColSplitter.addWidget(ControllerBox)
        LeftColSplitter.addWidget(TerminalBox)
        LeftColSplitter.addWidget(self.TempBox)
        RightColSplitter.addWidget(OutputBox)
        RightColSplitter.addWidget(self.HistoryBox)

        LeftColSplitterSP = LeftColSplitter.sizePolicy()
        LeftColSplitterSP.setHorizontalStretch(self.HorizontalSeg[0])
        LeftColSplitter.setSizePolicy(LeftColSplitterSP)

        RightColSplitterSP = RightColSplitter.sizePolicy()
        RightColSplitterSP.setHorizontalStretch(self.HorizontalSeg[1])
        RightColSplitter.setSizePolicy(RightColSplitterSP)

        MainSplitter.addWidget(LeftColSplitter)
        MainSplitter.addWidget(RightColSplitter)

        mainbodylayout.addWidget(MainSplitter)
        # mainbodylayout.addWidget(ControllerBox, sum(self.VerticalSegCol0[:0]), sum(self.HorizontalSeg[:0]), self.VerticalSegCol0[0], self.HorizontalSeg[0])
        # mainbodylayout.addWidget(TerminalBox, sum(self.VerticalSegCol0[:1]), sum(self.HorizontalSeg[:0]), self.VerticalSegCol0[1], self.HorizontalSeg[0])
        # mainbodylayout.addWidget(OutputBox, sum(self.VerticalSegCol1[:0]), sum(self.HorizontalSeg[:1]), self.VerticalSegCol1[0], self.HorizontalSeg[1])
        # mainbodylayout.addWidget(HistoryBox, sum(self.VerticalSegCol1[:1]), sum(self.HorizontalSeg[:1]), self.VerticalSegCol1[1], self.HorizontalSeg[1])

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

    def createApp(self):
        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()

        self.UploadButton = QPushButton("&Upload Results")
        self.UploadButton.clicked.connect(self.testHandler.upload_to_Panthera)
        self.UploadButton.setDisabled(True)

        self.BackButton = QPushButton("&Back")
        self.BackButton.clicked.connect(self.sendBackSignal)
        self.BackButton.clicked.connect(self.closeWindow)
        self.BackButton.clicked.connect(self.creatStartWindow)

        self.FinishButton = QPushButton("&Finish")
        self.FinishButton.setDefault(True)
        self.FinishButton.clicked.connect(self.closeWindow)

        self.StartLayout.addStretch(1)
        if self.master.expertMode == True:
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
            self.AppOption, sum(self.GroupBoxSeg[0:2]), 0, self.GroupBoxSeg[2], 1
        )
        self.mainLayout.addWidget(
            self.LogoGroupBox, sum(self.GroupBoxSeg[0:3]), 0, self.GroupBoxSeg[2], 1
        )

    def destroyApp(self):
        self.AppOption.deleteLater()
        self.mainLayout.removeWidget(self.AppOption)

    def closeWindow(self):
        self.close()

    def creatStartWindow(self):
        if self.backSignal == True and self.master.expertMode == True:
            self.master.openNewTest()

    def occupied(self):
        self.master.ProcessingTest = True

    def release(self):
        self.abortTest()
        self.master.ProcessingTest = False
        if self.master.expertMode == True:
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
        for index, test_results in enumerate(self.modulestatus):
            row = self.StatusTable.rowCount()
            self.StatusTable.setRowCount(row + 1)
            if isCompositeTest(self.info):
                self.StatusTable.setItem(
                    row, 0, QTableWidgetItem(CompositeTests[self.info][index % len(CompositeTests[self.info])])
                )
            else:
                self.StatusTable.setItem(row, 0, QTableWidgetItem(self.info))
            for module_result in test_results:
                moduleName = list(module_result.keys())[0]
                status = "Pass" if module_result[moduleName][0] else "Failed"
                moduleID = f"Module{moduleName}"
                if moduleID in self.header:
                    columnID = self.header.index(moduleID)
                    self.StatusTable.setItem(
                        row, columnID, QTableWidgetItem(status)
                    )
                    if status == "Pass":
                        self.StatusTable.item(row, columnID).setBackground(
                            QColor(Qt.green)
                        )
                    elif status == "Failed":
                        self.StatusTable.item(row, columnID).setBackground(
                            QColor(Qt.red)
                        )

        self.HistoryLayout.addWidget(self.StatusTable)

    def displayTestResultPopup(self, item):
        try:
            row = item.row() #row = index, they are aligned in refreshHistory()
            col = item.column()
            message = self.modulestatus[row][self.header.index(self.header[col])-1][self.header[col].lstrip("Module")][1]
        
            msg_box = QMessageBox()
            msg_box.setWindowTitle("Additional Information")
            msg_box.setText(message)
            msg_box.exec_()
        except KeyError as e:
            if e.args[0] != 'TestName':
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
        self.j = 0
        self.testHandler.abortTest()

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

    def updateConsoleInfo(self, text):
        textCursor = self.ConsoleView.textCursor()
        self.ConsoleView.setTextCursor(textCursor)
        self.ConsoleView.appendHtml(text)

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
        # self.ResultWidget.updateResult("/Users/czkaiweb/Research/data")
        if self.master.expertMode:
            self.ResultWidget.updateResult(newResult)
        else:
            #self.ResultWidget.updateResult(newResult)
            step, displayDict = newResult
            self.ResultWidget.updateDisplayList(step, displayDict)

    def updateIVResult(self, newResult):
        # self.ResultWidget.updateResult("/Users/czkaiweb/Research/data")
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

    def updateValidation(self, results:list):
        try:
            self.modulestatus.append(results)
        except Exception as err:
            logger.error(err)

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
        if self.processingFlag == True:
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
                    self.master.instruments.off(
                        hv_delay=0.3, hv_step_size=10
                    )
                else:
                    QMessageBox.information(self, "Info", "You must turn off "
                                            "instruments manually",
                                            QMessageBox.Ok)
                event.accept()
            else:
                self.backSignal = False
                event.ignore()

