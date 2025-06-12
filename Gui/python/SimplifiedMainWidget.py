import os
import time
from serial import SerialException
from typing import Optional, Callable, Union
import requests
from bs4 import BeautifulSoup

from Gui.QtGUIutils.QtStartWindow import SummaryBox
from PyQt5.QtCore import Qt, QSize, pyqtSignal, QObject, QThread, pyqtSlot
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

from Gui.GUIutils.FirmwareUtil import fwStatusParser
from Gui.python.CustomizedWidget import SimpleBeBoardBox
from Gui.python.Firmware import QtBeBoard
from Gui.python.ArduinoWidget import ArduinoWidget
from Gui.python.Peltier import PeltierSignalGenerator
from Gui.python.logging_config import logger
import Gui.siteSettings as site_settings # type: ignore
from icicle.icicle.instrument_cluster import InstrumentNotInstantiated


class SimplifiedMainWidget(QWidget):
    abort_signal = pyqtSignal()
    close_signal = pyqtSignal()
    config_and_test_Signal = pyqtSignal()

    def __init__(self, master, dimension):
        logger.debug("SimplifiedMainWidget.__init__()")
        super().__init__()
        self.master = master
        self.dimension = dimension

        try:
            self.instruments = master.instruments
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

        greenledimage = QImage("icons/led-green-on.png").scaled(
            QSize(60, 10), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        self.greenledpixmap = QPixmap.fromImage(greenledimage)

        self.config_and_test_Signal.connect(self.config_and_test)

        self.createWindow()

    def createWindow(self):

        self.monitoring_values = [
            "temperature", "database", "hv", "lv", "condensation_risk"
        ] + [f"fc7_{firmwareName}" for firmwareName in site_settings.FC7List.keys()]

        self.create_monitoring_leds()

        self.setDeviceStatus()

        self.setupMonitoring()

        self.setupLogFile()

        self.setupBeBoard()

        self.setupUI()

        self.RunButtonState()

    def config_and_test(self):
        self.master.RunNewTest.resetConfigTest()
        self.master.RunNewTest.initialTest()

    def setupBeBoard(self):
        # self.BeBoard.setFPGAConfig(default_settings.FPGAConfigList[site_settings.defaultFC7])
        # logger.debug(f"Default FC7: {site_settings.defaultFC7}")
        logger.debug("Initialized BeBoard in SimplifiedGUI")
        self.BeBoardWidget = SimpleBeBoardBox(self.master, self.firmware)
        logger.debug("Initialized SimpleBeBoardBox in Simplified GUI")

    def setupCondensationRiskMonitoring(self):
        self.instrument_info["condensationRisk"] = {
            "Label": QLabel(), "Value": QLabel()}
        if site_settings.cooler == "Tessie":  # Arduino is now a misnomer
            self.instrument_info["condensationRisk"]["Label"].setText(
                "Condensation Risk")
        else:
            self.ArduinoGroup = ArduinoWidget()
            self.ArduinoGroup.stop.connect(self.abort_signal.emit)
            self.ArduinoGroup.enable()
            self.ArduinoGroup.setBaudRate(site_settings.defaultSensorBaudRate)
            self.ArduinoGroup.setPort(site_settings.defaultArduino)
            self.ArduinoGroup.frozeArduinoPanel()
            self.instrument_info["arduino"]["Label"].setText(
                "Condensation Risk")

    def setupLogFile(self):
        for firmwareName in site_settings.FC7List.keys():
            LogFileName = "{0}/Gui/.{1}.log".format(
                os.environ.get("GUI_dir"), firmwareName
            )
            logger.debug(f"FC7 log file saved to {LogFileName}")

            try:
                logFile = open(LogFileName, "w")
                logFile.close()
            except Exception as e:
                messageBox = QMessageBox()
                messageBox.setIcon(QMessageBox.Error)
                logger.error("Could not create file due to error {}".format(e))
                messageBox.setText(
                    "Can not create log files: {}".format(LogFileName))
                messageBox.exec()

    def setupColdbox(self) -> None:
        try:
            self.instrument_info["cooler"] = {
                "Label": QLabel(), "Value": QLabel()}
            self.instrument_info["cooler"]["Label"].setText("Cooler Status")
            logger.debug("Setting up Cooler")

            # Check for coldbox interlock
            # NOTE: Check this, don't know if I need to worry about other indices
            # I also don't know if INTERLOCK == 1 is good or bad
            self.cooler = self.instruments.get_cb()[0]
            if int(self.cooler.interlocked()):  
                # If coldbox interlock is safe, begin cooling
                self.cooler.on() # Set cooler to default temperature
                
            # Monitor the temperature of all "in-use" modules and
            

            # change status light if any are not at the desired temperature
        except:
            ...
    def setupColdbox(self):
        """
        Setup turn on the coldbox and set the default temperature. This function will take
        a while to run so it should be run in a separate thread.
        """
        try: 

    def setupPeltier(self):
        try:
            self.instrument_info["peltier"] = {
                "Label": QLabel(), "Value": QLabel()}
            self.instrument_info["peltier"]["Label"].setText(
                "Peltier Temperature")
            logger.debug("Setting up Peltier")
            self.Peltier = PeltierSignalGenerator()
            assert self.Peltier is not None, "Peltier object was not created"
            logger.debug("created self.Peltier")
            # These should emit signals
            if not self.Peltier.sendCommand(
                self.Peltier.createCommand(
                    "Set Type Define Write", [
                        "0", "0", "0", "0", "0", "0", "0", "0"]
                )
            )[1]:
                raise Exception("Could not communicate with Peltier")
            logger.debug("Execute Peltier write command")

            # Allows set point to be set by computer software
            if not self.Peltier.sendCommand(
                self.Peltier.createCommand(
                    "Control Type Write", [
                        "0", "0", "0", "0", "0", "0", "0", "1"]
                )
            )[1]:
                raise Exception(
                    "Could not communicate with Peltier"
                )  # Temperature should be PID controlled
            logger.debug("Executed Peltier PID command")

            message = self.Peltier.convertSetTempValueToList(
                site_settings.defaultPeltierSetTemp
            )

            self.Peltier.sendCommand(
                self.Peltier.createCommand(
                    "Fixed Desired Control Setting Write", message
                )
            )
            logger.debug("Set peltier temp")
            if not self.Peltier.sendCommand(
                self.Peltier.createCommand(
                    "Power On/Off Write", ["0", "0",
                                           "0", "0", "0", "0", "0", "1"]
                )
            )[1]:
                # Turn on Peltier
                raise Exception("Could not communicate with Peltier")

            if not self.Peltier.sendCommand(
                self.Peltier.createCommand(
                    "Proportional Bandwidth Write",
                    ["0", "0", "0", "0", "0", "0", "c", "8"],
                )
            )[1]:
                raise Exception(
                    "Could not communicate with Peltier"
                )  # Set proportional bandwidth
            logger.debug("Set Peltier Bandwidth")
            time.sleep(0.5)

            self.peltier_temperature_label = QLabel(self)
        except Exception as e:
            print("Error while attempting to set Peltier", e)
            self.Peltier = None


    def setupUI(self):
        self.simplifiedStatusBox = QGroupBox(
            "Hello, {}!".format(self.master.operator_name_first)
        )
        self.StatusLayout = QGridLayout()
        self.StatusLayout.addWidget(
            self.instrument_info["database"]["Label"], 0, 1, 1, 1
        )
        self.StatusLayout.addWidget(
            self.instrument_info["database"]["Value"], 0, 2, 1, 1
        )
        self.StatusLayout.addWidget(
            self.instrument_info["hv"]["Label"], 0, 3, 1, 1)
        self.StatusLayout.addWidget(
            self.instrument_info["hv"]["Value"], 0, 4, 1, 1)

        self.StatusLayout.addWidget(
            self.instrument_info["lv"]["Label"], 1, 1, 1, 1)
        self.StatusLayout.addWidget(
            self.instrument_info["lv"]["Value"], 1, 2, 1, 1)

        offset = -1
        for index, firmwareName in enumerate(site_settings.FC7List.keys()):
            self.StatusLayout.addWidget(
                self.instrument_info[f"fc7_{firmwareName}"]["Label"], 1 +
                index, 3, 1, 1
            )
            self.StatusLayout.addWidget(
                self.instrument_info[f"fc7_{firmwareName}"]["Value"], 1 +
                index, 4, 1, 1
            )
            offset += 1

        self.StatusLayout.addWidget(
            self.instrument_info["condensationRisk"]["Label"], 2, 1, 1, 1
        )
        self.StatusLayout.addWidget(
            self.instrument_info["condensationRisk"]["Value"], 2, 2, 1, 1
        )
        self.StatusLayout.addWidget(
            self.instrument_info["temperature"]["Label"], 2 + offset, 3, 1, 1
        )
        self.StatusLayout.addWidget(
            self.instrument_info["temperature"]["Value"], 2 + offset, 4, 1, 1
        )
        
        # self.StatusLayout.addWidget(self.RefreshButton, 3, 3, 1, 1)
        logger.debug("Setup StatusLayout")
        ModuleEntryLayout = QGridLayout()
        ModuleEntryLayout.addWidget(self.BeBoardWidget)
        logger.debug("Setup ModuleEntryLayout")

        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()
        self.TestGroup = QGroupBox()
        self.TestGroupLayout = QVBoxLayout()
        self.FunctionTestButton = QRadioButton("&Functional Test")
        self.AssemblyTestButton = QRadioButton("&Assembly QC Test")
        self.FullPerformanceTestButton = QRadioButton("&Full Performance Test")
        self.FunctionTestButton.setChecked(True)
        self.TestGroupLayout.addWidget(self.FunctionTestButton)
        self.TestGroupLayout.addWidget(self.AssemblyTestButton)
        self.TestGroupLayout.addWidget(self.FullPerformanceTestButton)

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
        self.RunButton.clicked.connect(self.config_and_test)
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

    def create_monitoring_leds(self) -> None:
        """
        Create the monitoring LEDs for the simplified GUI.
        These LEDs will be used to indicate the status of the instruments.
        """
        for monitor_led in self.monitoring_values:
            self.instrument_info[monitor_led] = {
                "Label": QLabel(),
                "Value": QLabel(),
            }
            self.instrument_info[monitor_led]["Label"].setText(
                monitor_led.replace("_", " ").capitalize()
            )
            self.instrument_info[monitor_led]["Value"].setPixmap(
                self.redledpixmap
            )
          
    def updateMonitoringLED(self, monitor_led:str, status:bool)->None:
        """
        Modify the status of the LED on the simplified widget.

        Arguments:
        monitor_led:
        The LED that you would like to change the status of. Currently the following
        values are available ("lv", "database", "hv", "fc7_<firmware_name>", "environment"

        status: What status you would like to change the LED to False -> red, True -> green
        """
        if status:
            self.instrument_info[monitor_led]["Value"].setPixmap(self.greenledpixmap)
        else:
            self.instrument_info[monitor_led]["Value"].setPixmap(self.redledpixmap)

        
    def updateTemperatureMonitoring(self, temperature: Union[float, list[float]]) -> None:
        """
        Validate temperature of cooling method and update environment 
        """
        set_temp =  site_settings.defaultPeltierSetTemp if site_settings.cooler == "Peltier" else cb.default_temp
        if isinstance(temperature, list):
            status = all(abs(x - cb.default_temperature) < 5 for x in temperature)
        else:
            status = abs(x - cb.default_temperature) < 5

        self.updateMonitoringLED("temperature", status)

        
    def updateArduinoIndicator(self) -> bool:
        if site_settings.cooler == "Tessie":
            try:
                soup = BeautifulSoup(requests.get(
                    'http://coldboxx:3000/').text, 'html.parser')
            except requests.exceptions.ConnectionError as e:
                logger.error(e)
                self.instrument_info["arduino"]["Value"].setText(
                    "<span style='color:red;'>Can't connect to coldbox</span>")
                return None
            except Exception as e:
                print(type(e))
                logger.error(e)
                self.instrument_info["arduino"]["Value"].setText(
                    "<span style='color:red;'>Error: No data</span>")
                return None

            circle = soup.find('circle', id='circleG')
            style = circle.get('style')
            style.split
            for part in style.split(';'):
                if 'fill:' in part:
                    fill_value = part.split(':')[1].strip()
                    if fill_value == "green":
                        self.instrument_info["arduino"]["Value"].setPixmap(
                            self.greenledpixmap)
                        return True
                    else:
                        self.instrument_info["arduino"]["Value"].setPixmap(
                            self.redledpixmap)
                        return False
        else:
            if self.ArduinoGroup.condensationRisk:
                self.instrument_info["arduino"]["Value"].setPixmap(
                    self.redledpixmap)
            else:
                self.instrument_info["arduino"]["Value"].setPixmap(
                    self.greenledpixmap)

    def updatePeltierTemp(self, temp: float):
        self.peltier_temperature_label.setText("{}C".format(temp))
        if abs(temp - site_settings.defaultPeltierSetTemp) < 15:
            self.instrument_info["peltier"]["Value"].setPixmap(
                self.greenledpixmap)
        else:
            self.instrument_info["peltier"]["Value"].setPixmap(
                self.redledpixmap)

    def runNewTest(self):
        for module in self.BeBoardWidget.getModules():
            if module.getSerialNumber() == "":
                QMessageBox.information(
                    None, "Error", "No valid serial number!", QMessageBox.Ok
                )
                return
            if module.getID() == "":
                QMessageBox.information(
                    None, "Error", "No valid ID!", QMessageBox.Ok)
                return

        self.firmwareDescription, message = self.BeBoardWidget.getFirmwareDescription()
        if (
            not self.firmwareDescription
        ):  # firmware description returns none if no modules are entered
            QMessageBox.information(
                None,
                "Error",
                message,
                QMessageBox.Ok,
            )
            return

        if self.FunctionTestButton.isChecked():
            self.info = "TFPX_Functional_Test"
        elif self.AssemblyTestButton.isChecked():
            self.info = "TFPX_Assembly_QC"
        elif self.FullPerformanceTestButton.isChecked():
            self.info = "TFPX_FullPerformance_Test"
        self.runFlag = True
        self.RunButton.setDisabled(True)
        self.StopButton.setDisabled(False)

        module = self.firmwareDescription[0].getModules()[0]
        module_type = module.getModuleType()
        self.master.module_in_use = module_type

        print("Firmware Description")
        for beboard in self.firmwareDescription:
            print(beboard)

        print("Firmware Check")
        for beboard in self.firmwareDescription:
            SummaryBox.checkFwPar(
                beboard.getBoardName(), module_type, beboard.getIPAddress()
            )

        self.master.openRunWindowSignal.emit(
            self.info, self.firmwareDescription, {})
        self.config_and_test_Signal.emit()

    def abortTest(self):
        self.master.RunNewTest.abortTest()
        self.StopButton.setDisabled(True)
        self.RunButton.setDisabled(False)

    def setupMonitoring(self):
        """ 
        Setup variables that will be constantly polled.  
        """
        # Launch QThread to monitor Peltier temperature/power and Arduino temperature/humidity
        thread = QThread()

        if site_settings.usePeltier:
            worker = Environment_Monitoring(monitor_peltier)
        else:
            worker = Environment_Monitoring(lambda: monitor_coldbox(self.coldbox))

        worker.moveToThread(thread)
        thread.started.connect(worker.run)
        if site_settings.usePeltier:
            worker.temp.connect(self.updatePeltierTemp)
        worker.temp.connect(self.updateArduinoIndicator)
        thread.start()

    def setDeviceStatus(self) -> None:
        """
        Set initial status for all monitored values

        The qualifications for a passing status are
        HV  -> HV is on and connected as stated by InstrumentCluster.status()
        LV  -> LV is on and connected as stated by InstrumentCluster.status()
        Arduino -> Can read correctly from the Arduino sensor as defined in ArduinoWidget.py
        Database -> Check if you can connect to database as defined in checkDBConnection()
        Peltier -> check if the Peltier is at the right temperature and is reachable
        """

        self.instrument_status = {key: False for key in self.monitoring_values}

        logger.debug("Getting FC7 Comment")
        self.firmware = []
        for firmwareName, ipaddress in site_settings.FC7List.items():
            LogFileName = "{0}/Gui/.{1}.log".format(
                os.environ.get("GUI_dir"), firmwareName
            )
            BeBoard = QtBeBoard(
                BeBoardID=str(len(self.firmware)),
                boardName=firmwareName,
                ipAddress=ipaddress,
            )
            FwStatusComment, _, _ = fwStatusParser(BeBoard, LogFileName)
            if FwStatusComment == "Connected":
                self.firmware.append(BeBoard)
                self.instrument_status[f"fc7_{firmwareName}"] = True

        logger.debug("Checking DB Connection")
        self.instrument_status["database"] = self.master.panthera_connected

        logger.debug("Setting up instrument_status")
        logger.debug("instrument_status: {}".format(self.instrument_status))

        if self.instruments:
            logger.debug(
                f"{__name__} Setup instrument status {self.instrument_status}")
            for key, value in self.instrument_info.items():
                if self.instrument_status[key]:
                    value["Value"].setPixmap(self.greenledpixmap)
                else:
                    value["Value"].setPixmap(self.redledpixmap)
        else:
            for key, value in self.instrument_info.items():
                value["Value"].setPixmap(self.redledpixmap)
        logger.debug(f"{__name__} Setup led labels")
        logger.debug(f"Instrument status: {self.instrument_status}")

        # These will be polled by the worker thread so just set to False by default
        self.instrument_status["temperature"] = False
        self.instrument_status["condensationRisk"] = False

        # If using coldbox, turn on here and set LED based on presence of errors
        if site_settings.cooler == "Tessie":
            try: 
                self.instruments.cb_on()

            # TODO: Handle exceptions more gracefully to differentiate between
            # failure to set humidity and failure to set temperature
            except Exception as e:
                logger.error(f"Error setting up coldbox: {e}")
                self.instrument_status["temperature"] = False
                self.instrument_info["temperature"]["Value"].setPixmap(
                    self.redledpixmap)
                self.instrument_status["condensationRisk"] = False
                self.instrument_info["condensationRisk"]["Value"].setPixmap(
                    self.redledpixmap)

    def RunButtonState(self):
        """
        Check the status of LV, HV, and FC7, and disable the Run button
        if any of these statuses are False.
        """
        try:
            # Check FC7 statuses
            fc7_status = any(self.instrument_status.get(
                f"fc7_{firmwareName}", False) for firmwareName in site_settings.FC7List.keys())
            logger.debug(f"FC7 status: {fc7_status}")

            # Disable the Run button if any status is False
            # if self.instruments is False then icicle failed to connect to LV and HV
            if not (self.instruments and fc7_status):
                self.RunButton.setDisabled(True)
                logger.debug(
                    "RunButton disabled due to one or more failing statuses.")
            else:
                self.RunButton.setDisabled(False)
                logger.debug("RunButton enabled. All statuses are good.")

        except Exception as e:
            logger.error(f"Error while checking instrument statuses: {e}")
            self.RunButton.setDisabled(True)

    def check_icicle_devices(self) -> Optional[dict[str, int]]:
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
            elif type(value) is InstrumentNotInstantiated:
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



def monitor_coldbox(coldbox) -> Optional[list[float]]:
    """
    Monitor the coldbox temperature and emit signals for each module.
    This function is intended to be run in a separate thread.

    Parameters:
    coldbox: The coldbox object to monitor.
    Returns:
    A list of temperatures for each module in the coldbox, or None if communication fails.
    """
    try:
        temperatures = coldbox.query("TEMPERATURE_MEASURED")
        if temperatures:
            # Emit the temperature signal with the current temperatures
            return temperatures
        else:
            logger.warning("No temperatures received from coldbox.")
            return None
    except Exception as e:
        logger.error(f"Error monitoring coldbox: {e}")
        return None

def monitor_peltier() -> Optional[float]:
    """
    Poll the Peltier for its current temperature in degrees Celsius.
    Returns float temperature or None if communication fails.
    """
    peltier = PeltierSignalGenerator()
    peltier_temp_message, temp_message_pass = peltier.sendCommand(
        peltier.createCommand(
            "Input1", ["0", "0", "0", "0", "0", "0", "0", "0"]
        )
    )
    if not temp_message_pass:
        peltier_temp_message = None
    logger.debug("Formatting peltier output")
    if peltier_temp_message:
        # Convert the hex string to an integer and divide by 100 to get the temperature 
        peltier_temp = int(
            "".join(peltier_temp_message[1:9]), 16) / 100
    else:
        peltier_temp = None

    return peltier_temp


class FunctionRunner(QObject):
    """
    A worker class to monitor the environment, specifically the Peltier temperature and 
    coldbox temperature.
    """
    finished = pyqtSignal(object) # Can be float or list of floats or None

    def __init__(self, func:Callable[[], Union[float, list[float], None]], 
                 *args, **kwargs):
        super().__init__()
        # Delay in seconds between polling
        self.func = func
        self.args = args
        self.kwargs = kwargs

    @pyqtSlot()
    def run(self):
        try:
            result = self.func(*self.args, **self.kwargs)
        except Exception as e:
            logger.error(f"Error in FunctionRunner: {e}")
            result = e 
        self.finished.emit(result)
