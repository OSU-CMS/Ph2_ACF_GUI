#!/usr/bin/env python3
from PyQt5 import QtCore
from PyQt5.QtCore import pyqtSignal
from Gui.QtGUIutils.QtLoginDialog import QtLoginDialog
from Gui.GUIutils.DBConnection import connect_to_database
from Gui.QtGUIutils.QtExpertWindow import QtExpertWindow
from Gui.QtGUIutils.QtSimplifiedMainWidget import QtSimplifiedMainWidget
from Gui.GUIutils.GPIBInterface import PowerSupply
from PyQt5.QtWidgets import QWidget


class QtApplication(QWidget):
    globalStop = pyqtSignal()

    def __init__(self):
        """Load all necessary objects into this function to improve communication between widgets"""
        super().__init__()
        self.database_connection = None
        self.ProcessingTest = False
        self.hv_powersupply = PowerSupply(powertype="HV")
        self.lv_powersupply = PowerSupply(powertype="LV")
        self.fc7_dict = {}

        self.login = QtLoginDialog()
        self.login.login_signal.connect(self.gui_login)
        self.login.exec_()

    def gui_login(self, connection_parameters):
        """Login to either expert or simplified gui depending on QtLoginDialog output"""
        self.connection_parameters = connection_parameters
        print("IN gui_login")
        self.database_connection = connect_to_database(
            connection_parameters["username"],
            connection_parameters["password"],
            connection_parameters["address"],
            connection_parameters["database"],
        )
        print("DATABASE: ", self.database_connection)
        if connection_parameters["expert"]:
            self.login.close()
            self.expert_window = QtExpertWindow(self)
            self.simple_window = None
        else:
            self.login.close()
            self.simple_window = QtSimplifiedMainWidget(self)
            self.expert_window = None

    def releaseLVPowerPanel(self):
        self.lv_powersupply.TurnOff()
        self.expert_window.lv_port_combo.setDisabled(False)
        self.expert_window.lv_power_status_label.setText("")
        self.expert_window.lv_use_button.setDisabled(False)
        self.expert_window.lv_release_button.setDisabled(True)
        self.LVPowerList = self.LVpowersupply.listResources()

    @QtCore.pyqtSlot()
    def GlobalStop(self):
        """Turn off power supplies"""
        print("Critical status detected: Emitting Global Stop signal")
        self.globalStop.emit()
        self.HVpowersupply.TurnOffHV()
        self.LVpowersupply.TurnOff()
        if self.expertMode == True:
            self.releaseHVPowerPanel()
            self.releaseLVPowerPanel()


def getFwComment(self, firmwareName, fileName):
    comment, color, verboseInfo = fwStatusParser(self.FwDict[firmwareName], fileName)
    return comment, color, verboseInfo
