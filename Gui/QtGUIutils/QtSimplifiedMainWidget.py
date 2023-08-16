from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont, QPixmap, QImage, QIcon
from PyQt5.QtWidgets import (
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QRadioButton,
    QHBoxLayout,
    QVBoxLayout,
    QWidget,
    QMessageBox,
)
import time
import logging

# NOTE: This may not be used
logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger(__name__)

from Gui.QtGUIutils.QtRunWindow import QtRunWindow
from Gui.QtGUIutils.QtFwCheckDetails import *
from Gui.python.CustomizedWidget import *
from Gui.python.Firmware import *
from Gui.GUIutils.DBConnection import *
from Gui.GUIutils.FirmwareUtil import fwStatusParser
import Gui.Config.siteConfig as sc
import Gui.Config.staticSettings as ss
from Gui.python.ArduinoWidget import *
from Gui.python.Peltier import PeltierSignalGenerator


class QtSimplifiedMainWidget(QWidget):
    """
    Use to create the Non-expert user window

    Methods:
    createLogin -- Used to place widgets on screen
    setupMainUI -- Used to connect buttons and display buttons correctly
    """

    def __init__(self, master):
        super().__init__()
        print("Inside Simplified Widget")
        self.master = master
        self.connection = self.master.database_connection
        self.LVpowersupply = self.master.lv_powersupply
        self.HVpowersupply = self.master.hv_powersupply
        self.TryUsername = self.master.connection_parameters["username"]
        self.DisplayedPassword = self.master.connection_parameters["password"]
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

        self.createLogin()
        self.setupMainUI()
        self.show()

    def createLogin(self):
        """Create login section of the Simplified GUI"""

        print("Inside createLogin()")
        self.LoginGroupBox = QGroupBox("")
        self.LoginGroupBox.setCheckable(False)

        TitleLabel = QLabel('<font size="12"> Phase2 Pixel Module Test </font>')
        TitleLabel.setFont(QFont("Courier"))
        TitleLabel.setMaximumHeight(30)

        UsernameLabel = QLabel("Username:")
        self.UsernameEdit = QLineEdit("")
        self.UsernameEdit.setEchoMode(QLineEdit.Normal)
        self.UsernameEdit.setPlaceholderText(self.TryUsername)
        self.UsernameEdit.setMinimumWidth(220)
        self.UsernameEdit.setMaximumWidth(260)
        self.UsernameEdit.setMaximumHeight(30)
        self.UsernameEdit.setReadOnly(True)

        PasswordLabel = QLabel("Password:")
        self.PasswordEdit = QLineEdit("")
        self.PasswordEdit.setEchoMode(QLineEdit.Password)
        self.PasswordEdit.setPlaceholderText(self.DisplayedPassword)
        self.PasswordEdit.setMinimumWidth(220)
        self.PasswordEdit.setMaximumWidth(260)
        self.PasswordEdit.setMaximumHeight(30)
        self.PasswordEdit.setReadOnly(True)

        layout = QGridLayout()
        layout.setSpacing(20)
        layout.addWidget(TitleLabel, 0, 1, 1, 3, Qt.AlignCenter)
        layout.addWidget(UsernameLabel, 1, 1, 1, 1, Qt.AlignCenter)
        layout.addWidget(self.UsernameEdit, 1, 2, 1, 2)
        layout.addWidget(PasswordLabel, 2, 1, 1, 1, Qt.AlignCenter)
        layout.addWidget(self.PasswordEdit, 2, 2, 1, 2)

        layout.setColumnStretch(0, 1)
        layout.setColumnStretch(1, 1)
        layout.setColumnStretch(2, 2)
        layout.setColumnStretch(3, 1)
        self.LoginGroupBox.setLayout(layout)

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

        self.mainLayout.addWidget(self.LoginGroupBox, 0, 0)

    def setupMainUI(self):
        ##################################
        ##  Testing some things out #######
        ##################################
        print("inside setupMainUI")
        self.simplifiedStatusBox = QGroupBox("Hello, {}!".format(self.TryUsername))

        statusString, _ = check_database_connection(self.connection)
        self.DBStatusLabel = QLabel()
        self.DBStatusLabel.setText("Database connection:")
        self.DBStatusValue = QLabel()
        if "offline" in statusString:
            self.DBStatusValue.setPixmap(self.redledpixmap)
        else:
            self.DBStatusValue.setPixmap(self.greenledpixmap)

        self.RefreshButton = QPushButton("&Refresh")
        self.RefreshButton.clicked.connect(self.checkDevices)

        self.HVPowerStatusLabel = QLabel()
        self.HVPowerStatusValue = QLabel()
        self.LVPowerStatusLabel = QLabel()
        self.LVPowerStatusValue = QLabel()

        self.ModuleEntryBox = QGroupBox("Please scan module QR code")
        ModuleEntryLayout = QGridLayout()

        SerialLabel = QLabel("SerialNumber:")
        self.SerialEdit = QLineEdit()

        CableLabel = QLabel("CableNumber")
        self.CableEdit = QLineEdit()

        # Selecting default HV
        self.HVpowersupply.setPowerModel(sc.defaultHVModel[0])
        self.HVpowersupply.setPortAddress(sc.defaultUSBPortHV[0])
        statusString = self.HVpowersupply.getInfo()
        self.HVPowerStatusLabel.setText("HV status")
        if statusString != "No valid device" and statusString != None:
            self.HVPowerStatusValue.setPixmap(self.greenledpixmap)

        else:
            self.HVPowerStatusValue.setPixmap(self.redledpixmap)
        time.sleep(0.5)

        # Selecting default LV
        self.LVpowersupply.setPowerModel(sc.defaultLVModel[0])
        self.LVpowersupply.setPortAddress(sc.defaultUSBPortLV[0])
        statusString = self.LVpowersupply.getInfo()
        self.LVPowerStatusLabel.setText("LV status")

        if statusString != "No valid device" and statusString != None:
            self.LVPowerStatusValue.setPixmap(self.greenledpixmap)
        else:
            self.LVPowerStatusValue.setPixmap(self.redledpixmap)

        self.StatusList = []
        self.StatusList.append([self.DBStatusLabel, self.DBStatusValue])
        self.StatusList.append([self.HVPowerStatusLabel, self.HVPowerStatusValue])
        self.StatusList.append([self.LVPowerStatusLabel, self.LVPowerStatusValue])

        ### FC7 Setup ###
        self.FC7NameLabel = QLabel()
        self.FC7NameLabel.setText(sc.defaultFC7)
        self.FC7StatusValue = QLabel()

        fc7_name, fwAddress = sc.defaultFC7, sc.defaultFC7IP

        self.BeBoard = FC7()
        self.BeBoard.setBoardName(fc7_name)
        self.BeBoard.setIPAddress(sc.FC7List[fc7_name])
        self.BeBoard.setFPGAConfig(ss.FPGAConfigList[fc7_name])

        self.master.fc7_dict[fc7_name] = self.BeBoard
        self.BeBoardWidget = SimpleBeBoardBox(self.BeBoard)

        LogFileName = "{0}/Gui/.{1}.log".format(os.environ.get("GUI_dir"), fc7_name)
        try:
            logFile = open(LogFileName, "w")
            logFile.close()
        except:
            QMessageBox(
                None, "Error", "Can not create log files: {}".format(LogFileName)
            )

        fc7_status_comment, FwStatusColor, FwStatusVerbose = fwStatusParser(
            self.BeBoard, LogFileName
        )
        if "Connected" in fc7_status_comment:
            self.FC7StatusValue.setPixmap(self.greenledpixmap)
        else:
            self.FC7StatusValue.setPixmap(self.redledpixmap)
        self.FwModule = self.master.fc7_dict[fc7_name]
        self.StatusList.append([self.FC7NameLabel, self.FC7StatusValue])

        ### Setup Arduino ###
        self.ArduinoGroup = ArduinoWidget()
        self.ArduinoGroup.stop.connect(self.master.GlobalStop)
        self.ArduinoGroup.enable()
        self.ArduinoGroup.setBaudRate(defaultSensorBaudRate)
        self.ArduinoGroup.frozeArduinoPanel()
        self.ArduinoMonitorLabel = QLabel()
        self.ArduinoMonitorValue = QLabel()
        self.ArduinoMonitorLabel.setText("Temperature and Humidity")
        if self.ArduinoGroup.ArduinoGoodStatus == True:
            self.ArduinoMonitorValue.setPixmap(self.greenledpixmap)
        else:
            self.ArduinoMonitorValue.setPixmap(self.redledpixmap)
        self.StatusList.append([self.ArduinoMonitorLabel, self.ArduinoMonitorValue])

        ### Setup Peltier ###
        try:
            self.pelt = PeltierSignalGenerator()
            # Sending commands to setup Peltier Controller
            self.pelt.sendCommand(
                self.pelt.createCommand(
                    "Set Type Define Write", ["0", "0", "0", "0", "0", "0", "0", "0"]
                )
            )  # Allows set point to be set by computer software
            self.pelt.sendCommand(
                self.pelt.createCommand(
                    "Control Type Write", ["0", "0", "0", "0", "0", "0", "0", "1"]
                )
            )  # Temperature should be PID controlled
            self.pelt.sendCommand(
                self.pelt.createCommand(
                    "Proportional Bandwidth Write",
                    ["0", "0", "0", "0", "0", "0", "c", "8"],
                )
            )  # Set proportional bandwidth

            set_temp = self.pelt.convertSetTempValueToList(ss.defaultPeltierSetTemp)
            self.pelt.sendCommand(
                self.pelt.createCommand("Fixed Desired Control Setting Write", set_temp)
            )
            # self.Peltier.powerController(1)
            time.sleep(0.5)

            self.PeltierPower, _ = self.pelt.sendCommand(
                self.pelt.createCommand(
                    "Power On/Off Read", ["0", "0", "0", "0", "0", "0", "0", "0"]
                )
            )

            # If the power is on the last value in the list will be 1, if off, 0
            self.PeltierPower = self.PeltierPower[-1]

        except Exception as e:
            print("Error while attempting to setup Peltier: ", e)
            self.PeltierPower = None

        self.PeltierMonitorLabel = QLabel()
        self.PeltierMonitorValue = QLabel()
        self.PeltierMonitorValue.setText("Peltier Value")
        self.PeltierMonitorLabel.setText("Peltier Cooling")

        if self.PeltierPower:
            if int(self.PeltierPower) == 1:
                self.PeltierMonitorValue.setPixmap(self.greenledpixmap)
            else:
                self.PeltierMonitorLabel.setPixmap(self.redledpixmap)
        else:
            self.PeltierMonitorValue.setPixmap(self.redledpixmap)

        self.StatusLayout = QGridLayout()
        self.StatusLayout.addWidget(self.DBStatusLabel, 0, 1, 1, 1)
        self.StatusLayout.addWidget(self.DBStatusValue, 0, 2, 1, 1)
        self.StatusLayout.addWidget(self.HVPowerStatusLabel, 0, 3, 1, 1)
        self.StatusLayout.addWidget(self.HVPowerStatusValue, 0, 4, 1, 1)

        self.StatusLayout.addWidget(self.LVPowerStatusLabel, 1, 1, 1, 1)
        self.StatusLayout.addWidget(self.LVPowerStatusValue, 1, 2, 1, 1)
        self.StatusLayout.addWidget(self.FC7NameLabel, 1, 3, 1, 1)
        self.StatusLayout.addWidget(self.FC7StatusValue, 1, 4, 1, 1)

        self.StatusLayout.addWidget(self.ArduinoMonitorLabel, 2, 1, 1, 1)
        self.StatusLayout.addWidget(self.ArduinoMonitorValue, 2, 2, 1, 1)
        # self.StatusLayout.addWidget(self.ArduinoGroup.ArduinoMeasureValue)
        self.StatusLayout.addWidget(self.PeltierMonitorLabel, 2, 3, 1, 1)
        self.StatusLayout.addWidget(self.PeltierMonitorValue, 2, 4, 1, 1)
        self.StatusLayout.addWidget(self.RefreshButton, 3, 5, 1, 2)

        ### Setup Module Entry Section of Simplified GUI ###
        ModuleEntryLayout.addWidget(self.BeBoardWidget)

        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()
        self.TestGroup = QGroupBox()
        self.TestGroupLayout = QVBoxLayout()
        # self.TestGroup = QWidget()
        self.ProductionButton = QRadioButton("&Production Test")
        self.QuickButton = QRadioButton("&Quick Test")
        self.ProductionButton.setChecked(True)
        # self.QuickButton.setChecked(True)
        self.TestGroupLayout.addWidget(self.ProductionButton)
        self.TestGroupLayout.addWidget(self.QuickButton)
        self.TestGroup.setLayout(self.TestGroupLayout)

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
        self.RunButton.setIcon(RunIcon)
        self.RunButton.setIconSize(QSize(80, 80))
        self.RunButton.clicked.connect(self.runNewTest)
        self.StartLayout.addStretch(1)
        self.StartLayout.addWidget(self.TestGroup)
        # self.StartLayout.addWidget(self.ExitButton)
        self.StartLayout.addWidget(self.StopButton)
        self.StartLayout.addWidget(self.RunButton)
        self.AppOption.setLayout(self.StartLayout)

        self.simplifiedStatusBox.setLayout(self.StatusLayout)
        self.ModuleEntryBox.setLayout(ModuleEntryLayout)
        # self.mainLayout.addWidget(self.welcomebox)
        self.mainLayout.addWidget(self.simplifiedStatusBox)
        self.mainLayout.addWidget(self.ModuleEntryBox)
        self.mainLayout.addWidget(self.AppOption)
        self.mainLayout.addWidget(self.LogoGroupBox)
        print("At end of setupMainUI()")

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
        self.LVpowersupply.setPoweringMode(defaultPowerMode)
        # self.LVpowersupply.setCompCurrent(compcurrent = 1.05) # Fixed for different chip
        # self.LVpowersupply.setModuleType(ss.defaultModuleType)
        self.LVpowersupply.TurnOn()
        # self.HVpowersupply.RampingUp(-60,-3)
        current = self.LVpowersupply.ReadCurrent()
        current = float(current) if current else 0.0
        voltage = self.LVpowersupply.ReadVoltage()
        voltage = float(voltage) if voltage else 0.0
        # print('Current = {0}'.format(current))
        self.RunButton.setDisabled(True)
        self.StopButton.setDisabled(False)

        self.RunTest.resetConfigTest()
        self.RunTest.initialTest()
        # self.RunTest.runTest()

    def abortTest(self):
        self.RunTest.abortTest()
        self.StopButton.setDisabled(True)
        self.RunButton.setDisabled(False)

    def checkDevices(self):
        statusString, colorString = check_database_connection(self.connection)
        if "offline" in statusString:
            self.DBStatusValue.setPixmap(self.redledpixmap)
        else:
            self.DBStatusValue.setPixmap(self.greenledpixmap)
        # self.DBStatusValue.setText(statusString)
        # self.DBStatusValue.setStyleSheet(colorString)

        # Selecting default HV
        self.HVpowersupply.setPowerModel(defaultHVModel[0])
        self.HVpowersupply.setInstrument(defaultUSBPortHV[0])
        statusString = self.HVpowersupply.getInfo()
        self.HVPowerStatusLabel.setText("HV status")
        if statusString != "No valid device" and statusString != None:
            # self.HVPowerStatusValue.setStyleSheet("color:green")
            self.HVPowerStatusValue.setPixmap(self.greenledpixmap)
        else:
            # self.HVPowerStatusValue.setStyleSheet("color:red")
            self.HVPowerStatusValue.setPixmap(self.redledpixmap)
        time.sleep(0.5)
        # Selecting default LV
        self.LVpowersupply.setPowerModel(defaultLVModel[0])
        self.LVpowersupply.setInstrument(defaultUSBPortLV[0])
        statusString = self.LVpowersupply.getInfo()
        self.LVPowerStatusLabel.setText("LV status")
        if statusString != "No valid device" and statusString != None:
            # self.LVPowerStatusValue.setStyleSheet("color:green")
            self.LVPowerStatusValue.setPixmap(self.greenledpixmap)
        else:
            # self.LVPowerStatusValue.setStyleSheet("color:red")
            self.LVPowerStatusValue.setPixmap(self.redledpixmap)

        firmwareName = defaultFC7

        LogFileName = "{0}/Gui/.{1}.log".format(os.environ.get("GUI_dir"), firmwareName)
        try:
            logFile = open(LogFileName, "w")
            logFile.close()
        except:
            QMessageBox(
                None, "Error", "Can not create log files: {}".format(LogFileName)
            )

        FwStatusComment, _, _ = self.master.getFwComment(firmwareName, LogFileName)
        if "Connected" in FwStatusComment:
            self.FC7StatusValue.setPixmap(self.greenledpixmap)
        else:
            self.FC7StatusValue.setPixmap(self.redledpixmap)

        self.FwModule = self.master.FwDict[firmwareName]

        # self.StatusList.append([self.FC7NameLabel,self.FC7StatusValue])

        # Arduino stuff
        # self.ArduinoGroup = ArduinoWidget()
        self.ArduinoGroup.stop.connect(self.master.GlobalStop)
        self.ArduinoGroup.createArduino()
        self.ArduinoGroup.enable()
        self.ArduinoGroup.setBaudRate(defaultSensorBaudRate)
        self.ArduinoGroup.frozeArduinoPanel()

        # self.ArduinoMonitorLabel = QLabel()
        # self.ArduinoMonitorValue = QLabel()
        if self.ArduinoGroup.ArduinoGoodStatus == True:
            self.ArduinoMonitorValue.setPixmap(self.greenledpixmap)
        else:
            self.ArduinoMonitorValue.setPixmap(self.redledpixmap)

        # self.StatusList.append([self.ArduinoMonitorLabel,self.ArduinoMonitorValue])

        # for index, items in enumerate(self.StatusList):
        # 	self.StatusLayout.addWidget(items[0], index, 1,  1, 1)
        # 	self.StatusLayout.addWidget(items[1], index, 2,  1, 2)

        # self.StatusLayout.addWidget(self.RefreshButton,len(self.StatusList) ,1, 1, 1)

        ######################################
        ## Testin some things out (end) #######
        ######################################
