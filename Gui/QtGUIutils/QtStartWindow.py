import os
import math
import subprocess
import logging
import requests
import re

from PyQt5.QtCore import QSize, Qt, pyqtSignal
from PyQt5.QtGui import QPixmap, QImage
from PyQt5.QtWidgets import (
    QComboBox,
    QGridLayout,
    QGroupBox,
    QLabel,
    QPushButton,
    QHBoxLayout,
    QVBoxLayout,
    QWidget,
    QMessageBox,
    QLineEdit,
    QRadioButton
)
from Gui.QtGUIutils.Loading import LoadingThread
from Gui.QtGUIutils.QtFwCheckDetails import QtFwCheckDetails
from Gui.python.CustomizedWidget import BeBoardBox
from Gui.GUIutils.FirmwareUtil import FEPowerUpVD
from Gui.GUIutils.settings import firmware_image, ModuleLaneMap
from Gui.siteSettings import (
    FC7List,
    ModuleCurrentMap,
)

from InnerTrackerTests.TestSequences import TestList
from siteSettings import icicle_instrument_setup


# Customize the logging configuration
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    filename="my_project.log",  # Specify a log file
    filemode="w",  # 'w' for write, 'a' for append
)

logger = logging.getLogger(__name__)


# from Gui.QtGUIutils.QtApplication import *

# from Gui.python.Firmware import *
# from Gui.GUIutils.DBConnection import *

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
        if icicle_instrument_setup is not None:
            self.measureFwPar()
        # self.checkFwPar()
        self.setLayout(self.mainLayout)

    def initResult(self):
        for i in ModuleLaneMap[self.module.getType()].keys():
            self.verboseResult[i] = {}
            self.chipSwitches[i] = True

    def measureFwPar(self):
        for index, (key, value) in enumerate(self.verboseResult.items()):
            value["Power-up Mode"] = "SLDO" #self.PowerModeCombo.currentText()
            # Fixme
            measureList = [
                "Set Bias Voltage (V)",
                "Set LV Current (A)",
            ]
            for item in measureList:
                if "LV Current" in item:
                    value[item] = ModuleCurrentMap[self.module.getType()]
                if "Bias Voltage" in item:
                    value[item] = icicle_instrument_setup["instrument_dict"]["hv"][
                        "default_voltage"
                    ]
                    # assumes only 1 HV titled 'hv' in instruments.json
            self.verboseResult[key] = value

    @staticmethod
    def checkFwPar(pfirmwareName, module_type, fc7_ip):
        # To be finished
        try:
            # self.result = True
            FWisPresent = False
            boardtype = "RD53B"
            if "CROC" in module_type:
                boardtype = "RD53B"
            else:
                boardtype = "RD53A"
            print("board type is: {0}".format(boardtype))
            # updating uri value in template xml file with correct fc7 ip address, as specified in siteSettings.py
            # fc7_ip = site_settings.FC7List[pfirmwareName] #Commented because I don't think we need it.  Remove line after test.
            print("The fc7 ip is: {0}".format(fc7_ip))
            uricmd = "sed -i -e 's/fc7-1/{0}/g' {1}/Gui/CMSIT_{2}.xml".format(
                fc7_ip, os.environ.get("GUI_dir"), boardtype
            )
            subprocess.call([uricmd], shell=True)
            print("updated the uri value")
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
                            os.environ.get("GUI_dir")
                            + "/Gui/CMSIT_{}.xml".format(boardtype),
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
                except OSError:
                    print(
                        "unable to save {0} to FC7 SD card".format(
                            os.environ.get("GUI_dir")
                            + "/FirmwareImages/"
                            + firmwareImage
                        )
                    )

            if FWisPresent:
                print("Loading FW image")
                fwload = subprocess.run(
                    [
                        "fpgaconfig",
                        "-c",
                        os.environ.get("GUI_dir")
                        + "/Gui/CMSIT_{}.xml".format(boardtype),
                        "-i",
                        "{}".format(firmwareImage),
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                print(fwload.stdout.decode("UTF-8"))
                print("resetting beboard")
                print(
                    f"command: CMSITminiDAQ -f {os.environ.get('GUI_dir') + '/Gui/CMSIT_{}.xml'.format(boardtype)} -r"
                )
                fwreset = subprocess.run(
                    [
                        "CMSITminiDAQ",
                        "-f",
                        os.environ.get("GUI_dir")
                        + "/Gui/CMSIT_{}.xml".format(boardtype),
                        "-r",
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                print(fwreset.stdout.decode("UTF-8"))
                print(fwreset.stderr.decode("UTF-8"))

                print("Firmware image is now loaded")
            logging.debug("Made it to turn on LV")
            return True
        except Exception as err:
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
    onThreadFinishSignal = pyqtSignal()
    loaderSignal = pyqtSignal()
    openRunWindowSignal = pyqtSignal()
    def __init__(self, master, firmware):
        super(QtStartWindow, self).__init__()
        self.master = master
        self.firmware = firmware
        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)
        self.runFlag = False
        self.passCheck = False
        self.setLoginUI()
        self.createHead()
        self.createMain()
        self.createApp()
        self.occupied()

        self.closeFlag = False
        self.loading_counter = 0
        self.loaderSignal.connect(self.loader)
        self.onThreadFinishSignal.connect(self.onThreadFinish)
        self.openRunWindowSignal.connect(self.openRunWindow)

    def setLoginUI(self):
        self.setGeometry(400, 400, 400, 400)
        self.setWindowTitle("Start a new test")
        self.show()

    def createHead(self):
        self.TestBox = QGroupBox()
        testlayout = QGridLayout()
        TestLabel = QLabel("Test:")
        self.TestCombo = QComboBox()
        # self.TestList = getAllTests(self.master.connection)
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

        for beboard in self.firmware:
            beboard.removeModules()
            beboard.removeAllOpticalGroups()

        self.BeBoardWidget = BeBoardBox(self.master, self.firmware)  # FLAG
        self.mainLayout.addWidget(self.TestBox, 0, 0, 1, 2)
        self.mainLayout.addWidget(self.BeBoardWidget, 1, 0, 1, 2)

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

        self.txt_box = QGroupBox()
        main_txt_layout = QVBoxLayout()  # vertical: text field on top, radio buttons below

        # Text input
        self.txt_entry = QLineEdit()
        self.txt_entry.setPlaceholderText("Panthera .txt's (panthera.fit.edu/panthera_storage/results/ModuleID**/SequenceID***/ResultID****/)")
        self.txt_entry.editingFinished.connect(lambda:self.change_chip_txts(self.txt_entry.text().replace(" ", "")))

        main_txt_layout.addWidget(self.txt_entry)

        # Radio buttons: OUT and IN side by side
        radio_layout = QHBoxLayout()
        self.out_radio = QRadioButton("OUT")
        self.in_radio = QRadioButton("IN")
        self.out_radio.setLayoutDirection(Qt.RightToLeft)
        self.in_radio.setLayoutDirection(Qt.RightToLeft)
        self.out_radio.setChecked(True)  # OUT selected by default

        radio_layout.addWidget(self.out_radio, alignment=Qt.AlignTop)
        radio_layout.addWidget(self.in_radio, alignment=Qt.AlignTop)
        radio_layout.addStretch()

        main_txt_layout.addLayout(radio_layout)
        self.txt_box.setLayout(main_txt_layout)

        self.mainLayout.addWidget(self.txt_box, 2, 0, 1, 1)
        self.mainLayout.addWidget(self.firmwareCheckBox, 2, 1, 1, 1)

        for module in self.BeBoardWidget.getModules():
            module.SerialEdit.editingFinished.connect(self.txt_entry.clear)

    def change_chip_txts(self, url):
        links = {}
        if url != "":
            erroredFlag=False

            pantheraURL=False
            idx = url.find("ResultID")
            if "panthera.fit.edu" in url and idx != -1:
                i = idx+len("ResultID")
                while i < len(url) and url[i].isdigit():
                    i += 1
                url = url[:i+1]
                if url[-1]!='/': url = url+'/'
                pantheraURL=True

            outOrIn = 'OUT' if self.out_radio.isChecked() else 'IN'
            for moduleBox in self.BeBoardWidget.getModules():
                moduleName = moduleBox.getSerialNumber()
                moduleId = moduleBox.getFMCPort()
                for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                    if pantheraURL:
                        pantheraFile = re.sub(r'_+', '_', url+"CMSIT_RD53_{0}_{1}_0_{2}_{3}.txt".format(
                                moduleName, moduleId, chipid, outOrIn
                            )) #The re function replaces any instance of multiple underscores with just one.
                    else:
                        pantheraFile = url
                    
                    #Check if the Panthera file actually exists.
                    print(erroredFlag)
                    if erroredFlag==False:
                        try:
                            response = requests.head(pantheraFile, allow_redirects=True)  # or .get() if you need content
                            if response.status_code == 404:
                                print("Panthera file doesn't exist: "+pantheraFile)
                                self.master.errorMessageBoxSignal.emit("One or more of the Panthera chip txt pages don't exist!")
                                erroredFlag=True
                                pantheraFile = ""
                        except requests.exceptions.RequestException as e:
                            logger.error("Error checking the page: ", e)

                    links[moduleName, chipid] = pantheraFile

        
        #Do this at the end so that there's no autofill unless all files pass the check above.
        for moduleBox in self.BeBoardWidget.getModules():
            moduleName = moduleBox.getSerialNumber()
            for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                chiptxt = self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0).widget()
                if links!={}:
                    chiptxt.setText(links[moduleName, chipid])
                else:
                    chiptxt.setText("")

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
        self.NextButton.clicked.connect(self.openRunWindow_starter)

        self.StartLayout.addStretch(1)
        self.StartLayout.addWidget(self.CancelButton)
        # self.StartLayout.addWidget(self.ResetButton)
        # self.StartLayout.addWidget(self.CheckButton)
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

        self.mainLayout.addWidget(self.AppOption, 3, 0, 1, 2)
        self.mainLayout.addWidget(self.LogoGroupBox, 4, 0, 1, 2)

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
            # item.checkFwPar(pfirmwareName, item.module.getType())
            GlobalCheck = GlobalCheck and item.checkFwPar(
                pfirmwareName, item.module.getType(), FC7List[pfirmwareName]
            )
            # GlobalCheck = GlobalCheck and item.getResult()
        self.passCheck = GlobalCheck
        return GlobalCheck

    def setupBeBoard(self):
        # Setup the BeBoard
        pass

    def loader(self):
        self.NextButton.setText(". " * (self.loading_counter + 1))
        self.loading_counter = (self.loading_counter + 1) % 3

    def onThreadFinish(self):
        if self.closeFlag:
            self.close()
        else:
            self.NextButton.setText("&Next")
            self.NextButton.setDisabled(False)

    def openRunWindow_starter(self):
        self.NextButton.setDisabled(True)
        self.NextButton.setDisabled(True)
        self.NextButton.setText(". . .")
        self.run_window_thread = LoadingThread(self.openRunWindow, 500)
        self.run_window_thread.finished.connect(self.onThreadFinishSignal)
        self.run_window_thread.timer.timeout.connect(self.loaderSignal)
        self.run_window_thread.timer.start()
        self.run_window_thread.start()

    def openRunWindow(self):
        # if not os.access(os.environ.get('GUI_dir'),os.W_OK):
        # 	QMessageBox.warning(None, "Error",'write access to GUI_dir is {0}'.format(os.access(os.environ.get('GUI_dir'),os.W_OK)), QMessageBox.Ok)
        # 	return
        # if not os.access("{0}/test".format(os.environ.get('PH2ACF_BASE_DIR')),os.W_OK):
        # 	QMessageBox.warning(None, "Error",'write access to Ph2_ACF is {0}'.format(os.access(os.environ.get('PH2ACF_BASE_DIR'),os.W_OK)), QMessageBox.Ok)
        # 	return
        
        files = {} #Maybe this block could be combined with change_chip_txts.
        for moduleBox in self.BeBoardWidget.getModules():
            moduleName = moduleBox.getSerialNumber()
            for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                text = self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0).widget().text().replace(" ", "")
                if text != "":
                    if 'panthera.fit.edu' in text:
                        IDsIndex = text.find("panthera_storage/results/")
                        destination = os.environ.get("PH2ACF_BASE_DIR") + "/settings/RD53Files/" + text[IDsIndex+25:]
                        try:
                            os.system(f"wget -O {destination} {text}")
                            files[moduleName, chipid] = destination
                        except:
                            self.master.errorMessageBoxSignal.emit("Could not get file from Panthera!")
                            return
                    elif os.path.exists(text):
                        files[moduleName, chipid] = text
                    else:
                        self.master.errorMessageBoxSignal.emit("The chip txt file doesn't exist!")

        
        for moduleBox in self.BeBoardWidget.getModules():
            moduleName = moduleBox.getSerialNumber()
            for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                if (moduleName, chipid) in files.keys():
                    self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0).widget().setText(files[moduleName, chipid])

        # NOTE This is not the best way to do this, we should be emitting a signal to change
        # the module type but ModuleBox is not publically accessible so we have to go through BeBoardWidget
        self.master.module_in_use = self.BeBoardWidget.getModules()[0].getType()

        for module in self.BeBoardWidget.getModules():
            if module.getSerialNumber() == "":
                self.master.errorMessageBoxSignal.emit(
                    "No valid serial number!",
                )  # Needs to be in a signal or QThread throws an error
                return
            if module.getFMCPort() == "":
                self.master.errorMessageBoxSignal.emit("No valid ID!")
                return

        self.firmwareDescription, message = self.BeBoardWidget.getFirmwareDescription()

        if (
            not self.firmwareDescription
        ):  # firmware description returns none if no modules are entered
            self.master.errorMessageBoxSignal.emit(message)
            return

        for fw in self.firmwareDescription:
            self.checkFwPar(fw.getBoardName())
        if not self.passCheck:
            reply = QMessageBox().question(  # For some reason this isn't an issue for QThread
                None,
                "Error",
                "Front-End parameter check failed, forced to continue?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No,
            )
            if reply == QMessageBox.No:
                return

        for beboard in self.firmwareDescription:
            print(beboard)

        self.info = self.TestCombo.currentText()

        self.runFlag = True
        self.master.BeBoardWidget = self.BeBoardWidget

        self.master.openRunWindowSignal.emit(self.info, self.firmwareDescription, files)

        self.closeFlag = True

    def closeEvent(self, event):
        if self.runFlag:
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
                        self.master.instruments.off(hv_delay=0.5, hv_step_size=10)

                        print("Window closed")
                    else:
                        logger.info(
                            " You are running in manual mode."
                            " You must turn off powers supplies yourself."
                        )
                except Exception as e:
                    print(
                        "Waring: Incident detected while trying to turn of power supply, please check power status"
                    )
                    logger.error(e)
            else:
                event.ignore()
