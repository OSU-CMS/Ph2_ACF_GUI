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
    QProgressBar,
    QPushButton,
    QRadioButton,
    QScrollBar,
    QSizePolicy,
    QSlider,
    QSpinBox,
    QStyleFactory,
    QTableView,
    QTableWidget,
    QTabWidget,
    QTextEdit,
    QHBoxLayout,
    QVBoxLayout,
    QWidget,
    QMainWindow,
    QMessageBox,
    QSplitter,
)

import sys
import os
import subprocess
import hashlib
from subprocess import Popen, PIPE

from Gui.GUIutils.DBConnection import (
    checkDBConnection,
    getLocalRemoteTests,
    retrieveWithConstraint,
    insertGenericTable,
)
from Gui.GUIutils.guiUtils import (
    isActive,
    GetTBrowser,
)
#from Gui.QtGUIutils.QtStartWindow import *
from Gui.QtGUIutils.QtTableWidget import QtTableWidget
from Gui.QtGUIutils.QtLoginDialog import QtLoginDialog
#from Gui.python.ROOTInterface import *
from Gui.python.logging_config import logger


class QtModuleReviewWindow(QWidget):
    def __init__(self, master, info=None):
        super(QtModuleReviewWindow, self).__init__()
        self.master = master
        self.info = info
        self.connection = self.master.connection
        self.GroupBoxSeg = [1, 10, 1]
        # self.columns = ["id","part_id","username","description","testname","data_id","grade","date"]
        # self.columns = ["part_id","test_id","test_name","date","test_grade","user","Chip0InConfig","Chip0OutConfig","Chip1InConfig","Chip1OutConfig","Chip2InConfig","Chip2OutConfig","Chip3InConfig","Chip3OutConfig","plot1","plot2","root_file"]
        self.columns = [
            "part_id",
            "test_id",
            "test_name",
            "date",
            "test_grade",
            "user",
            "plot1",
            "plot2",
            "root_file",
        ]
        # Fixme: QTimer to be added to update the page automatically
        print("The review window got opened")
        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)

        self.setLoginUI()
        self.createHeadLine()
        self.createMain()
        self.createApp()

    def setLoginUI(self):
        self.setGeometry(200, 200, 1000, 1600)
        self.setWindowTitle("Module Review Page")
        self.show()

    def createHeadLine(self):
        self.HeadBox = QGroupBox()

        self.HeadLayout = QHBoxLayout()

        HeadLabel = QLabel('<font size="4"> Module: {0} </font>'.format(self.info))
        HeadLabel.setMaximumHeight(30)

        statusString, colorString = checkDBConnection(self.connection)
        StatusLabel = QLabel()
        StatusLabel.setText(statusString)
        StatusLabel.setStyleSheet(colorString)

        self.ConnectButton = QPushButton("&Connect to DB")
        self.ConnectButton.clicked.connect(self.connectDB)
        if isActive(self.connection):
            self.ConnectButton.setDisabled(True)
        else:
            self.ConnectButton.setDisabled(False)

        self.HeadLayout.addWidget(HeadLabel)
        self.HeadLayout.addStretch(1)
        self.HeadLayout.addWidget(StatusLabel)
        self.HeadLayout.addWidget(self.ConnectButton)

        self.HeadBox.setLayout(self.HeadLayout)

        self.mainLayout.addWidget(self.HeadBox, 0, 0, self.GroupBoxSeg[0], 1)

    def destroyHeadLine(self):
        self.HeadBox.deleteLater()
        self.mainLayout.removeWidget(self.HeadBox)

    def createMain(self):
        self.MainBodyBox = QGroupBox()

        mainbodylayout = QHBoxLayout()

        # Group Box for history
        HistoryBox = QGroupBox("&Record")

        HistoryLayout = QGridLayout()

        dataList = getLocalRemoteTests(self.connection, self.info, self.columns)
        self.proxy = QtTableWidget(dataList)
        self.proxy.filtering.connect(self.addButtons)

        self.lineEdit = QLineEdit()
        self.lineEdit.textChanged.connect(self.proxy.on_lineEdit_textChanged)
        self.view = QTableView()
        self.view.setSortingEnabled(True)
        self.comboBox = QComboBox()
        self.comboBox.addItems(["{0}".format(x) for x in dataList[0]])
        self.comboBox.currentIndexChanged.connect(
            self.proxy.on_comboBox_currentIndexChanged
        )
        self.label = QLabel()
        self.label.setText("Regex Filter")

        self.view.setModel(self.proxy)
        self.view.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.view.setSelectionMode(QAbstractItemView.MultiSelection)
        self.view.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.addButtons()
        # for row in range(len(self.proxy.dataBody)):
        # 	DetailButton = QPushButton("&Show...")
        # 	DetailButton.clicked.connect(lambda state, x="{0}".format(self.proxy.dataBody[row][len(self.proxy.dataHeader)-1]) : self.openDQM(x))
        # 	self.view.setIndexWidget(self.proxy.index(row,0),DetailButton)

        HistoryLayout = QGridLayout()
        HistoryLayout.addWidget(self.lineEdit, 0, 1, 1, 1)
        HistoryLayout.addWidget(self.view, 1, 0, 1, 3)
        HistoryLayout.addWidget(self.comboBox, 0, 2, 1, 1)
        HistoryLayout.addWidget(self.label, 0, 0, 1, 1)

        HistoryBox.setLayout(HistoryLayout)

        mainbodylayout.addWidget(HistoryBox)

        self.MainBodyBox.setLayout(mainbodylayout)
        self.mainLayout.addWidget(
            self.MainBodyBox, sum(self.GroupBoxSeg[0:1]), 0, self.GroupBoxSeg[1], 1
        )

    def addButtons(self):
        for row in range(self.proxy.rowCount()):
            DetailButton = QPushButton("&Show...")
            DetailButton.clicked.connect(
                lambda state, x="{0}".format(
                    self.proxy.data(self.proxy.index(row, len(self.proxy.dataHeader)))
                ): self.openDQM(x)
            )
            self.view.setIndexWidget(self.proxy.index(row, 0), DetailButton)

    def destroyMain(self):
        self.MainBodyBox.deleteLater()
        self.mainLayout.removeWidget(self.MainBodyBox)

    def createApp(self):
        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()

        self.SyncButton = QPushButton("&Sync to DB")
        self.SyncButton.clicked.connect(self.syncDB)
        if not isActive(self.connection):
            self.SyncButton.setDisabled(True)

        self.ResetButton = QPushButton("&Reset")
        self.ResetButton.clicked.connect(self.destroyMain)
        self.ResetButton.clicked.connect(self.createMain)

        self.FinishButton = QPushButton("&Finish")
        self.FinishButton.setDefault(True)
        self.FinishButton.clicked.connect(self.closeWindow)

        self.StartLayout.addStretch(1)
        self.StartLayout.addWidget(self.SyncButton)
        self.StartLayout.addWidget(self.ResetButton)
        self.StartLayout.addWidget(self.FinishButton)
        self.AppOption.setLayout(self.StartLayout)

        self.mainLayout.addWidget(
            self.AppOption, sum(self.GroupBoxSeg[0:2]), 0, self.GroupBoxSeg[2], 1
        )

    def destroyApp(self):
        self.AppOption.deleteLater()
        self.mainLayout.removeWidget(self.AppOption)

    def closeWindow(self):
        self.close()

    def sendBackSignal(self):
        self.backSignal = True

    def connectDB(self):
        if isActive(self.master.connection):
            self.connection = self.master.connection
            self.refresh()
            return

        LoginDialog = QtLoginDialog(self.master)
        response = LoginDialog.exec_()
        if response == QDialog.Accepted:
            self.connectDB()
        else:
            return

    def openDQM(self, DQMFile):
        print("Open" + DQMFile)
        GetTBrowser(DQMFile)
        print("Close" + DQMFile)

    def syncDB(self):
        print("syncDB button was pushed!")
        if not isActive(self.connection):
            return
        print("syncDB is trying to do a thing")
        selectedrows = self.view.selectionModel().selectedRows()
        print("the selected rows are {0}".format(selectedrows))
        for index in selectedrows:
            try:
                rowNumber = index.row()
                if self.proxy.data(self.proxy.index(rowNumber, 1)) != "Local":
                    print("This record is verified to be Non-local")
                    continue

                if self.proxy.data(self.proxy.index(rowNumber, 1)) == "Local":
                    print("trying to send local stuff to DB")
                    ################################
                    ##  Block to get binary Info
                    ################################
                    # localDir = self.proxy.data(self.proxy.index(rowNumber,len(self.proxy.dataHeader)))
                    localDir = self.proxy.data(
                        self.proxy.index(rowNumber, 8)
                    )  # This is a temporary hack!!!
                    print("local dir is {0}".format(localDir))
                    if localDir != "":
                        print("Local Directory found in : {}".format(localDir))

                    getFiles = subprocess.run(
                        'find {0} -mindepth 1  -maxdepth 1 -type f -name "*.root"  '.format(
                            localDir
                        ),
                        shell=True,
                        stdout=subprocess.PIPE,
                    )
                    fileList = getFiles.stdout.decode("utf-8").rstrip("\n").split("\n")
                    print("the filelist is {0}".format(fileList))
                    if fileList == [""]:
                        logger.warning(
                            "No ROOT file found in the local folder, skipping the record..."
                        )
                        continue

                    module_id = self.proxy.data(
                        self.proxy.index(rowNumber, self.columns.index("part_id") + 2)
                    )
                    for submitFile in fileList:
                        print("Submitting  {}".format(submitFile))
                        data_id = hashlib.md5(
                            "{}".format(submitFile).encode()
                        ).hexdigest()
                        if not self.checkRemoteFile(data_id):
                            self.uploadFile(submitFile, data_id)

                        getConfigInFiles = subprocess.run(
                            'find {0} -mindepth 1  -maxdepth 1 -type f -name "CMSIT_RD53_{1}_*_IN.txt"  '.format(
                                localDir, module_id
                            ),
                            shell=True,
                            stdout=subprocess.PIPE,
                        )
                        configInFileList = (
                            getConfigInFiles.stdout.decode("utf-8")
                            .rstrip("\n")
                            .split("\n")
                        )
                        getConfigOutFiles = subprocess.run(
                            'find {0} -mindepth 1  -maxdepth 1 -type f -name "CMSIT_RD53_{1}_*_OUT.txt"  '.format(
                                localDir, module_id
                            ),
                            shell=True,
                            stdout=subprocess.PIPE,
                        )
                        configOutFileList = (
                            getConfigOutFiles.stdout.decode("utf-8")
                            .rstrip("\n")
                            .split("\n")
                        )
                        getXMLFiles = subprocess.run(
                            'find {0} -mindepth 1  -maxdepth 1 -type f -name "*.xml"  '.format(
                                localDir
                            ),
                            shell=True,
                            stdout=subprocess.PIPE,
                        )
                        XMLFileList = (
                            getXMLFiles.stdout.decode("utf-8").rstrip("\n").split("\n")
                        )
                        configcolumns = []
                        configdata = []
                        for configInFile in configInFileList:
                            print("config files are: {0}".format(configInFile))
                            if configInFile != [""]:
                                configcolumns.append(
                                    "Chip{}InConfig".format(configInFile.split("_")[-2])
                                )
                                configInBuffer = open(configInFile, "rb")
                                configInBin = configInBuffer.read()
                                configdata.append(configInBin)
                        for configOutFile in configOutFileList:
                            if configOutFile != [""]:
                                configcolumns.append(
                                    "Chip{}OutConfig".format(
                                        configOutFile.split("_")[-2]
                                    )
                                )
                                configOutBuffer = open(configOutFile, "rb")
                                configOutBin = configOutBuffer.read()
                                configdata.append(configOutBin)
                        xmlcolumns = []
                        xmldata = []
                        if len(XMLFileList) > 1:
                            print("Warning!  There are multiple xml files here!")
                        for XMLFile in XMLFileList:
                            if XMLFile != [""]:
                                xmlcolumns.append("xml_file")
                                xmlBuffer = open(XMLFile, "rb")
                                xmlBin = xmlBuffer.read()
                                xmldata.append(xmlBin)

                        SubmitArgs = []
                        Value = []
                        for column in self.columns:
                            if (
                                column == "part_id"
                                or column == "date"
                                or column == "test_name"
                            ):
                                SubmitArgs.append(column)
                                Value.append(
                                    self.proxy.data(
                                        self.proxy.index(
                                            rowNumber, self.columns.index(column) + 2
                                        )
                                    )
                                )
                            if column == "description":
                                SubmitArgs.append(column)
                                Value.append("No Comment")
                            if column == "test_grade":
                                SubmitArgs.append(column)
                                Value.append(
                                    self.proxy.data(
                                        self.proxy.index(
                                            rowNumber, self.columns.index(column) + 2
                                        )
                                    )
                                )
                            if column == "test_id":
                                SubmitArgs.append(column)
                                Value.append(data_id)
                            if column == "user":
                                SubmitArgs.append(column)
                                Value.append(self.master.TryUsername)
                            if column == "root_file":
                                SubmitArgs.append(column)
                                Value.append(submitFile.split("/")[-1])

                        SubmitArgs = SubmitArgs + configcolumns + xmlcolumns
                        Value = Value + configdata + xmldata

                        try:
                            insertGenericTable(
                                self.connection, "module_tests", SubmitArgs, Value
                            )
                            print("trying to insert table")
                        except:
                            print("Failed to insert")
            except Exception as err:
                print("Error: {}".format(repr(err)))

        self.destroyMain()
        self.createMain()

    def uploadFile(self, fileName, file_id):
        fileBuffer = open(fileName, "rb")
        data = fileBuffer.read()
        insertGenericTable(
            self.connection,
            "result_files",
            ["file_id", "file_content"],
            [file_id, data],
        )

    def checkRemoteFile(self, file_id):
        remoteRecords = retrieveWithConstraint(
            self.connection, "result_files", file_id=file_id, columns=["file_id"]
        )
        return remoteRecords != []

    def refresh(self):
        self.destroyHeadLine()
        self.destroyMain()
        self.destroyApp()
        self.createHeadLine()
        self.createMain()
        self.createApp()
