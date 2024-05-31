import logging

# Customize the logging configuration
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    filename="my_project.log",  # Specify a log file
    filemode="w",  # 'w' for write, 'a' for append
)

logger = logging.getLogger(__name__)

from PyQt5.QtCore import *
from PyQt5.QtWidgets import (
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
    QTableWidget,
    QTabWidget,
    QTextEdit,
    QHBoxLayout,
    QVBoxLayout,
    QWidget,
    QMainWindow,
    QMessageBox,
)


import sys
import os
import math
import subprocess
import time

from Gui.QtGUIutils.QtRunWindow import QtRunWindow
from Gui.QtGUIutils.QtFwCheckDetails import QtFwCheckDetails
#from Gui.QtGUIutils.QtApplication import *
from Gui.python.CustomizedWidget import BeBoardBox
#from Gui.python.Firmware import *
#from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.FirmwareUtil import FEPowerUpVD
from Gui.GUIutils.settings import firmware_image, ModuleLaneMap
from Gui.siteSettings import (
    FC7List,
    ModuleCurrentMap,
)
from InnerTrackerTests.TestSequences import TestList
from siteSettings import icicle_instrument_setup

# from Gui.QtGUIutils.QtProductionTestWindow import *


class SummaryBox(QWidget):
    def __init__(self, master, module, index=0):
        super(SummaryBox, self).__init__()
        self.module = module
        self.master = master
        self.index = index
        self.result = False
        self.verboseResult = {}
        self.chipSwitches = {}

        self.mainLayout = QGridLayout()
       
        self.initResult()
        self.createBody()
        if icicle_instrument_setup is not None:
            self.measureFwPar()
        # self.checkFwPar()
        self.setLayout(self.mainLayout)

    def initResult(self):
        for i in ModuleLaneMap[self.module.getType()].keys():
            self.verboseResult[i] = {}
            self.chipSwitches[i] = True

    def createBody(self):
        FEIDLabel = QLabel("Module: {}".format(self.module.getSerialNumber()))
        FEIDLabel.setStyleSheet("font-weight:bold")
        PowerModeLabel = QLabel()
        PowerModeLabel.setText("FE Power Mode:")
        self.PowerModeCombo = QComboBox()
        self.PowerModeCombo.addItems(FEPowerUpVD.keys())
        # self.PowerModeCombo.currentTextChanged.connect(self.checkFwPar)
        # self.ChipBoxWidget = ChipBox(self.module.getType())

        self.CheckLabel = QLabel()
        
        self.mainLayout.addWidget(PowerModeLabel, 1, 0, 1, 1)
        self.mainLayout.addWidget(self.PowerModeCombo, 1, 1, 1, 1)
        self.mainLayout.addWidget(self.CheckLabel, 2, 0, 1, 1)

    def measureFwPar(self):
        for index, (key, value) in enumerate(self.verboseResult.items()):
            value["Power-up Mode"] = self.PowerModeCombo.currentText()
            # Fixme
            measureList = [
                "Set Bias Voltage (V)",
                "Set LV Current (A)",
            ]
            for item in measureList:
                if 'LV Current' in item:
                    value[item] = ModuleCurrentMap[self.module.getType()]
                if 'Bias Voltage' in item:
                    value[item] = icicle_instrument_setup['default_hv_voltage']
            self.verboseResult[key] = value

    @staticmethod
    def checkFwPar(pfirmwareName, module_type):
        # To be finished

        try:
            #self.result = True
            FWisPresent = False
            if "CROC" in module_type:
                boardtype = "RD53B"
            else:
                boardtype = "RD53A"

            # updating uri value in template xml file with correct fc7 ip address, as specified in siteSettings.py
            fc7_ip = FC7List[pfirmwareName]
            
            uricmd = "sed -i -e 's/fc7-1/{0}/g' {1}/Gui/CMSIT_{2}.xml".format(    
                fc7_ip, os.environ.get("GUI_dir"), boardtype
            )
            updateuri = subprocess.call([uricmd], shell=True)

            firmwareImage = firmware_image[module_type][
                os.environ.get("Ph2_ACF_VERSION")
            ]
            print("checking if firmware is on the SD card for {}".format(firmwareImage))
            fwlist = subprocess.run(
                [
                    "fpgaconfig",
                    "-c",
                    os.environ.get("GUI_dir") + "/Gui/CMSIT_{}.xml".format(boardtype),
                    "-l",
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

            # fwlist = subprocess.run(["fpgaconfig","-c",os.environ.get('PH2ACF_BASE_DIR')+'/test/CMSIT_{}.xml'.format(boardtype),"-l"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            print("firmwarelist is {0}".format(fwlist.stdout.decode("UTF-8")))
            print("firmwareImage is {0}".format(firmwareImage))
            if firmwareImage in fwlist.stdout.decode("UTF-8"):
                FWisPresent = True
                print("firmware found")
            else:
                try:
                    print(
                        "Saving fw image {0} to SD card".format(
                            os.environ.get("GUI_dir")
                            + "/FirmwareImages/"
                            + firmwareImage
                        )
                    )
                    fwsave = subprocess.run(
                        [
                            "fpgaconfig",
                            "-c",
                            os.environ.get("GUI_dir") + "/Gui/CMSIT_{}.xml".format(boardtype),
                            "-f",
                            "{}".format(
                                os.environ.get("GUI_dir")
                                + "/FirmwareImages/"
                                + firmwareImage
                            ),
                            "-i",
                            "{}".format(firmwareImage),
                        ],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                    )
                    # self.fw_process.start("fpgaconfig",["-c","CMSIT.xml","-f","{}".format(os.environ.get("GUI_dir")+'/FirmwareImages/' + self.firmwareImage),"-i","{}".format(self.firmwareImage)])
                    print(fwsave.stdout.decode("UTF-8"))
                    FWisPresent = True
                except:
                    print(
                        "unable to save {0} to FC7 SD card".format(
                            os.environ.get("GUI_dir")
                            + "/FirmwareImages/"
                            + self.firmwareImage
                        )
                    )

            if FWisPresent:
                print("Loading FW image")
                fwload = subprocess.run(
                    [
                        "fpgaconfig",
                        "-c",
                        os.environ.get("GUI_dir") + "/Gui/CMSIT_{}.xml".format(boardtype),
                        "-i",
                        "{}".format(firmwareImage),
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                print(fwload.stdout.decode("UTF-8"))
                print("resetting beboard")
                fwreset = subprocess.run(
                    ["CMSITminiDAQ", "-f", os.environ.get("GUI_dir") + "/Gui/CMSIT_{}.xml".format(boardtype), "-r"],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                print(fwreset.stdout.decode("UTF-8"))
                print(fwreset.stderr.decode("UTF-8"))

                print("Firmware image is now loaded")
            logging.debug("Made it to turn on LV")
            return True
        except Exception as err:
            #self.result = False
            #self.CheckLabel.setText("No measurement")
            #self.CheckLabel.setStyleSheet("color:red")
            print(err)
            return False

    def getDetails(self):
        pass

    def showDetails(self):
        self.measureFwPar()
        self.occupied()
        self.DetailPage = QtFwCheckDetails(self)
        self.DetailPage.closedSignal.connect(self.release)

    def getResult(self):
        return self.result

    def occupied(self):
        self.DetailsButton.setDisabled(True)

    def release(self):
        self.DetailsButton.setDisabled(False)


class QtStartWindow(QWidget):
    def __init__(self, master, firmware):
        super(QtStartWindow, self).__init__()
        self.master = master
        self.firmware = firmware
        self.firmwares = firmware
        self.firmwareName = firmware.getBoardName()
        self.connection = self.master.connection
        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)
        self.runFlag = False
        self.passCheck = False
        self.setLoginUI()
        self.createHead()
        self.createMain()
        self.createApp()
        self.occupied()

    def setLoginUI(self):
        self.setGeometry(400, 400, 400, 400)
        self.setWindowTitle("Start a new test")
        self.show()

    def createHead(self):
        self.TestBox = QGroupBox()
        testlayout = QGridLayout()
        TestLabel = QLabel("Test:")
        self.TestCombo = QComboBox()
        #self.TestList = getAllTests(self.master.connection)
        self.TestList = TestList
        if not self.master.instruments:
            if "AllScan" in self.TestList:
                self.TestList.remove("AllScan")
            if "QuickTest" in self.TestList:
                self.TestList.remove("QuickTest")
            if "FullSequence" in self.TestList:
                self.TestList.remove("FullSequence")

        self.TestCombo.addItems(self.TestList)
        TestLabel.setBuddy(self.TestCombo)

        testlayout.addWidget(TestLabel, 0, 0, 1, 1)
        testlayout.addWidget(self.TestCombo, 0, 1, 1, 1)
        self.TestBox.setLayout(testlayout)

        self.firmware.removeAllModule()

        self.BeBoardWidget = BeBoardBox(self.master,self.firmware)  # FLAG



        self.mainLayout.addWidget(self.TestBox, 0, 0)
        self.mainLayout.addWidget(self.BeBoardWidget, 1, 0)

    def createMain(self):
        self.firmwareCheckBox = QGroupBox()
        firmwarePar = QGridLayout()
        ## To be finished
        self.ModuleList = []
        for i, module in enumerate(self.BeBoardWidget.ModuleList):
            ModuleSummaryBox = SummaryBox(master=self.master, module=module)
            self.ModuleList.append(ModuleSummaryBox)
            firmwarePar.addWidget(
                ModuleSummaryBox, math.floor(i / 2), math.ceil(i % 2 / 2), 1, 1
            )
        self.BeBoardWidget.updateList()  ############FIXME:  This may not work for multiple modules at a time.
        self.firmwareCheckBox.setLayout(firmwarePar)

        self.mainLayout.addWidget(self.firmwareCheckBox, 2, 0)

    def destroyMain(self):
        self.firmwareCheckBox.deleteLater()
        self.mainLayout.removeWidget(self.firmwareCheckBox)

    def createApp(self):
        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()

        self.CancelButton = QPushButton("&Cancel")
        self.CancelButton.clicked.connect(self.release)
        self.CancelButton.clicked.connect(self.closeWindow)

        self.ResetButton = QPushButton("&Reset")
        self.ResetButton.clicked.connect(self.destroyMain)
        self.ResetButton.clicked.connect(self.createMain)

        self.CheckButton = QPushButton("&Check")
        self.CheckButton.clicked.connect(self.checkFwPar)

        self.NextButton = QPushButton("&Next")
        self.NextButton.setDefault(True)
        # self.NextButton.setDisabled(True)
        self.NextButton.clicked.connect(self.openRunWindow)

        self.StartLayout.addStretch(1)
        self.StartLayout.addWidget(self.CancelButton)
        #self.StartLayout.addWidget(self.ResetButton)
        #self.StartLayout.addWidget(self.CheckButton)
        self.StartLayout.addWidget(self.NextButton)
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

        # self.mainLayout.addWidget(self.LoginGroupBox, 0, 0)
        # self.mainLayout.addWidget(self.LogoGroupBox, 1, 0)

        self.mainLayout.addWidget(self.AppOption, 3, 0)
        self.mainLayout.addWidget(self.LogoGroupBox, 4, 0)

    def closeWindow(self):
        self.close()

    def occupied(self):
        self.master.ProcessingTest = True

    def release(self):
        self.master.ProcessingTest = False
        self.master.NewTestButton.setDisabled(False)
        self.master.LogoutButton.setDisabled(False)
        self.master.ExitButton.setDisabled(False)

    def checkFwPar(self, pfirmwareName):
        GlobalCheck = True
        for item in self.ModuleList:
            #item.checkFwPar(pfirmwareName, item.module.getType())
            GlobalCheck = GlobalCheck and item.checkFwPar(pfirmwareName, item.module.getType())
            #GlobalCheck = GlobalCheck and item.getResult()
        self.passCheck = GlobalCheck
        return GlobalCheck

    def setupBeBoard(self):
        # Setup the BeBoard
        pass

    def openRunWindow(self):
        # if not os.access(os.environ.get('GUI_dir'),os.W_OK):
        # 	QMessageBox.warning(None, "Error",'write access to GUI_dir is {0}'.format(os.access(os.environ.get('GUI_dir'),os.W_OK)), QMessageBox.Ok)
        # 	return
        # if not os.access("{0}/test".format(os.environ.get('PH2ACF_BASE_DIR')),os.W_OK):
        # 	QMessageBox.warning(None, "Error",'write access to Ph2_ACF is {0}'.format(os.access(os.environ.get('PH2ACF_BASE_DIR'),os.W_OK)), QMessageBox.Ok)
        # 	return

        # NOTE This is not the best way to do this, we should be emitting a signal to change
        # the module type but ModuleBox is not publically accessible so we have to go through BeBoardWidget
        self.master.module_in_use = self.BeBoardWidget.getModules()[0].getType()

        for module in self.BeBoardWidget.getModules():
            if module.getSerialNumber() == "":
                QMessageBox.information(
                    None, "Error", "No valid serial number!", QMessageBox.Ok
                )
                return
            if module.getID() == "":
                QMessageBox.information(None, "Error", "No valid ID!", QMessageBox.Ok)
                return
        self.checkFwPar(self.firmwareName)
        if self.passCheck == False:
            reply = QMessageBox().question(
                None,
                "Error",
                "Front-End parameter check failed, forced to continue?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No,
            )
            if reply == QMessageBox.No:
                return

        self.firmwareDescription = self.BeBoardWidget.getFirmwareDescription()
        # self.info = [self.firmware.getModuleByIndex(0).getModuleID(), str(self.TestCombo.currentText())]
        self.info = [
            self.firmware.getModuleByIndex(0).getOpticalGroupID(),
            str(self.TestCombo.currentText()),
        ]
        self.runFlag = True
        self.master.BeBoardWidget = self.BeBoardWidget
        self.master.RunNewTest = QtRunWindow(
            self.master, self.info, self.firmwareDescription
        )
        self.close()

    def closeEvent(self, event):
        if self.runFlag == True:
            event.accept()

        else:
            reply = QMessageBox.question(
                self,
                "Window Close",
                "Are you sure you want to quit the test?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No,
            )

            if reply == QMessageBox.Yes:
                event.accept()
                self.release()
                # This line was previosly commented
                try:
                    if self.master.instruments:
                        self.master.instruments.off(
                            lv_channel=None, hv_delay=0.5, hv_step_size=10
                        )

                        print("Window closed")
                    else:
                        logger.info(" You are running in manual mode."
                                    " You must turn off powers supplies yourself.")
                except Exception as e:
                    print(
                        "Waring: Incident detected while trying to turn of power supply, please check power status"
                    )
                    logger.error(e)
            else:
                event.ignore()
