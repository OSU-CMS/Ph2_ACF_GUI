from PyQt5 import QtCore
from PyQt5.QtCore import Qt, QTimer

# from PyQt5.QtGui import QPixmap
from PyQt5.QtWidgets import (
    QGridLayout,
    QLabel,
    QProgressBar,
    QPushButton,
    QScrollArea,
    QTreeWidget,
    QTreeWidgetItem,
    QWidget,
    QSizePolicy
)
from PyQt5 import QtSvg

import os
import subprocess

# from Gui.GUIutils.settings import *
from Gui.GUIutils.guiUtils import isCompositeTest
from Gui.python.ROOTInterface import (
    GetDirectory,
    TCanvas2SVG,
)
from Gui.QtGUIutils.QtTCanvasWidget import QtTCanvasWidget
from Gui.python.logging_config import logger
from InnerTrackerTests.TestSequences import CompositeTests


class ResultTreeWidget(QWidget):
    def __init__(self, info, width, height, master, firmware):
        super(ResultTreeWidget, self).__init__()
        self.master = master
        self.firmware = firmware
        self.DisplayW = width
        self.DisplayH = height
        self.FileList = []
        self.IVFileList = []
        self.SLDOFileList = []
        self.info = info

        self.ProgressBarLists = [[] for _ in firmware]
        self.ProgressBars = [{} for _ in firmware]
        self.runtimes = [{} for _ in firmware]
        self.runtimeLists = [[] for _ in firmware]

        self.displayingImage = ""
        self.displayList = []
        self.displayIndex = 0
        self.allDisplayed = False
        self.timerFrozen = False
        self.Plot = []
        self.count = 0
        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)
        self.initializeProgressBar()
        self.setupUi()

    def initializeProgressBar(self):
        for fw_index, _ in enumerate(self.firmware):
            if isCompositeTest(self.info):
                self.ProgressBarLists[fw_index] = CompositeTests[self.info]
                self.runtimeLists[fw_index] = CompositeTests[self.info]
            else:
                self.ProgressBarLists[fw_index] = [self.info]
                self.runtimeLists[fw_index] = [self.info]

            for index, obj in enumerate(self.ProgressBarLists[0]):
                ProgressBar = QProgressBar()
                ProgressBar.setMinimum(0)
                ProgressBar.setMaximum(100)
                self.ProgressBars[fw_index][index] = ProgressBar
                runtime = QLabel()
                self.runtimes[fw_index][index] = runtime

    def setupUi(self):
        self.ScrollAreas = [QScrollArea() for _ in self.firmware]
        self.ProgressWidgets = [QWidget() for _ in self.firmware]
        self.ProgressLayouts = [QGridLayout() for _ in self.firmware]
        for fw_index, firmware in enumerate(self.firmware):
            self.ScrollAreas[fw_index].setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
            self.ScrollAreas[fw_index].setWidgetResizable(True)
            self.ProgressLayouts[fw_index].setAlignment(Qt.AlignTop)
            self.ProgressLayouts[fw_index].addWidget(QLabel(firmware.getBoardName()), 0, 0, 1, Qt.AlignTop)

            for index, key in enumerate(self.ProgressBars[fw_index].keys()):
                testLabel = QLabel("<b>{}</b>".format(self.ProgressBarLists[fw_index][index]))
                testProgress = self.ProgressBars[fw_index][key]
                statusLabel = QLabel()
                
                self.ProgressLayouts[fw_index].addWidget(testLabel, index+1, 0, 1, 1, Qt.AlignTop)
                self.ProgressLayouts[fw_index].addWidget(testProgress, index+1, 1, 1, 4, Qt.AlignTop)
                self.ProgressLayouts[fw_index].addWidget(statusLabel, index+1, 5, 1, 1, Qt.AlignTop)
                self.ProgressLayouts[fw_index].addWidget(
                    self.runtimes[fw_index][index], index+1, 5, 1, 1, Qt.AlignTop
                )

            self.ProgressWidgets[fw_index].setLayout(self.ProgressLayouts[fw_index])
            self.ScrollAreas[fw_index].setWidget(self.ProgressWidgets[fw_index])
            self.mainLayout.addWidget(self.ScrollAreas[fw_index], 1, fw_index, 10, 1)

        if self.master.expertMode:
            self.OutputTree = QTreeWidget()
            self.OutputTree.setMinimumWidth(320)
            self.OutputTree.horizontalScrollBar().setEnabled(True)
            self.OutputTree.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
            self.OutputTree.setHeaderLabels(["Name"])
            self.OutputTree.itemClicked.connect(self.onItemClicked)
            self.OutputTree.itemExpanded.connect(self.onItemExpanded)
            self.TreeRoot = QTreeWidgetItem(self.OutputTree)
            self.TreeRoot.setText(0, "Files...")
        else:
            self.TestLabel = QLabel("Test")

            self.ControlButtom = QPushButton("Pause")
            self.ControlButtom.clicked.connect(self.controlDisplay)

            self.rightArrow = QPushButton("->")
            self.rightArrow.clicked.connect(self.rightArrowFunc)
            self.leftArrow = QPushButton("<-")
            self.leftArrow.clicked.connect(self.leftArrowFunc)

            self.SVGWidget = QtSvg.QSvgWidget()
            minHeight = 400
            ratio = 1.5
            self.SVGWidget.setMinimumHeight(minHeight)
            self.SVGWidget.setMinimumWidth(minHeight * ratio)

        if self.master.expertMode:
            self.mainLayout.addWidget(self.OutputTree, 0, 1+len(self.firmware), 10, 2)
        else:
            self.mainLayout.addWidget(self.TestLabel, 0, 1+len(self.firmware), 1, 2)
            self.mainLayout.addWidget(self.ControlButtom, 0, 4+len(self.firmware), 1, 1)
            self.mainLayout.addWidget(self.rightArrow, 0, 3+len(self.firmware), 1, 1)
            self.mainLayout.addWidget(self.leftArrow, 0, 2+len(self.firmware), 1, 1)
            self.mainLayout.addWidget(self.SVGWidget, 1, 1+len(self.firmware), 9, 3)

        if not self.master.expertMode:
            # Initialize timer:
            self.timer = QTimer(self)
            # adding action to timer
            self.timer.timeout.connect(self.showNextPlot)
            # update the timer every second
            self.timer.start(3000)

    ## Old methods: To be removed
    def resizeImage(self, width, height):
        pass
        # self.DisplayView = QPixmap(self.displayingImage).scaled(QSize(width,height), Qt.KeepAspectRatio, Qt.SmoothTransformation)
        # self.DisplayLabel.setPixmap(self.DisplayView)
        # self.update

    @QtCore.pyqtSlot(QTreeWidgetItem, int)
    def onItemClicked(self, item, col):
        self.OutputTree.resizeColumnToContents(0)
        if item.text(0).endswith(";TCanvas"):
            temp = item.clone()
            while item.parent().text(0) != "Files...":
                item = item.parent()
            runNumber = item.text(0).split("_")[0]
            # print("the test is {0}".format(item.text(0)))
            # print("This item is a TCanvas")
            canvas = temp.data(0, Qt.UserRole)
            canvasname = str(temp.text(0))
            canvasname = canvasname.split(";")[0]
            # print("The canvas is {0}".format(canvas))
            self.displayResult(canvas, canvasname, runNumber)
        elif "svg" in str(item.data(0, Qt.UserRole)):
            canvas = item.data(0, Qt.UserRole)
            self.displayResult(canvas)

    @QtCore.pyqtSlot(QTreeWidgetItem)
    def onItemExpanded(self, item):
        self.OutputTree.resizeColumnToContents(0)

    def DirectoryVAL(self, QTreeNode, node):
        if node.getDaugthers() != []:
            for Node in node.getDaugthers():
                CurrentNode = QTreeWidgetItem()
                if Node.getClassName() == "TCanvas":
                    CurrentNode.setText(0, Node.getKeyName() + ";TCanvas")
                    CurrentNode.setData(0, Qt.UserRole, Node.getObject())
                else:
                    CurrentNode.setText(0, Node.getKeyName())
                QTreeNode.addChild(CurrentNode)
                self.DirectoryVAL(CurrentNode, Node)
        else:
            return

    def getResult(self, QTreeNode, sourceFile):
        Nodes = GetDirectory(sourceFile)

        CurrentNode = QTreeWidgetItem()
        for Node in Nodes:
            CurrentNode = QTreeWidgetItem()
            CurrentNode.setText(0, Node.getKeyName())
            QTreeNode.addChild(CurrentNode)
            self.DirectoryVAL(CurrentNode, Node)

    def updateDisplayList(self, step, resultDict):
        toBeDisplayed = len(self.displayList)
        for module in resultDict.keys():
            for plot in resultDict[module]:
                if (step, plot) not in self.displayList:
                    self.displayList.append((step, plot))

        if self.allDisplayed:
            self.displayIndex = toBeDisplayed
            if not self.timerFrozen:
                self.showNextPlot()
            self.allDisplayed = False

    def showNextPlot(self):
        self.timer.start(3000)
        if len(self.displayList) > 0:
            self.showPlot()
            self.displayIndex += 1

    def showPlot(self):
        if self.displayIndex == len(self.displayList):
            self.allDisplayed = True
        self.displayIndex = self.displayIndex % len(self.displayList)
        step, displayPlot = self.displayList[self.displayIndex]
        self.TestLabel.setText("Step{}".format(step))
        self.SVGWidget.load(displayPlot)

    def rightArrowFunc(self):
        self.timer.start(3000)
        if len(self.displayList) > 0:
            self.displayIndex += 1
            self.showPlot()

    def leftArrowFunc(self):
        self.timer.start(3000)
        if len(self.displayList) > 0:
            self.displayIndex -= 1
            self.showPlot()

    def controlDisplay(self):
        if self.ControlButtom.text() == "Pause":
            self.timer.stop()
            self.timerFrozen = True
            self.ControlButtom.setText("Resume")
        elif self.ControlButtom.text() == "Resume":
            self.timer.start()
            self.timerFrozen = False
            self.ControlButtom.setText("Pause")

    def updateResult(self, sourceFolder):
        process = subprocess.run(
            'find {0} -type f -name "*.root" '.format(sourceFolder),
            shell=True,
            stdout=subprocess.PIPE,
        )
        stepFiles = process.stdout.decode("utf-8").rstrip("\n").split("\n")

        if stepFiles == [""]:
            return
        self.FileList = []

        self.FileList += stepFiles

        for File in set(self.FileList):
            CurrentNode = QTreeWidgetItem()
            CurrentNode.setText(0, File.split("/")[-1])
            CurrentNode.setData(0, Qt.UserRole, File)
            self.TreeRoot.addChild(CurrentNode)
            self.getResult(CurrentNode, File)

    def updateIVResult(self, sourceFolder):
        process2 = subprocess.run(
            'find {0} -type f -name "*IVCurve_Module_*.svg" '.format(sourceFolder),
            shell=True,
            stdout=subprocess.PIPE,
        )
        stepFiles2 = process2.stdout.decode("utf-8").rstrip("\n").split("\n")

        if stepFiles2 == [""]:
            # print("No IV files found.")  # Debugging output if no IV files are found
            return
        # print("IV files found:", stepFiles2)  # Debugging output to show the found IV files

        self.IVFileList += stepFiles2

        for File in set(self.IVFileList):
            CurrentNode = QTreeWidgetItem()
            CurrentNode.setText(0, File.split("/")[-1])
            CurrentNode.setData(0, Qt.UserRole, File)
            self.TreeRoot.addChild(CurrentNode)
        # print("IV files processed.")  # Debugging output to indicate IV files processing is done

    def updateSLDOResult(self, sourceFolder):
        process2 = subprocess.run(
            'find {0} -type f -name "*.svg" '.format(sourceFolder),
            shell=True,
            stdout=subprocess.PIPE,
        )
        stepFiles2 = process2.stdout.decode("utf-8").rstrip("\n").split("\n")

        if stepFiles2 == [""]:
            print("No SLD files found.")  # Debugging output if no SLD files are found
            return

        print(
            "SLD files found:", stepFiles2
        )  # Debugging output to show the found SLD files

        self.SLDOFileList += stepFiles2

        for File in set(self.SLDOFileList):
            CurrentNode = QTreeWidgetItem()
            CurrentNode.setText(0, File.split("/")[-1])
            CurrentNode.setData(0, Qt.UserRole, File)
            self.TreeRoot.addChild(CurrentNode)
        print(
            "SLD files processed."
        )  # Debugging output to indicate SLD files processing is done

    def displayResult(self, canvas, name=None, runNumber=""):
        tmpDir = os.environ.get("GUI_dir") + f"/Gui/.tmp/{runNumber}"
        # tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
        if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
            try:
                os.mkdir(tmpDir)
                logger.info("Creating " + tmpDir)
            except OSError:
                logger.warning("Failed to create " + tmpDir)

        if "svg" in str(canvas):
            svgFile = str(canvas)
        else:
            svgFile = TCanvas2SVG(tmpDir, canvas, name)
        self.displayingImage = svgFile

        try:
            # self.DisplayView = QPixmap(jpgFile).scaled(QSize(self.DisplayW,self.DisplayH), Qt.KeepAspectRatio, Qt.SmoothTransformation)
            # self.DisplayLabel.setPixmap(self.DisplayView)
            # self.update
            self.Plot.append(QtTCanvasWidget(self.master, svgFile))
            logger.info("Displaying " + svgFile)
        except Exception as e:
            logger.error("Failed to display " + svgFile + f"due to error {e}")
        pass
