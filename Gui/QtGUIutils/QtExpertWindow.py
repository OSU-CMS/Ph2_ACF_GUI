#!/usr/bin/env python3
"""Use to create the main expert window of the GUI."""
from PyQt5.QtCore import pyqtSignal
from PyQt5.QtWidgets import (
    QGridLayout,
    QGroupBox,
    QLabel,
    QPushButton,
    QWidget,
    QComboBox,
)
from Gui.GUIutils.DBConnection import check_database_connection
import Gui.Config.staticSettings as static_settings
import Gui.Config.siteSettings as settings
import Gui.Config.siteConfig as site_config
from Gui.python.Firmware import FC7
from Gui.GUIutils.FirmwareUtil import fwStatusParser
from Gui.GUIutils.GPIBInterface import PowerSupply
import os
import sys
import pyvisa as visa


class QtExpertWindow(QWidget):
    globalStop = pyqtSignal()

    def __init__(self, database_connection):
        """Use to intialize class."""
        super().__init__()
        self.main_layout = QGridLayout()
        self.setLayout(self.main_layout)
        self.connected_devices = {}
        self.fc7_boards_in_use = []
        self.processing_test = False
        self.database_connection = database_connection
        self.fc7_status_verbose_dict = {}
        self.fc7_dict = {}  # Dictionary of fc7 objects

        self.create_main_window()

        self.show()

    def create_main_window(self):
        """Use to create qt window for expert GUI."""
        statusString, colorString = check_database_connection(self.database_connection)

        self.FirmwareStatus = QGroupBox("Hello, User!")
        self.FirmwareStatus.setDisabled(False)

        DBStatusLabel = QLabel()
        DBStatusLabel.setText("Database connection:")
        DBStatusValue = QLabel()
        DBStatusValue.setText(statusString)
        DBStatusValue.setStyleSheet(colorString)

        self.connected_devices["Database"] = [DBStatusLabel, DBStatusValue]

        # Setup fc7 objects
        try:
            for index, (fc7_name, fc7_address) in enumerate(
                site_config.FC7List.items()
            ):
                fc7_name_label = QLabel()
                fc7_name_label.setText(fc7_name)
                fc7_status_value = QLabel()
                self.connected_devices[fc7_name] = [fc7_name_label, fc7_status_value]

                fc7 = FC7()
                fc7.setName(fc7_name)
                fc7.setIPAddress(fc7_address)
                fc7.setFPGAConfig(static_settings.FPGAConfigList[fc7_name])
                self.fc7_dict[fc7_name] = fc7
        except Exception as e:
            print("Problem checking firmware")
            print(e)

        self.use_buttons_dict = {}
        status_layout = QGridLayout()

        for index, key in enumerate(self.connected_devices):
            print("key", key)
            if key == "Database":
                self.check_button = QPushButton("&FC7 Check")
                # NOTE see if this is named right
                self.check_button.clicked.connect(self.checkFc7Status)
                self.check_button.clicked.connect(lambda: self.connectFc7(key))
                status_layout.addWidget(self.check_button, index, 0, 1, 1)
                status_layout.addWidget(self.connected_devices[key][0], index, 1, 1, 1)
                status_layout.addWidget(self.connected_devices[key][1], index, 2, 1, 2)

            else:
                use_button = QPushButton("&Use")
                use_button.setDisabled(True)
                use_button.toggle()
                # You subtract one since the database is taking an extra spot in the dictionary
                # but is not in the use_button list
                use_button.clicked.connect(lambda x=key: self.switchFc7(x))

                if sys.version.startswith(("3.11", "3.6", "3.7", "3.9")):
                    use_button.clicked.connect(self.update)
                # if self.PYTHON_VERSION.startswith("3.8"):
                #     use_button.clicked.connect(self.destroyMain)
                #     use_button.clicked.connect(self.createMain)
                #     use_button.clicked.connect(self.checkFirmware)
                use_button.setCheckable(True)
                self.use_buttons_dict[key] = use_button
                # Adds use button and fc7 strings to GUI
                status_layout.addWidget(use_button, index, 0, 1, 1)
                status_layout.addWidget(self.connected_devices[key][0], index, 1, 1, 1)
                status_layout.addWidget(self.connected_devices[key][0], index, 1, 1, 1)

                self.FirmwareStatus.setLayout(status_layout)

                self.main_layout.addWidget(self.FirmwareStatus, 0, 0, 1, 1)

                self.connect_devices_groupbox = QGroupBox()
                connect_devices_layout = QGridLayout()

                connect_devices_button = QPushButton("&Connect All Devices")
                connect_devices_label = QLabel("")

                connect_devices_layout.addWidget(connect_devices_button, 0, 0)
                connect_devices_layout.addWidget(connect_devices_button, 0, 1)

                hv_power_supply_groupbox = QGroupBox("HV Power Supply")
                hv_power_supply_layout = QGridLayout()

                self.hv_port_combo = QComboBox()
                self.hv_port_label = QLabel("Choose HV Port:")
                self.hv_port_combo.addItems(
                    visa.ResourceManager("@py").list_resources()
                )
                self.hv_model_label = QLabel("Choose HV Power Model: ")
                self.hv_model_combo = QComboBox()
                self.hv_model_combo.addItems(static_settings.HVPowerSupplyModel.keys())
                self.hv_power_status_label = QLabel()
                self.hv_use_button = QPushButton("&Use")
                self.hv_use_button.clicked.connect(self.connect_to_hv_powersupply)
                self.hv_release_button = QPushButton("&Release")
                self.hv_release_button.clicked.connect(self.release_hv_powersupply)

                hv_power_supply_layout.addWidget(self.hv_port_label, 0, 0)
                hv_power_supply_layout.addWidget(self.hv_port_combo, 0, 1)
                hv_power_supply_layout.addWidget(self.hv_model_label, 0, 2)
                hv_power_supply_layout.addWidget(self.hv_model_combo, 0, 3)
                hv_power_supply_layout.addWidget(self.hv_use_button, 0, 4)
                hv_power_supply_layout.addWidget(self.hv_release_button, 0, 5)

                hv_power_supply_groupbox.setLayout(hv_power_supply_layout)

                lv_power_supply_groupbox = QGroupBox("LV Power Supply")
                lv_power_supply_layout = QGridLayout()
                self.lv_port_combo = QComboBox()
                self.lv_port_label = QLabel("Choose LV Port:")
                self.lv_port_combo.addItems(
                    visa.ResourceManager("@py").list_resources()
                )
                self.lv_model_label = QLabel("Choose LV Power Model: ")
                self.lv_model_combo = QComboBox()
                self.lv_model_combo.addItems(static_settings.LVPowerSupplyModel.keys())
                self.lv_power_status_label = QLabel()
                self.lv_use_button = QPushButton("&Use")
                self.lv_use_button.clicked.connect(self.connect_to_lv_powersupply)
                self.lv_release_button = QPushButton("&Release")
                self.lv_release_button.clicked.connect(self.release_lv_powersupply)

                lv_power_supply_layout.addWidget(self.lv_port_label, 0, 0)
                lv_power_supply_layout.addWidget(self.lv_port_combo, 0, 1)
                lv_power_supply_layout.addWidget(self.lv_model_label, 0, 2)
                lv_power_supply_layout.addWidget(self.lv_model_combo, 0, 3)
                lv_power_supply_layout.addWidget(self.lv_use_button, 0, 4)
                lv_power_supply_layout.addWidget(self.lv_release_button, 0, 5)

                lv_power_supply_groupbox.setLayout(lv_power_supply_layout)
                self.connect_devices_groupbox.setLayout(connect_devices_layout)
                self.main_layout.addWidget(hv_power_supply_groupbox, 1, 0)
                self.main_layout.addWidget(lv_power_supply_groupbox, 2, 0)

                connect_devices_button.clicked.connect(self.connect_to_default_devices)

    def connect_to_default_devices(self) -> None:
        """
        Use to connect all devices that are predefined in the siteConfig.py file

        Will set combobox selection to the default power supply defined in siteConfig.py then
        use this value to connect to the power supplies.
        """
        hv_index = self.hv_port_combo.findText(site_config.defaultUSBPortHV[0])
        if hv_index != -1:
            self.hv_port_combo.setCurrentIndex(hv_index)

        lv_index = self.lv_port_combobox.findText(site_config.defaultUSBPortLV[0])
        if lv_index != -1:
            self.lv_port_combobox.setCurrentIndex(lv_index)

        self.connect_to_hv_powersupply()
        self.connect_to_lv_powersupply()

    def connect_to_hv_powersupply(self):
        self.hv_powersupply = PowerSupply()

        # Need to have just keithley or just keysight, not whole string
        self.hv_powersupply.setPowerModel(self.hv_model_combo.currentText().split()[0])
        self.hv_powersupply.setPortAddress(self.hv_port_combo.currentText())
        self.hv_powersupply.TurnOffHV()

        status_string = self.hv_powersupply.getInfo()
        if status_string != "No valid device" and status_string != None:
            self.hv_power_status_value.setStyleSheet("color:green")
            self.hv_power_combo.setDisabled(True)
            self.hv_power_status_value.setText(status_string)
            self.use_hv_button.setDisabled(True)
            self.release_hv_button.setDisabled(False)
        else:
            self.hv_power_status_value.setStyleSheet("color:red")
            self.hv_power_status_value.setText("No valid device")

    def release_hv_powersupply(self):
        self.hv_powersupply.TurnOffHV()
        try:
            self.hv_port_combo.setDisabled(False)
            self.hv_power_status_label.setText("")
            self.hv_use_button.setDisabled(False)
            self.hv_release_button.setDisabled(True)
        except Exception as e:
            print(f"HV not released properly due to error: {e}")

    def release_lv_powersupply(self):
        self.lv_powersupply.TurnOff()
        try:
            self.lv_port_combo.setDisabled(False)
            self.lv_power_status_label.setText("")
            self.lv_use_button.setDisabled(False)
            self.lv_release_button.setDisabled(True)
        except Exception as e:
            print(f"LV not released properly due to error: {e}")

    def connect_to_lv_powersupply(self):
        self.lv_powersupply = PowerSupply()
        self.lv_powersupply.setPowerModel(self.lv_model_combo.currentText().split()[0])
        self.lv_powersupply.setPortAddress(self.lv_port_combo.currentText())
        self.lv_powersupply.TurnOff()
        status_string = self.lv_powersupply.getInfo()
        if status_string != "No valid device":
            self.lv_power_status_value.setStyleSheet("color:green")
        else:
            self.lv_power_status_value.setStyleSheet("color:red")
        if status_string:
            self.lv_port_combo.setDisabled(True)
            self.lv_power_status_value.setText(statusString)
            self.use_lv_button.setDisabled(True)
            self.release_lv_button.setDisabled(False)
        else:
            self.lv_power_status_value.setStyleSheet("color:red")
            self.lv_power_status_value.setText("No valid device")

    def switch_fc7(self, index: int) -> None:
        """Use to"""
        if self.use_buttons_dict[index].isChecked():
            self.occupy

    def get_fc7_comment(self, fc7_name, file_name):
        comment, color, verbose_comment = fwStatusParser(
            self.fc7_dict[fc7_name], file_name
        )
        return comment, color, verbose_comment

    def getIndex(self, element, list_2d):
        for index, item in enumerate(list_2d):
            if item[0].text() == element:
                return index - 1
        return -1

    def checkFC7(self):
        for index, fc7_name in enumerate(site_config.FC7List.keys()):
            file_name = "{0}/Gui/.{1}.log".format(os.environ.get("GUI_dir"), fc7_name)
            if fc7_name not in self.fc7_boards_in_use:
                (
                    fc7_status_comment,
                    fc7_status_color,
                    fc7_status_verbose,
                ) = self.get_fc7_comment(fc7_name, file_name)
                self.connected_devices[fc7_name][1].setText(fc7_status_comment)
                self.connected_devices[fc7_name][1].setStyleSheet(fc7_status_color)
                self.fc7_status_verbose_dict[fc7_name] = fc7_status_verbose

            self.use_buttons_dict[index].setDisabled(False)

        for board in self.fc7_boards_in_use:
            self.connected_devices[board][1].setText("Connected")
            self.connected_devices[board][1].setStyleSheet("color: green")
            self.useFc7(board)

    def switchFc7(self, key: str) -> None:
        if self.use_buttons_dict[key].isChecked():
            self.useFc7(key)
        else:
            self.releaseFc7(key)
            self.checkFirmware()

    def useFc7(self, fc7_board_name: str) -> None:
        if self.connected_devices[fc7_board_name][1].text() == "Connected":
            self.new_test_button.setDisabled(False)
            self.use_buttons_dict[fc7_board_name].setChecked(True)
            self.use_buttons_dict[fc7_board_name].setText("&In use")
            self.use_buttons_dict[fc7_board_name].setDisabled(False)
            self.fc7_boards_in_use.append(fc7_board_name)

    def releaseFc7(self, fc7_board_name: str) -> None:
        self.fc7_boards_in_use.remove(fc7_board_name)
        self.use_buttons_dict[fc7_board_name].setText("&Use")
        self.use_buttons_dict[fc7_board_name].setDown(False)
        self.use_buttons_dict[fc7_board_name].setDisabled(False)
        self.check_button.setDisabled(False)
        self.new_test_button.setDisabled(True)

    def checkFc7Status(self):
        for fc7_name in site_config.FC7List.keys():
            file_name = "{0}/Gui/.{1}.log".format(os.environ.get("GUI_dir"), fc7_name)
            if fc7_name not in self.fc7_boards_in_use:
                (
                    fc7_status_comment,
                    fc7_status_color,
                    fc7_status_verbose,
                ) = self.get_fc7_comment(fc7_name, file_name)
                self.connected_devices[fc7_name][1].setText(fc7_status_comment)
                self.connected_devices[fc7_name][1].setStyleSheet(fc7_status_color)

    def connectFc7(self, fc7_name: str) -> None:
        print("connectfc7", fc7_name)
        self.connected_devices[fc7_name][1].setText("Connected")
        self.connected_devices[fc7_name][1].setStyleSheet("color: green")
        self.useFc7(fc7_name)
