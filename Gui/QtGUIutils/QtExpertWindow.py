#!/usr/bin/env python3
"""Use to create the main expert window of the GUI."""
from PyQt5.QtCore import pyqtSignal
from PyQt5.QtWidgets import QGridLayout, QGroupBox, QLabel, QPushButton
from Gui.GUIutils.DBConnection import check_database_connection
import Gui.Config.staticSettings as static_settings
from Gui.python.Firmware import FC7


class QtExpertWindow:
    globalStop = pyqtSignal()

    def __init__(self, dimension, database_connection):
        """Use to intialize class."""
        super().__init__()
        self.main_layout = QGridLayout()
        self.setLayout(self.main_layout)
        self.processing_test = False
        self.database_connection = database_connection

        self.create_window()

    def create_window(self):
        """Use to create qt window for expert GUI."""
        statusString, colorString = check_database_connection(self.database_connection)

        self.FirmwareStatus = QGroupBox("Hello,{}!".format(self.TryUsername))
        self.FirmwareStatus.setDisabled(True)

        DBStatusLabel = QLabel()
        DBStatusLabel.setText("Database connection:")
        DBStatusValue = QLabel()
        DBStatusValue.setText(statusString)
        DBStatusValue.setStyleSheet(colorString)

        self.status_list = []
        self.status_list.append([DBStatusLabel, DBStatusValue])

        try:
            for index, (fc7_name, fc7_address) in enumerate(
                static_settings.fc7_list.items
            ):
                fc7_name_label = QLabel()
                fc7_name_label.setText(fc7_name)
                fc7_status_value = QLabel()
                self.status_list.append([fc7_name_label, fc7_status_value])
                self.fc7_status_verbose_dict[str(fc7_name)] = {}

                fc7 = FC7()
                fc7.setName(fc7_name)
                fc7.setIPAdress(static_settings.fc7_list[fc7_name])
                fc7.setFPGAConfig(static_settings.FPGAConfigList[fc7_name])
                self.fc7_dict[fc7_name] = fc7
        except Exception as e:
            print("Problem checking firmware")
            print(e)

        self.use_buttons = []
        status_layout = QGridLayout()

        for index, items in enumerate(self.status_list):
            if index == 0:
                self.check_button = QPushButton("&Fw Check")
                # NOTE see if this is named right
                self.check_button.clicked.connect(self.checkFirmware)
                status_layout.addWidget(self.check_button, index, 0, 1, 1)
                status_layout.addWidget(items[0], index, 1, 1, 1)
                status_layout.addWidget(items[1], index, 2, 1, 2)

            else:
                use_button = QPushButton("&Use")
                use_button.setDisabled(True)
                use_button.toggle()
                use_button.clicked.connect(lambda x=str(index-1): self.switchFw(x))

                if self.PYTHON_VERSION.startswith(("3.6", "3.7", "3.9")):
                    use_button.clicked.connect(self.update)
                if self.PYTHON_VERSION.startswith("3.8"):
                    use_button.clicked.connect(self.destroyMain)
                    use_button.clicked.connect(self.createMain)
                    use_button.clicked.connect(self.checkFirmware)
                use_button.setCheckable(True)
                self.use_buttons.append(use_button)
                # Adds use button and fc7 strings to GUI
                status_layout.addWidget(use_button, index, 0, 1, 1)
                status_layout.addWidget(items[0], index, 1, 1, 1)
                status_layout.addWidget(items[0], index, 1, 1, 1)

                fpga_config_button = QPushButton("&Change uDTC firmware")
                fpga_config_button.clicked.connect(lambda x=str(index-1): self.setuDTCFw(x))
                status_layout.addWidget(fpga_config_button, index, 4, 1, 1)
                # NOTE: This may need named incorrectly, may be fc7
                firmware_status_button = QPushButton("&Firmware Status")
                firmware_status_button.clicked.connect(lambda x=str(index-1): self.showCommentFw(x))
                status_layout.addWidget(firmware_status_button, index, 5, 1, 1)
                log_button = QPushButton("&Log")
                log_button.clicked.connect(lambda x=str(index-1): self.showLogFw(x))

                status_layout.addWidget(log_button, index, 6, 1, 1)

    def switch_fc7(self, index):
        """Use to """
