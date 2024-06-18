import os
import time
from serial import SerialException
from typing import Optional

from Gui.QtGUIutils.QtStartWindow import SummaryBox
from PyQt5.QtCore import Qt, QSize, pyqtSignal, QObject, QThread
from PyQt5.QtGui import QPixmap, QImage, QIcon
from PyQt5.QtWidgets import (
    QGridLayout,
    QGroupBox,
    QLabel,
    QPushButton,
    QRadioButton,
    QHBoxLayout,
    QVBoxLayout,
    QWidget,
    QMessageBox,
)


from Gui.QtGUIutils.QtRunWindow import QtRunWindow
from Gui.GUIutils.FirmwareUtil import fwStatusParser
from Gui.python.CustomizedWidget import SimpleBeBoardBox, SimpleModuleBox
from Gui.python.Firmware import QtBeBoard
from Gui.GUIutils.DBConnection import checkDBConnection
import Gui.GUIutils.settings as default_settings
from Gui.python.ArduinoWidget import ArduinoWidget
from Gui.python.Peltier import PeltierSignalGenerator
from Gui.python.logging_config import logger
import Gui.siteSettings as site_settings
from icicle.icicle.instrument_cluster import InstrumentCluster, InstrumentNotInstantiated


class SimplifiedMainWidget(QWidget):
    abort_signal = pyqtSignal()
    close_signal = pyqtSignal()

    def __init__(self, master, connection, username, password, dimension):
        logger.debug("SimplifiedMainWidget.__init__()")
        super().__init__()
        self.master = master
        self.connection = connection
        self.username = username
        self.password = password
        self.dimension = dimension
        
        try:
            self.instruments = InstrumentCluster(**site_settings.
                                                 icicle_instrument_setup)
            self.instruments.open()
        except SerialException:
            instrument_warning_message = QMessageBox()
            instrument_warning_message.setIcon(QMessageBox.Critical)
            instrument_warning_message.setInformativeText(
                    """
                    Your instruments could not be
                    connected! Check siteConfig.py
                    to make sure you have set the
                    ports and devices correctly and
                    check the phyiscal connections to
                    the devices
                    """
            )
            self.close()

        self.instrument_info = {}

        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)
        redledimage = QImage("icons/led-red-on.png").scaled(
            QSize(60, 10), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        self.redledpixmap = QPixmap.fromImage(redledimage)
        greenledimage = QImage("icons/green-led-on.png").scaled(
            QSize(60, 10), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        self.greenledpixmap = QPixmap.fromImage(greenledimage)
        self.createWindow()

    def setupBeBoard(self):
        self.BeBoard = QtBeBoard()
        self.BeBoard.setBoardName(site_settings.defaultFC7)
        self.BeBoard.setIPAddress(site_settings.FC7List[site_settings.defaultFC7])
        #self.BeBoard.setFPGAConfig(default_settings.FPGAConfigList[site_settings.defaultFC7])
        logger.debug(f"Default FC7: {site_settings.defaultFC7}")
        logger.debug("Initialized BeBoard in SimplifiedGUI")
        self.BeBoardWidget = SimpleBeBoardBox(self.BeBoard)
        logger.debug("Initialized SimpleBeBoardBox in Simplified GUI")

    def setupArduino(self):
        self.ArduinoGroup = ArduinoWidget()
        self.ArduinoGroup.stop.connect(self.abort_signal.emit)
        self.ArduinoGroup.enable()
        self.ArduinoGroup.setBaudRate(site_settings.defaultSensorBaudRate)
        self.ArduinoGroup.setPort(site_settings.defaultArduino)
        self.ArduinoGroup.frozeArduinoPanel()
        self.instrument_info["arduino"] = {"Label": QLabel(), "Value": QLabel()}
        self.instrument_info["arduino"]["Label"].setText("Condensation Risk")

    def setupLogFile(self):
        LogFileName = "{0}/Gui/.{1}.log".format(os.environ.get("GUI_dir"),
                                                site_settings.defaultFC7)
        logger.debug(f"FC7 log file saved to {LogFileName}")

        try:
            logFile = open(LogFileName, "w")
            logFile.close()
        except Exception as e:
            messageBox = QMessageBox()
            messageBox.setIcon(QMessageBox.Error)
            logger.error("Could not create file due to error {}".format(e))
            messageBox.setText("Can not create log files: {}".format(LogFileName))
            messageBox.exec()

    def setupPeltier(self):
        try:
            self.instrument_info["peltier"] = {"Label" : QLabel(), "Value" : QLabel()}
            self.instrument_info["peltier"]["Label"].setText("Peltier Temperature")
            logger.debug("Setting up Peltier")
            self.Peltier = PeltierSignalGenerator()
            assert self.Peltier is not None, "Peltier object was not created"
            logger.debug("created self.Peltier")
            # These should emit signals
            if not self.Peltier.sendCommand(
                self.Peltier.createCommand(
                    "Set Type Define Write", ["0", "0", "0", "0", "0", "0", "0", "0"]
                )
            )[1]: raise Exception("Could not communicate with Peltier")
            logger.debug("Execute Peltier write command")

            # Allows set point to be set by computer software
            if not self.Peltier.sendCommand(
                self.Peltier.createCommand(
                    "Control Type Write", ["0", "0", "0", "0", "0", "0", "0", "1"]
                )
            )[1]: raise Exception("Could not communicate with Peltier") # Temperature should be PID controlled
            logger.debug("Executed Peltier PID command")

            message = self.Peltier.convertSetTempValueToList(site_settings.defaultPeltierSetTemp)

            self.Peltier.sendCommand(
                self.Peltier.createCommand("Fixed Desired Control Setting Write", message)
            )
            logger.debug("Set peltier temp")
            if not self.Peltier.sendCommand(
                self.Peltier.createCommand(
                    "Power On/Off Write", ["0", "0", "0", "0", "0", "0", "0", "1"]
                )
            )[1]: raise Exception("Could not communicate with Peltier")   # Turn on Peltier
            logger.debug("Turned off Peltier")
            if not self.Peltier.sendCommand(
                self.Peltier.createCommand(
                    "Proportional Bandwidth Write",
                    ["0", "0", "0", "0", "0", "0", "c", "8"],
                )
            )[1]: raise Exception("Could not communicate with Peltier")  # Set proportional bandwidth
            logger.debug("Set Peltier Bandwidth")
            time.sleep(0.5)

            self.peltier_temperature_label = QLabel(self) 
        except Exception as e:
            print("Error while attempting to set Peltier", e)
            self.Peltier = None

    def setupStatusWidgets(self):
        logger.debug("Set device status")
        self.StatusLayout = QGridLayout()
        self.StatusLayout.addWidget(self.instrument_info["database"]["Label"], 0, 1, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["database"]["Value"], 0, 2, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["hv"]["Label"], 0, 3, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["hv"]["Value"], 0, 4, 1, 1)

        self.StatusLayout.addWidget(self.instrument_info["lv"]["Label"], 1, 1, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["lv"]["Value"], 1, 2, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["fc7"]["Label"], 1, 3, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["fc7"]["Value"], 1, 4, 1, 1)

        self.StatusLayout.addWidget(self.instrument_info["arduino"]["Label"], 2, 1, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["arduino"]["Value"], 2, 2, 1, 1)
        if self.Peltier:
            self.StatusLayout.addWidget(self.instrument_info["peltier"]["Label"], 2, 3, 1, 1)
            self.StatusLayout.addWidget(self.instrument_info["peltier"]["Value"], 2, 4, 1, 1)
            self.StatusLayout.addWidget(self.peltier_temperature_label, 3, 3, 1, 1)
        self.RefreshButton = QPushButton("&Refresh")
        self.RefreshButton.clicked.connect(self.setDeviceStatus)
        self.StatusLayout.addWidget(self.RefreshButton, 3, 3, 1, 1)
        logger.debug("Setup StatusLayout")

    def setupUI(self):

        self.StatusLayout = QGridLayout()
        self.StatusLayout.addWidget(self.instrument_info["database"]["Label"], 0, 1, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["database"]["Value"], 0, 2, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["hv"]["Label"], 0, 3, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["hv"]["Value"], 0, 4, 1, 1)

        self.StatusLayout.addWidget(self.instrument_info["lv"]["Label"], 1, 1, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["lv"]["Value"], 1, 2, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["fc7"]["Label"], 1, 3, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["fc7"]["Value"], 1, 4, 1, 1)

        self.StatusLayout.addWidget(self.instrument_info["arduino"]["Label"], 2, 1, 1, 1)
        self.StatusLayout.addWidget(self.instrument_info["arduino"]["Value"], 2, 2, 1, 1)
        if self.Peltier:
            self.StatusLayout.addWidget(self.instrument_info["peltier"]["Label"], 2, 3, 1, 1)
            self.StatusLayout.addWidget(self.instrument_info["peltier"]["Value"], 2, 4, 1, 1)
            self.StatusLayout.addWidget(self.peltier_temperature_label, 3, 3, 1, 1)
        #self.StatusLayout.addWidget(self.RefreshButton, 3, 3, 1, 1)
        logger.debug("Setup StatusLayout")
        ModuleEntryLayout = QGridLayout()
        ModuleEntryLayout.addWidget(self.BeBoardWidget)
        logger.debug("Setup ModuleEntryLayout")


        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()
        self.TestGroup = QGroupBox()
        self.TestGroupLayout = QVBoxLayout()
        self.ProductionButton = QRadioButton("&Full Test")
        self.QuickButton = QRadioButton("&Quick Test")
        self.QuickButton.setChecked(True)
        self.TestGroupLayout.addWidget(self.QuickButton)
        self.TestGroupLayout.addWidget(self.ProductionButton)

        self.TestGroup.setLayout(self.TestGroupLayout)
        logger.debug("Added Boxes/Layouts to Simplified GUI")

        self.ExitButton = QPushButton("&Exit")
        self.ExitButton.clicked.connect(self.close_signal.emit)
        self.StopButton = QPushButton(self)
        Stopimage = QImage("icons/Stop_v2.png").scaled(
            QSize(80, 80), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        Stoppixmap = QPixmap.fromImage(Stopimage)
        StopIcon = QIcon(Stoppixmap)
        self.StopButton.setIcon(StopIcon)
        self.StopButton.setIconSize(QSize(80, 80))
        self.StopButton.clicked.connect(self.abortTest)
        self.StopButton.setDisabled(True)
        self.RunButton = QPushButton(self)
        Goimage = QImage("icons/gosign_v1.svg").scaled(
            QSize(80, 80), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        Gopixmap = QPixmap.fromImage(Goimage)
        RunIcon = QIcon(Gopixmap)

        self.LogoGroupBox = QGroupBox("")
        self.LogoGroupBox.setCheckable(False)
        self.LogoGroupBox.setMaximumHeight(100)

        self.LogoLayout = QHBoxLayout()
        OSULogoLabel = QLabel()
        OSUimage = QImage("../icons/osuicon.jpg").scaled(
            QSize(200, 60), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        OSUpixmap = QPixmap.fromImage(OSUimage)
        OSULogoLabel.setPixmap(OSUpixmap)
        CMSLogoLabel = QLabel()
        CMSimage = QImage("../icons/cmsicon.png").scaled(
            QSize(200, 60), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        logger.debug("Setup Pixmap")
        CMSpixmap = QPixmap.fromImage(CMSimage)
        CMSLogoLabel.setPixmap(CMSpixmap)
        self.LogoLayout.addWidget(OSULogoLabel)
        window_label = QLabel()
        window_label.setText("Phase 2 Pixel Module Test")
        self.LogoLayout.addWidget(window_label)
        self.LogoLayout.addWidget(CMSLogoLabel)
        logger.debug("Added Logos")
        self.LogoGroupBox.setLayout(self.LogoLayout)

        self.RunButton.setIcon(RunIcon)
        self.RunButton.setIconSize(QSize(80, 80))
        self.RunButton.clicked.connect(self.runNewTest)
        self.StartLayout.addStretch(1)
        self.StartLayout.addWidget(self.TestGroup)
        self.StartLayout.addWidget(self.StopButton)
        self.StartLayout.addWidget(self.RunButton)
        self.AppOption.setLayout(self.StartLayout)

        logger.debug("Setup StartLayout")
        self.simplifiedStatusBox.setLayout(self.StatusLayout)
        self.ModuleEntryBox = QGroupBox("Please scan module QR code")
        self.ModuleEntryBox.setLayout(ModuleEntryLayout)
        self.mainLayout.addWidget(self.simplifiedStatusBox)
        self.mainLayout.addWidget(self.ModuleEntryBox)
        self.mainLayout.addWidget(self.AppOption)

        logger.debug("Simplied GUI UI Loaded")

    def createWindow(self):
        self.simplifiedStatusBox = QGroupBox("Hello, {}!".format(self.username))

        self.instrument_info["database"] = {"Label": QLabel(), "Value": QLabel()}
        self.instrument_info["database"]["Label"].setText("Database connection:")

        self.instrument_info["hv"] = {"Label": QLabel(), "Value": QLabel()}
        self.instrument_info["hv"]["Label"].setText("HV status")

        self.instrument_info["lv"] = {"Label": QLabel(), "Value": QLabel()}
        self.instrument_info["lv"]["Label"].setText("LV status")

        self.instrument_info["fc7"] = {"Label": QLabel(), "Value": QLabel()}
        self.instrument_info["fc7"]["Label"].setText(site_settings.defaultFC7)


        self.setupBeBoard()
        self.setupLogFile() 
        self.setupArduino()
        if site_settings.usePeltier:
            self.setupPeltier()
        else:
            self.Peltier = None
        self.setDeviceStatus() 
        #self.setupStatusWidgets()
        self.setupUI()

    def updateArduinoIndicator(self):
        if(self.ArduinoGroup.condensationRisk):
            self.instrument_info["arduino"]["Value"].setPixmap(self.redledpixmap)
        else:
            self.instrument_info["arduino"]["Value"].setPixmap(self.greenledpixmap)

    def updatePeltierTemp(self, temp:float):
        self.peltier_temperature_label.setText("{}C".format(temp))
        if abs(temp - site_settings.defaultPeltierSetTemp) < 15:
            self.instrument_info["peltier"]["Value"].setPixmap(self.greenledpixmap)
        else: 
            self.instrument_info["peltier"]["Value"].setPixmap(self.redledpixmap)


    def runNewTest(self):
        for module in self.BeBoardWidget.getModules():
            if module.getSerialNumber() == "":
                QMessageBox.information(
                    None, "Error", "No valid serial number!", QMessageBox.Ok
                )
                return
            if module.getID() == "":
                QMessageBox.information(None, "Error", "No valid ID!", QMessageBox.Ok)
                return

        self.firmwareDescription = self.BeBoardWidget.getFirmwareDescription()
        if self.BeBoard.getModuleByIndex(0) == None:
            QMessageBox.information(
                None,
                "Error",
                "No valid Module found!  If manually entering module number be sure to press 'Enter' on keyboard.",
                QMessageBox.Ok,
            )
            return
        if self.ProductionButton.isChecked():
            self.info = [
                self.BeBoard.getModuleByIndex(0).getOpticalGroupID(),
                "ROCTune",
            ]
        else:
            self.info = [
                self.BeBoard.getModuleByIndex(0).getOpticalGroupID(),
                "QuickTest",
            ]
        self.runFlag = True
        self.RunTest = QtRunWindow(self.master, self.info, self.firmwareDescription)
        self.RunButton.setDisabled(True)
        self.StopButton.setDisabled(False)

        self.RunTest.resetConfigTest()
        
        module:SimpleModuleBox = self.BeBoardWidget.getModules()[0]

        module_type = module.getType(module.getSerialNumber())
        print("Module Type: {}".format(module_type))
        self.master.module_in_use = module_type
        print("BeBoard is: {}".format(self.BeBoard))
        fw_check = SummaryBox.checkFwPar(site_settings.defaultFC7, module_type, site_settings.FC7List[site_settings.defaultFC7])
        self.RunTest.initialTest()
        # self.RunTest.runTest()

    def abortTest(self):
        self.RunTest.abortTest()
        self.StopButton.setDisabled(True)
        self.RunButton.setDisabled(False)

    def setDeviceStatus(self) -> None:
        """
        Set status for all connected devices
        The only device that needs to be polled is the Peltier (and maybe the Arduino)
        Send these to a worker thread to avoid freezing of the GUI.
        The qualifications for a passing status are
        HV  -> HV is on and connected as stated by InstrumentCluster.status()
        LV  -> LV is on and connected as stated by InstrumentCluster.status()
        Arduino -> Can read correctly from the Arduino sensor as defined in ArduinoWidget.py 
        Database -> Check if you can connect to database as defined in checkDBConnection()
        Peltier -> check if the Peltier is at the right temperature and is reachable 
        """ 

        #self.instrument_status = self.check_icicle_devices()
        self.instrument_status = {} 
        #logger.debug(f"Instrument status is {self.instrument_status}")

        logger.debug("Getting FC7 Comment")

        LogFileName = "{0}/Gui/.{1}.log".format(os.environ.get("GUI_dir"), site_settings.defaultFC7)
        FwStatusComment, _, _ = fwStatusParser(self.BeBoard, LogFileName) 
        logger.debug("Checking DB Connection")
        statusString, _ = checkDBConnection(self.connection)


        # Launch QThread to monitor Peltier temperature/power and Arduino temperature/humidity
        self.thread = QThread()
        self.worker = Worker_Polling()
        self.worker.moveToThread(self.thread)
        self.thread.started.connect(self.worker.run)
        if site_settings.usePeltier:
            self.worker.temp.connect(self.updatePeltierTemp)
        self.worker.temp.connect(self.updateArduinoIndicator)
        self.thread.start()

        logger.debug("Setting up instrument_status")
        logger.debug("instrument_status: {}".format(self.instrument_status))
        logger.debug("instruments: ".format(self.instruments))
       
        self.instrument_status["arduino"] = self.ArduinoGroup.ArduinoGoodStatus 
        self.instrument_status["fc7"] = "Connected" in FwStatusComment 
        self.instrument_status["database"]= not "offline" in statusString 
        

        # Icicle will deal with the powersupplies, so I will just always set their status to good
        # Technically a false sense of security for the user. 
        self.instrument_status["hv"] = True
        self.instrument_status["lv"] = True
        if site_settings.usePeltier:
            self.instrument_status["peltier"] = False
        if self.instruments:
            logger.debug(f'{__name__} Setup instrument status {self.instrument_status}')
            for key, value in self.instrument_info.items():
                if self.instrument_status[key]:  
                    value["Value"].setPixmap(self.greenledpixmap)
                else:
                    value["Value"].setPixmap(self.redledpixmap)
        else:
            for key, value in self.instrument_info.items():
                value["Value"].setPixmap(self.redledpixmap)
        logger.debug(f'{__name__} Setup led labels')

    def check_icicle_devices(self) ->Optional[dict[str, int]]:
        """
        Check if LV, HV, and relay board are connected and communicable

        If the LV or HV are connected but not on they will have a status of 0
        using the instrument_cluster.status() method. If we cannot communicate
        with them then they will freeze the GUI. Therefore, if the code reaches
        this part the HV and LV should always have a good status and we need to
        invert the values from instrument_cluster.status()
        """
        try: 
            status = self.instruments.status(lv_channel=1)
        except RuntimeError:
            error_box = QMessageBox()
            error_box.setInformativeText(
                """
                Could not connect to all devices, please check device
                connections and ensure you are want to connect to all devices
                as stated in siteConfig.py in the icicle_instrument_setup
                dictionary. 
                """
                )
            error_box.setIcon(QMessageBox.Critical)
            error_box.setStandardButtons(QMessageBox.Ok)
            error_box.exec()
            self.destroySimplified()
        logger.debug(f"Status of instrument_cluster instruments: {status}")
        return_status = {} 
        for key, value in status.items():
            if value == 0:
                return_status[key] = 1
            elif type(value) == InstrumentNotInstantiated:
                return_status[key] = 0
        return return_status        

    def retryLogin(self):
        self.master.mainLayout.removeWidget(self)
        self.deleteLater()

    def destroySimplified(self):
        self.clearLayout(self.master.mainLayout)
        self.master.createLogin()

    def clearLayout(self, layout):
        while layout.count():
            child = layout.takeAt(0)
            if child.widget():
                child.widget().deleteLater()


class Worker_Polling(QObject):
    temp = pyqtSignal(float)
    power = pyqtSignal(bool)
    def __init__(self):
        super().__init__()
        # Delay in seconds between polling
        self.delay = 0.5
        self.abort = False
    def run(self):
        while not self.abort and site_settings.usePeltier: 
            self.Peltier = PeltierSignalGenerator()
            peltier_power_status = 1 if int(self.Peltier.sendCommand(self.Peltier.createCommand("Power On/Off Read", ["0", "0"]))[-1]) == 1 else 0
            peltier_temp_message, temp_message_pass = self.Peltier.sendCommand(self.Peltier.createCommand("Input1",  ["0", "0", "0", "0", "0", "0", "0", "0"]))
            if not temp_message_pass:
                peltier_temp_message = None
            logger.debug("Formatting peltier output")
            if peltier_temp_message:
                peltier_temp = int("".join(peltier_temp_message[1:9]), 16)/100
            else:
                peltier_temp = None

            self.temp.emit(peltier_temp)
            self.power.emit(peltier_power_status)
            time.sleep(self.delay)
        
        while not self.abort:
            self.temp.emit(0.0)
            time.sleep(self.delay)
    def abort_worker(self):
        print("Worker aborted")
        self.abort = True
