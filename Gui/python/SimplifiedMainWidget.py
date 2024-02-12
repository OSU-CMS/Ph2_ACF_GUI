import os
import time
from serial import SerialException

from PyQt5.QtCore import Qt, QSize
from PyQt5.QtGui import QPixmap, QImage, QIcon
from PyQt5.QtWidgets import (
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QRadioButton,
    QHBoxLayout,
    QVBoxLayout,
    QWidget,
    QMessageBox,
)


from Gui.QtGUIutils.QtRunWindow import QtRunWindow
from Gui.python.CustomizedWidget import SimpleBeBoardBox
from Gui.python.Firmware import QtBeBoard
from Gui.GUIutils.DBConnection import checkDBConnection
import Gui.GUIutils.settings as default_settings
from Gui.python.ArduinoWidget import ArduinoWidget
from Gui.python.Peltier import PeltierSignalGenerator 
from Gui.python.logging_config import logger
import Gui.siteSettings as site_settings
from icicle.icicle.instrument_cluster import InstrumentCluster, InstrumentNotInstantiated


class SimplifiedMainWidget(QWidget):
    def __init__(self, master):
        logger.debug("SimplifiedMainWidget.__init__()")
        super().__init__()
        self.master = master
        self.connection = self.master.connection
        self.TryUsername = self.master.TryUsername
        self.DisplayedPassword = self.master.DisplayedPassword

        try :
            self.instruments = InstrumentCluster(**site_settings.icicle_instrument_setup)
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
        self.setupMainUI()


    def setupMainUI(self):
        self.simplifiedStatusBox = QGroupBox("Hello, {}!".format(self.TryUsername))

        self.instrument_info["database"] = {"Label": QLabel(), "Value": QLabel()}
        self.instrument_info["database"]["Label"].setText("Database connection:")

        self.instrument_info["hv"] = {"Label": QLabel(), "Value": QLabel()}
        self.instrument_info["hv"]["Label"].setText("HV status")

        self.instrument_info["lv"] = {"Label": QLabel(), "Value": QLabel()}
        self.instrument_info["lv"]["Label"].setText("LV status")

        self.instrument_info["fc7"] = {"Label": QLabel(), "Value": QLabel()}
        self.instrument_info["fc7"]["Label"].setText(site_settings.defaultFC7)

        self.RefreshButton = QPushButton("&Refresh")
        self.RefreshButton.clicked.connect(self.checkDevices)


        self.ModuleEntryBox = QGroupBox("Please scan module QR code")
        ModuleEntryLayout = QGridLayout()


        self.BeBoard = QtBeBoard()
        self.BeBoard.setBoardName(site_settings.defaultFC7)
        self.BeBoard.setIPAddress(site_settings.FC7List[site_settings.defaultFC7])
        self.BeBoard.setFPGAConfig(default_settings.FPGAConfigList[site_settings.defaultFC7])
        logger.debug(f"Default FC7: {site_settings.defaultFC7}")

        self.master.FwDict[default_settings.defaultFC7] = self.BeBoard
        logger.debug("Initialized BeBoard in SimplifiedGUI")
        self.BeBoardWidget = SimpleBeBoardBox(self.BeBoard)
        logger.debug("Initialized SimpleBeBoardBox in Simplified GUI")
        
        LogFileName = "{0}/Gui/.{1}.log".format(os.environ.get("GUI_dir"), default_settings.defaultFC7)
        logger.debug(f"FC7 log file saved to {LogFileName}")

        try:
            logFile = open(LogFileName, "w")
            logFile.close()
        except:
            messageBox = QMessageBox()
            messageBox.setIcon(QMessageBox.Error)
            messageBox.setText("Can not create log files: {}".format(LogFileName))
            messageBox.exec() 

        self.FwModule = self.master.FwDict[default_settings.defaultFC7]


        self.ArduinoGroup = ArduinoWidget()
        self.ArduinoGroup.stop.connect(self.master.GlobalStop)
        self.ArduinoGroup.enable()
        self.ArduinoGroup.setBaudRate(default_settings.defaultSensorBaudRate)
        self.ArduinoGroup.frozeArduinoPanel()

        self.instrument_info["arduino"] = {"Label": QLabel(), "Value": QLabel()}
        self.instrument_info["arduino"]["Label"].setText("Temperature and Humidity")

        self.Peltier = None
        if site_settings.usePeltier:
            try:
                self.instrument_info["peltier"] = {"Label" : QLabel(), "Value" : QLabel()}
                self.instrument_info["peltier"]["Label"].setText("Chip Temperature")
                logger.debug("Setting up Peltier")
                self.Peltier = PeltierSignalGenerator()
                logger.debug("created self.Peltier")
                # These should emit signals
                if not self.pelt.sendCommand(
                    self.pelt.createCommand(
                        "Set Type Define Write", ["0", "0", "0", "0", "0", "0", "0", "0"]
                    )
                )[1]: raise Exception("Could not communicate with Peltier")

                    # Allows set point to be set by computer software
                if not self.pelt.sendCommand(
                    self.pelt.createCommand(
                        "Control Type Write", ["0", "0", "0", "0", "0", "0", "0", "1"]
                    )
                )[1]: raise Exception("Could not communicate with Peltier") # Temperature should be PID controlled

                if not self.pelt.sendCommand(
                    self.pelt.createCommand(
                        "Power On/Off Write", ["0", "0", "0", "0", "0", "0", "0", "0"]
                    )
                )[1]: raise Exception("Could not communicate with Peltier")   # Turn off power to Peltier in case it is on at the start

                if not self.pelt.sendCommand(
                    self.pelt.createCommand(
                        "Proportional Bandwidth Write",
                        ["0", "0", "0", "0", "0", "0", "c", "8"],
                    )
                )[1]: raise Exception("Could not communicate with Peltier")  # Set proportional bandwidth

                message = self.Peltier.convertSetTempValueToList(site_settings.defaultPeltierSetTemp)

                self.Peltier.sendCommand(
                    self.Peltier.createCommand("Fixed Desired Control Setting Write", message)
                )

                time.sleep(0.5)

            except Exception as e:
                print("Error while attempting to set Peltier", e)
                self.Peltier = None
        logger.debug("About to set device status")
        self.setDeviceStatus() 

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
        self.StatusLayout.addWidget(self.RefreshButton, 3, 3, 1, 1)
        logger.debug("Setup StatusLayout")
        ModuleEntryLayout.addWidget(self.BeBoardWidget)
        logger.debug("Setup ModuleEntryLayout")

        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()
        self.TestGroup = QGroupBox()
        self.TestGroupLayout = QVBoxLayout()
        self.ProductionButton = QRadioButton("&Production Test")
        self.QuickButton = QRadioButton("&Quick Test")
        self.ProductionButton.setChecked(True)
        self.TestGroupLayout.addWidget(self.ProductionButton)
        self.TestGroupLayout.addWidget(self.QuickButton)
        self.TestGroup.setLayout(self.TestGroupLayout)
        logger.debug("Added Boxes/Layouts to Simplified GUI")

        self.ExitButton = QPushButton("&Exit")
        self.ExitButton.clicked.connect(self.master.close)
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
        self.ModuleEntryBox.setLayout(ModuleEntryLayout)
        self.mainLayout.addWidget(self.simplifiedStatusBox)
        self.mainLayout.addWidget(self.ModuleEntryBox)
        self.mainLayout.addWidget(self.AppOption)

        logger.debug("Simplied GUI UI Loaded")
        
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
        if self.FwModule.getModuleByIndex(0) == None:
            QMessageBox.information(
                None,
                "Error",
                "No valid Module found!  If manually entering module number be sure to press 'Enter' on keyboard.",
                QMessageBox.Ok,
            )
            return
        if self.ProductionButton.isChecked():
            self.info = [
                self.FwModule.getModuleByIndex(0).getOpticalGroupID(),
                "AllScan",
            ]
        else:
            self.info = [
                self.FwModule.getModuleByIndex(0).getOpticalGroupID(),
                "QuickTest",
            ]

        self.runFlag = True
        self.RunTest = QtRunWindow(self.master, self.info, self.firmwareDescription)
        # TODO  There shouldn't be a default value here, we should be getting the LV from the QR code or the module name
        current = float(current) if current else 0.0
        voltage = float(voltage) if voltage else 0.0
        self.RunButton.setDisabled(True)
        self.StopButton.setDisabled(False)

        self.RunTest.resetConfigTest()
        self.RunTest.initialTest()
        # self.RunTest.runTest()

    def abortTest(self):
        self.RunTest.abortTest()
        self.StopButton.setDisabled(True)
        self.RunButton.setDisabled(False)

    def setDeviceStatus(self) -> None:
        """
        Set status for all connected devices
        The qualifications for a passing status are
        HV  -> HV is on and connected as stated by InstrumentCluster.status()
        LV  -> LV is on and connected as stated by InstrumentCluster.status()
        Arduino -> Can read correctly from the Arduino sensor as defined in ArduinoWidget.py 
        Database -> Check if you can connect to database as defined in checkDBConnection()
        Peltier -> check if the Peltier is at the right temperature and is reachable 
        """ 

        self.instrument_status = self.check_powersupplies_and_relay()

        logger.debug(f"Instrument status is {self.instrument_status}")

        logger.debug("Getting FC7 Comment")
        FwStatusComment, _, _ = self.master.getFwComment(
            default_settings.defaultFC7, default_settings.defaultFC7IP
        )
        logger.debug("Checking DB Connection")
        statusString, _ = checkDBConnection(self.connection)


        if self.Peltier: 
            peltier_power_status = None
            peltier_temp_status = None
            logger.debug("Obtaining peltier status")
            peltier_power_status = 1 if int(self.Peltier.sendCommand(self.Peltier.createCommand("Power On/Off Read", ["0", "0"]))[-1]) == 1 else 0
            peltier_temp_message, temp_message_pass = self.Peltier.sendCommand(self.Peltier.createCommand("Input1",  ["0", "0", "0", "0", "0", "0", "0", "0"]))
            if not temp_message_pass:
                peltier_temp_message = None

            logger.debug("Formatting peltier output")
            if peltier_temp_message:
                peltier_temp = int("".join(peltier_temp_message[1:9]), 16)/100
            else:
                peltier_temp = None

            logger.debug("Evaluating temp status")

            peltier_temp_status = 1 if (peltier_temp and abs(peltier_temp - default_settings.defaultPeltierSetTemp) < 10) else 0

            logger.debug("Obtaining peltier status")
            peltier_power_status = 1 if int(self.Peltier.sendCommand(self.Peltier.createCommand("Power On/Off Read", ["0", "0"]))[-1]) == 1 else 0
            peltier_temp_message, _ = self.Peltier.sendCommand(self.Peltier.createCommand("Input1",  ["0", "0", "0", "0", "0", "0", "0", "0"]))
            logger.debug("Formatting peltier output")
            if peltier_temp_message: 
                peltier_temp = int("".join(map(str, peltier_temp_message[1:9])), 16)/100
            else:
                peltier_temp = None
            logger.debug("Evaluating temp status")
            if peltier_temp:
                peltier_temp_status = 1 if abs(peltier_temp - default_settings.defaultPeltierSetTemp) < 10 else 0


        logger.debug("Setting up instrument_status")
        if self.instruments:
            self.instrument_status["arduino"] = self.ArduinoGroup.ArduinoGoodStatus 
            self.instrument_status["fc7"] = "Connected" in FwStatusComment 
            self.instrument_status["database"]= not "offline" in statusString 
        if self.Peltier:
            self.instrument_status["peltier"] = peltier_power_status and peltier_temp_status 
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

    def check_powersupplies_and_relay(self) -> dict[str, int]:
        """
        Check if LV, HV, and relay board are connected and communicable

        If the LV or HV are connected but not on they will have a status of 0
        using the instrument_cluster.status() method. If we cannot communicate
        with them then they will freeze the GUI. Therefore, if the code reaches
        this part the HV and LV should always have a good status and we need to
        invert the values from instrument_cluster.status()
        """
        status = self.instruments.status(lv_channel=1)
        logger.debug(f"Status of instrument_cluster instruments: {status}")
        return_status = {} 
        for key, value in status.items():
            if value == 0:
                return_status[key] = 1
            elif type(value) == InstrumentNotInstantiated:
                return_status[key] = 0
        return return_status

    def checkDevices(self):
        statusString, _ = checkDBConnection(self.connection)
        if "offline" in statusString:
            self.DBStatusValue.setPixmap(self.redledpixmap)
        else:
            self.DBStatusValue.setPixmap(self.greenledpixmap)

        
        self.HVpowersupply.setPowerModel(defaultHVModel[0])
        self.HVpowersupply.setInstrument(defaultUSBPortHV[0])
        statusString = self.HVpowersupply.getInfo()
        self.HVPowerStatusLabel.setText("HV status")
        if statusString != "No valid device" and statusString != None:
            self.HVPowerStatusValue.setPixmap(self.greenledpixmap)
        else:
            self.HVPowerStatusValue.setPixmap(self.redledpixmap)
        time.sleep(0.5)
        self.LVPowerStatusLabel.setText("LV status")
        if statusString != "No valid device" and statusString != None:
            self.LVPowerStatusValue.setPixmap(self.greenledpixmap)
        else:
            self.LVPowerStatusValue.setPixmap(self.redledpixmap)

        firmwareName, fwAddress = defaultFC7, defaultFC7IP

        LogFileName = "{0}/Gui/.{1}.log".format(os.environ.get("GUI_dir"), firmwareName)
        try:
            logFile = open(LogFileName, "w")
            logFile.close()
        except:
            QMessageBox(
                None, "Error", "Can not create log files: {}".format(LogFileName)
            )

        FwStatusComment, _, _ = self.master.getFwComment(
            firmwareName, LogFileName
        )
        if "Connected" in FwStatusComment:
            self.FC7StatusValue.setPixmap(self.greenledpixmap)
        else:
            self.FC7StatusValue.setPixmap(self.redledpixmap)

        self.FwModule = self.master.FwDict[firmwareName]


        # Arduino stuff
        self.ArduinoGroup.stop.connect(self.master.GlobalStop)
        self.ArduinoGroup.createArduino()
        self.ArduinoGroup.enable()
        self.ArduinoGroup.setBaudRate(defaultSensorBaudRate)
        self.ArduinoGroup.frozeArduinoPanel()

        if self.ArduinoGroup.ArduinoGoodStatus == True:
            self.ArduinoMonitorValue.setPixmap(self.greenledpixmap)
        else:
            self.ArduinoMonitorValue.setPixmap(self.redledpixmap)

