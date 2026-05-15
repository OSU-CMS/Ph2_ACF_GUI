from PyQt5 import QtCore
from PyQt5.QtCore import Qt, QThread, pyqtSignal, QTimer, QSize
from PyQt5.QtGui import QIcon, QPixmap
from PyQt5.QtWidgets import (
    QApplication,  # ← Add this import
    QCheckBox,
    QComboBox,
    QGridLayout,
    QGroupBox,
    QLabel,
    QLineEdit,
    QMessageBox,
    QPushButton,
    QWidget,
    QVBoxLayout,
    QScrollArea,
)

import subprocess
import os
from PyQt5.QtCore import QUrl
from PyQt5.QtGui import QDesktopServices
#from Gui.python.thermal_utils import generate_thermal_file_paths

import sys
import os
import requests

from Gui.python.logging_config import get_logger
logger = get_logger(__name__)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
icon_path = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "icons",
    "send_modules.jpeg"
)

class ThermalModulesWindow(QWidget):
    def __init__(self, master):
        super(ThermalModulesWindow, self).__init__()
        self.master = master
        self.setWindowTitle("Thermal Test Modules")
        self.createMain()

    def createMain(self):   
        main_layout = QVBoxLayout(self)
        
        self.scrollArea = QScrollArea()
        self.scrollArea.setWidgetResizable(True)

        self.scrollWidget = QWidget()
        self.scrollLayout = QGridLayout(self.scrollWidget)

        self.scrollArea.setWidget(self.scrollWidget)

        # Limit height to ~5 rows visible
        row_height = QLineEdit().sizeHint().height()
        visible_rows = 5
        self.scrollArea.setFixedHeight(row_height * visible_rows + 20) # +20 for padding

        main_layout.addWidget(self.scrollArea)

        bottom_layout = QGridLayout()

        self.modules_list = [] #list of module identification strings
        self.rows = [] # each row = (label, lineEdit, button)

        #initialize buttons
        self.addModule()

        self.closeButton = QPushButton("Exit")
        self.subdetectorCombo = QComboBox()
        self.sendModulesButton = QPushButton("Send Modules")
        self.subdetectorLabel = QLabel("Subdetector: ")

        self.closeButton.setDisabled(False)
        self.sendModulesButton.setIcon(QIcon(icon_path))
        self.sendModulesButton.setIconSize(QSize(24, 24))

        #button clicks and other connections
        #self.addModuleButton.clicked.connect(self.addModule)
        self.closeButton.clicked.connect(self.closeWindow)
        self.sendModulesButton.clicked.connect(self.sendModules)
        
        #self.moduleEntry.editingFinished.connect(self.addModule) #user clicks away (tab, enter, or other)

        # Add subdetector combo box
        self.subdetectorCombo.addItems(["", "TFPX", "TEPX", "TBPX"])
        
        #bottom_layout.addWidget(self.addModuleButton, 0, 0)
        bottom_layout.addWidget(self.subdetectorLabel, 0, 0)
        bottom_layout.addWidget(self.subdetectorCombo, 0, 1)
        bottom_layout.addWidget(self.sendModulesButton, 0, 2)
        bottom_layout.addWidget(self.closeButton, 0, 3)
        
        # Set the layout on the widget
        main_layout.addLayout(bottom_layout)

        
        self.show()

    def addModule(self):
        row_index = len(self.rows)
        
        # Add new module row
        label = QLabel(f"Module {row_index + 1}: ")
        moduleEntry = QLineEdit()
        removeModuleButton = QPushButton("❌")

        moduleEntry.returnPressed.connect(self.addModule)
        removeModuleButton.clicked.connect(lambda _, r=row_index: self.removeModule(r))

        self.rows.append((label, moduleEntry, removeModuleButton))

        self.rebuildLayout()
        moduleEntry.setFocus()

    def removeModule(self, row):
        if 0 <= row < len(self.rows):
            # delete widgets
            for widget in self.rows[row]:
                widget.deleteLater()

            #remove row from list
            self.rows.pop(row)

            self.rebuildLayout()

    def rebuildLayout(self):
        # Clear existing layout
        while self.scrollLayout.count():
            item = self.scrollLayout.takeAt(0)
            widget = item.widget()
            if widget:
                widget.setParent(None)

        # Add rows back
        for i, (label, entry, button) in enumerate(self.rows):
            label.setText(f"Module {i + 1}:")

            try:
                button.clicked.disconnect()
            except Exception as e:
                logger.debug("Error disconnecting button: %s", e)
            button.clicked.connect(lambda _, r=i: self.removeModule(r))

            if len(self.rows) < 2:
                button.setVisible(False)
            else:
                button.setVisible(True)
                
            self.scrollLayout.addWidget(label, i, 0)
            self.scrollLayout.addWidget(entry, i, 1)
            self.scrollLayout.addWidget(button, i, 2)

    def sendModules(self):
        #subdetector = "TFPX"
        username = self.master.username
        logger.debug("Username: %s", username)

        userpass = self.master.password
        logger.debug("Password: %s", userpass)

        subdetector = self.subdetectorCombo.currentText().strip()
        logger.debug("Subdetector: %s", subdetector)

        url = "https://panthera.fit.edu/request_handlers/thermal_cycle_handler.php"
        #url = "fake url for debugging"
        logger.debug("URL: %s", url)

        filled_modules = [
            entry.text().strip()
            for (_, entry, _) in self.rows
            if entry.text().strip()
        ]

        # block if subdetector not selected
        if not subdetector:
            logger.warning("No subdetector selected")
            QMessageBox.warning(self, "Warning", "Please select a subdetector")
            return

        # block if no modules entered
        if not filled_modules:
            logger.warning("No modules entered")
            QMessageBox.warning(self, "Warning", "Please enter at least one module")
            return

        Data_sent_counter = 0 

        for name_module in filled_modules:
            logger.debug("Module: %s", name_module)

            if name_module not in self.modules_list:
                self.modules_list.append(name_module)

            try:
                response = requests.post(
                    url,
                    data={
                        "name_module": name_module,
                        "subdetector": subdetector,
                        "username": username,
                        "userpass": userpass,
                    }
                )

                if response.status_code == 200:
                    Data_sent_counter += 1
                else:
                    logger.error("Failed for %s: %s", name_module, response.text)
            except Exception as e:
                logger.error("Exception for %s: %s", name_module, e)

        logger.debug("Modules list: %s", filled_modules)

        if Data_sent_counter == len(filled_modules):
            logger.info("Data sent successfully for all %d modules", Data_sent_counter)
            message_box = QMessageBox()
            message_box.setText("Data sent successfully for all %d modules" % Data_sent_counter)
            message_box.setIcon(QMessageBox.Information)
            message_box.exec()
            self.closeWindow()
        elif Data_sent_counter == 0:
            logger.warning("No data sent")
            message_box = QMessageBox()
            message_box.setText("No data sent")
            message_box.setIcon(QMessageBox.Warning)
            message_box.exec()
        else:
            logger.warning("Data not sent for %d modules", len(filled_modules) - Data_sent_counter)
            message_box = QMessageBox()
            message_box.setText("Data not sent for %d modules" % (len(filled_modules) - Data_sent_counter))
            message_box.setIcon(QMessageBox.Warning)
            message_box.exec()

    def closeWindow(self):
        self.close()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = ThermalModulesWindow(None)
    sys.exit(app.exec())
