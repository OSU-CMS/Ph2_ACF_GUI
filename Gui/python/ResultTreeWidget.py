from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5.QtGui import QPixmap
from PyQt5.QtWidgets import (
    QAbstractItemView,
    QApplication,
    QCheckBox,
    QComboBox,
    QDateTimeEdit,
    QDial,
    QDialog,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QProgressBar,
    QPushButton,
    QRadioButton,
    QScrollBar,
    QScrollArea,
    QSizePolicy,
    QSlider,
    QSpinBox,
    QStyleFactory,
    QTableView,
    QTableWidget,
    QTabWidget,
    QTextEdit,
    QTreeWidget,
    QTreeWidgetItem,
    QWidget,
    QMainWindow,
)
from PyQt5 import QtSvg

import sys
import os
import subprocess
import logging

from functools import partial
from Gui.GUIutils.settings import *
from Gui.GUIutils.guiUtils import *
from Gui.python.ROOTInterface import *
from Gui.QtGUIutils.QtTCanvasWidget import *
from Gui.python.logging_config import logger


class ResultTreeWidget(QWidget):
    def __init__(self, info, width, height, master):
        super(ResultTreeWidget, self).__init__()
        self.master = master
        self.DisplayW = width
        self.DisplayH = height
        self.FileList = []
        self.IVFileList = []
        self.info = info
        self.ProgressBarList = []
        self.ProgressBar = {}
        self.StatusLabel = {}
        self.displayingImage = ""
        self.displayList = []
        self.displayIndex = 0
        self.allDisplayed = False
        self.timerFrozen = False
        self.Plot = []
        self.count = 0
        self.runtime = {}
        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)
        self.initializeProgressBar()
        self.setupUi()

        # For test:
        # self.updateResult("/Users/czkaiweb/Research/data")

    def initializeProgressBar(self):
        if isCompositeTest(self.info[1]):
            self.ProgressBarList = CompositeList[self.info[1]]
            self.runtimeList = CompositeList[self.info[1]]
        else:
            self.ProgressBarList = [self.info[1]]
            self.runtimeList = [self.info[1]]

        for index, obj in enumerate(self.ProgressBarList):
            ProgressBar = QProgressBar()
            ProgressBar.setMinimum(0)
            ProgressBar.setMaximum(100)
            self.ProgressBar[index] = ProgressBar
            runtime = QLabel()
            self.runtime[index] = runtime

    def setupUi(self):
        # self.DisplayTitle = QLabel('<font size="6"> Result: </font>')
        # self.DisplayLabel = QLabel()
        # self.DisplayLabel.setScaledContents(True)
        # self.displayingImage = 'test_plots/test_best1.png'
        # self.DisplayView = QPixmap('test_plots/test_best1.png').scaled(QSize(self.DisplayW,self.DisplayH), Qt.KeepAspectRatio, Qt.SmoothTransformation)
        # self.DisplayLabel.setPixmap(self.DisplayView)
        # self.ReferTitle = QLabel('<font size="6"> Reference: </font>')
        # self.ReferLabel = QLabel()
        # self.ReferLabel.setScaledContents(True)
        # self.ReferView = QPixmap('test_plots/test_best1.png').scaled(QSize(self.DisplayW,self.DisplayH), Qt.KeepAspectRatio, Qt.SmoothTransformation)
        # self.ReferLabel.setPixmap(self.ReferView)

        self.ScrollArea = QScrollArea()
        self.ProgressWidget = QWidget()
        self.ProgressWidget.setMinimumWidth(400)
        self.ProgressLayout = QGridLayout()
        self.ProgressLayout.setAlignment(Qt.AlignTop)

        for index, key in enumerate(self.ProgressBar.keys()):
            testLabel = QLabel("<b>{}</b>".format(self.ProgressBarList[index]))
            testProgress = self.ProgressBar[key]
            statusLabel = QLabel()

            self.ProgressLayout.addWidget(testLabel, index, 0, 1, 1, Qt.AlignTop)
            self.ProgressLayout.addWidget(testProgress, index, 1, 1, 4, Qt.AlignTop)
            self.ProgressLayout.addWidget(statusLabel, index, 6, 1, 1, Qt.AlignTop)
            self.ProgressLayout.addWidget(
                self.runtime[index], index, 5, 1, 1, Qt.AlignTop
            )
            self.StatusLabel[index] = statusLabel

        self.ProgressWidget.setLayout(self.ProgressLayout)

        if self.master.expertMode:
            self.OutputTree = QTreeWidget()
            self.OutputTree.horizontalScrollBar().setEnabled(True)
            self.OutputTree.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
            self.OutputTree.setHeaderLabels(["Name"])
            self.OutputTree.itemClicked.connect(self.onItemClicked)
            self.TreeRoot = QTreeWidgetItem(self.OutputTree)
            self.TreeRoot.setText(0, "Files..")
        else:
            self.TestLabel = QLabel("Test")
            self.ControlButtom = QPushButton("Pause")
            self.ControlButtom.clicked.connect(self.controlDisplay)
            self.SVGWidget = QtSvg.QSvgWidget()
            minHeight = 400
            ratio = 1.5
            self.SVGWidget.setMinimumHeight(minHeight)
            self.SVGWidget.setMinimumWidth(minHeight * ratio)

        self.ScrollArea.setWidget(self.ProgressWidget)

        ## Old display: To be removed
        # self.mainLayout.addWidget(self.DisplayTitle,0,0,1,2)
        # self.mainLayout.addWidget(self.DisplayLabel,1,0,1,2)
        # self.mainLayout.addWidget(self.ReferTitle,0,2,1,2)
        # self.mainLayout.addWidget(self.ReferLabel,1,2,1,2)
        # self.mainLayout.addWidget(self.OutputTree,0,4,2,1)

        self.mainLayout.addWidget(self.ScrollArea, 0, 0, 10, 2)
        if self.master.expertMode:
            self.mainLayout.addWidget(self.OutputTree, 0, 2, 10, 2)
        else:
            self.mainLayout.addWidget(self.TestLabel, 0, 2, 1, 2)
            self.mainLayout.addWidget(self.ControlButtom, 0, 4, 1, 1)
            self.mainLayout.addWidget(self.SVGWidget, 1, 2, 9, 3)

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
            print("the test is {0}".format(item.text(0)))
            print("This item is a TCanvas")
            canvas = item.data(0, Qt.UserRole)
            canvasname = str(item.text(0))
            canvasname = canvasname.split(";")[0]
            print("The canvas is {0}".format(canvas))
            self.displayResult(canvas, canvasname)
        elif "svg" in str(item.data(0, Qt.UserRole)):
            canvas = item.data(0, Qt.UserRole)
            self.displayResult(canvas)

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
        if len(self.displayList) == 0:
            return
        else:
            if self.displayIndex == len(self.displayList):
                self.allDisplayed = True
            self.displayIndex = self.displayIndex % len(self.displayList)
            step, displayPlot = self.displayList[self.displayIndex]
            self.TestLabel.setText("Step{}".format(step))
            self.SVGWidget.load(displayPlot)
            self.displayIndex += 1

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

        for File in self.FileList:
            CurrentNode = QTreeWidgetItem()
            CurrentNode.setText(0, File.split("/")[-1])
            CurrentNode.setData(0, Qt.UserRole, File)
            self.TreeRoot.addChild(CurrentNode)
            self.getResult(CurrentNode, File)

    def updateIVResult(self, sourceFolder):
        process2 = subprocess.run(
            'find {0} -type f -name "*.svg" '.format(sourceFolder),
            shell=True,
            stdout=subprocess.PIPE,
        )
        stepFiles2 = process2.stdout.decode("utf-8").rstrip("\n").split("\n")

        if stepFiles2 == [""]:
            return

        self.IVFileList += stepFiles2

        for File in self.IVFileList:
            CurrentNode = QTreeWidgetItem()
            CurrentNode.setText(0, File.split("/")[-1])
            CurrentNode.setData(0, Qt.UserRole, File)
            self.TreeRoot.addChild(CurrentNode)

    def displayResult(self, canvas, name=None):
        print("the name passed was {0}".format(name))
        tmpDir = os.environ.get("GUI_dir") + "/Gui/.tmp"
        if not os.path.isdir(tmpDir) and os.environ.get("GUI_dir"):
            try:
                os.mkdir(tmpDir)
                logger.info("Creating " + tmpDir)
            except:
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
            self.Plot.append("index")
            self.Plot[self.count] = QtTCanvasWidget(self.master, svgFile)
            self.count = self.count + 1
            logger.info("Displaying " + svgFile)
        except:
            logger.error("Failed to display " + svgFile)
        pass
