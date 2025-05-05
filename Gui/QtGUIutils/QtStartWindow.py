import os
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
    QCheckBox,
    QWidget,
    QMessageBox,
    QLineEdit,
    QRadioButton
)
from Gui.QtGUIutils.Loading import LoadingThread
from Gui.QtGUIutils.QtFwCheckDetails import QtFwCheckDetails
from Gui.python.CustomizedWidget import BeBoardBox
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
        self.setMinimumWidth(1000)
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
        ## To be finished
        self.ModuleList = []
        for i, module in enumerate(self.BeBoardWidget.ModuleList):
            ModuleSummaryBox = SummaryBox(master=self.master, module=module)
            self.ModuleList.append(ModuleSummaryBox)
        self.BeBoardWidget.updateList()  ############FIXME:  This may not work for multiple modules at a time.

        # Main vertical layout
        self.txt_box = QGroupBox()
        main_txt_layout = QVBoxLayout()

        # Create the top row as a horizontal layout
        top_row_layout = QHBoxLayout()

        # Create info button
        self.info_button = QPushButton("ℹ️")
        self.info_button.setToolTip("More information")
        self.info_button.setFixedSize(30, 30)
        self.info_button.clicked.connect(lambda:
            QMessageBox.information(self, "Info",
            "<ul><li>Log in to https://panthera.fit.edu/</li>"
            "<li>Go to 'Find Modules'</li>"
            "<li>Enter your module in 'Module Name', press Enter, hit 'Search'</li>"
            "<li>Find the .txt you want, right click, choose 'Copy Link Text'</li>"
            "<li>Enter chip .txt's or enter Panthera url to the right to autofill. Leave blank for default.</li>"
            "<li>Chip .txt URLs will look like:<br>"
            "panthera.fit.edu/panthera_storage/results/<br>"
            "ModuleID**/SequenceID***/ResultID****/<br>"
            "CMSIT_RD53_{ModuleName}_{Port #}_{ChipID}_{IN/OUT}.txt</li>"
            "</ul>")
        )

        # Create other widgetsQt.Checked
        self.customTxtLabel = QLabel("Use custom .txt files:")
        self.customTxtCheck = QCheckBox()

        self.customTxtCheck.setChecked(False)
        self.customTxtCheck.stateChanged.connect(lambda state: self.useCustomTxts(state==Qt.Checked))

        self.txt_entry = QLineEdit()
        self.txt_entry.setPlaceholderText("Panthera .txt's (panthera.fit.edu/panthera_storage/results/...)")
        self.txt_entry.editingFinished.connect(lambda: self.change_chip_txts(self.txt_entry.text().replace(" ", "")))

        # Add widgets to the horizontal row
        top_row_layout.addWidget(self.info_button)
        top_row_layout.addWidget(self.customTxtLabel)
        top_row_layout.addWidget(self.customTxtCheck)
        top_row_layout.addWidget(self.txt_entry)

        # Add top row to main layout
        main_txt_layout.addLayout(top_row_layout)

        # Radio buttons
        radio_layout = QHBoxLayout()
        self.out_radio = QRadioButton("OUT")
        self.in_radio = QRadioButton("IN")
        self.out_radio.setLayoutDirection(Qt.RightToLeft)
        self.in_radio.setLayoutDirection(Qt.RightToLeft)
        self.out_radio.setChecked(True)
        self.out_radio.toggled.connect(lambda checked: checked and self.radio_selected(replaceArgs=("_IN.txt", "_OUT.txt") ))
        self.in_radio.toggled.connect(lambda checked: checked and self.radio_selected(replaceArgs=("_OUT.txt", "_IN.txt") ))

        radio_layout.addWidget(self.out_radio)
        radio_layout.addWidget(self.in_radio)
        radio_layout.addStretch()

        main_txt_layout.addLayout(radio_layout)
        self.txt_box.setLayout(main_txt_layout)
        self.mainLayout.addWidget(self.txt_box, 2, 0, 1, 1)

        for module in self.BeBoardWidget.getModules():
            module.SerialEdit.editingFinished.connect(self.txt_entry.clear)

    def radio_selected(self, replaceArgs:tuple):
        erroredFlag = False
        for moduleBox in self.BeBoardWidget.getModules():
            for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                item = self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0)
                if item is not None:
                    chiplineedit = item.widget()
                    if chiplineedit is not None:
                        chiplineedit.setText(chiplineedit.text().replace(*replaceArgs))
                        if "panthera.fit.edu" in chiplineedit.text():
                            try:
                                response = requests.head(chiplineedit.text(), allow_redirects=True)  # or .get() if you need content
                                if response.status_code in (404, 0, 400, 403):
                                    logger.info(f"Panthera file doesn't exist:  {chiplineedit.text()}")
                                    if not erroredFlag:
                                        self.master.errorMessageBoxSignal.emit("One or more of the chip txt pages don't exist!")
                                        erroredFlag=True
                            except requests.exceptions.RequestException as e:
                                logger.error("Error checking the page: ", e)
                                if not erroredFlag:
                                    self.master.errorMessageBoxSignal.emit("Could not access one or more of the chip txt pages!")
                                    erroredFlag=True


    def useCustomTxts(self, state:bool):
        if state:
            for moduleBox in self.BeBoardWidget.getModules():
                for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                    ChipTxtEdit = QLineEdit()
                    ChipTxtEdit.setPlaceholderText("Prebuilt chip .txt file")
                    self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].addWidget(ChipTxtEdit, 1, 0, 1, 7)
        else:
            self.txt_entry.setText("")
            for moduleBox in self.BeBoardWidget.getModules():
                for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                    item = self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0)
                    if item is not None:
                        widget = item.widget()
                        if widget is not None:
                            self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].removeWidget(widget)
                            widget.setParent(None) 
                            widget.deleteLater()  # Optional: safely schedule widget for deletion
        

    def change_chip_txts(self, url:str):
        links = {}
        if url != "":
            if not self.customTxtCheck.isChecked():
                self.customTxtCheck.setChecked(True)
                moduleBox = self.BeBoardWidget.getModules()[0]
                chipid = tuple(self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys())[0]
                if self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0) is None:
                    self.useCustomTxts(True)

            erroredFlag=False
            noModuleInfoFlag=False

            if len(url)< 8 or ("https://"!=url[:8] and "http://"!=url[:7]):
                url = "https://"+url
                self.txt_entry.setText(url)

            pantheraURL=False
            idx = url.find("ResultID")
            if "panthera.fit.edu" in url and idx != -1:
                i = idx+len("ResultID")
                while i < len(url) and url[i].isdigit():
                    i += 1
                url = url[:i+1]
                if url[-1]!='/':
                    url = url+'/'
                self.txt_entry.setText(url)
                pantheraURL=True

            outOrIn = 'OUT' if self.out_radio.isChecked() else 'IN'
            for moduleBox in self.BeBoardWidget.getModules():
                
                moduleName = moduleBox.getSerialNumber()
                port = moduleBox.getFMCPort()
                if moduleName == "" or port == "":
                    if not noModuleInfoFlag:
                        self.master.errorMessageBoxSignal.emit("Please enter a Serial Number and FMC Port to autofill .txt files.")
                        noModuleInfoFlag=True
                    continue

                for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                    if pantheraURL:
                        fileLink = re.sub(r'_+', '_', url+"CMSIT_RD53_{0}_{1}_{2}_{3}.txt".format(
                                moduleName, port, chipid, outOrIn
                            )) #The re function replaces any instance of multiple underscores with just one.
                    else:
                        fileLink = url
                    
                    #Check if the Panthera file actually exists.
                    try:
                        response = requests.head(fileLink, allow_redirects=True)  # or .get() if you need content
                        if response.status_code in (404, 0, 400, 403):
                            logger.info(f"Panthera file doesn't exist:  {fileLink}")
                            if not erroredFlag:
                                self.master.errorMessageBoxSignal.emit("One or more of the chip txt pages don't exist!")
                                erroredFlag=True
                    except requests.exceptions.RequestException as e:
                        logger.error("Error checking the page: ", e)
                        if not erroredFlag:
                            self.master.errorMessageBoxSignal.emit("Could not access one or more of the chip txt pages!")
                            erroredFlag=True

                    links[moduleName, moduleBox.getFMCPort(), chipid] = fileLink
        
        #Do this at the end so that there's no autofill unless all files pass the check above.
        for moduleBox in self.BeBoardWidget.getModules():
            moduleName = moduleBox.getSerialNumber()
            for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                item = self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0)
                if item is not None:
                    chiplineedit = item.widget()
                    if chiplineedit is not None:
                        chiplineedit.setText(links[moduleName, moduleBox.getFMCPort(), chipid])

    def createApp(self):
        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()

        self.CancelButton = QPushButton("&Cancel")
        self.CancelButton.clicked.connect(self.release)
        self.CancelButton.clicked.connect(self.closeWindow)

        self.ResetButton = QPushButton("&Reset")
        self.ResetButton.clicked.connect(self.createMain)

        self.CheckButton = QPushButton("&Check")
        self.CheckButton.clicked.connect(self.checkFwPar)

        self.NextButton = QPushButton("&Next")
        self.NextButton.setDefault(True)
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
                        try:
                            txt_index = text.rfind(".txt")
                            destination = os.environ.get("PH2ACF_BASE_DIR") + "/test/" + text[text.rfind("/", 0, txt_index)+1:txt_index]
                            os.system(f"wget -O {destination} {text}")
                            self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0).widget().setText(destination)
                        except Exception as e:
                            logger.error(e)
                            self.master.errorMessageBoxSignal.emit(f"Could not Chip{chipid} file from Panthera!")
                            return
                    elif not os.path.exists(text):
                        self.master.errorMessageBoxSignal.emit(f"Chip{chipid}'s .txt file doesn't exist!")
                        return
                    files[moduleName, moduleBox.getFMCPort(), chipid] = self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0).widget().text()

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
