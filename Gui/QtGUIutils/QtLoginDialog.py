"""
Use to open up Qt window to connect to GUI and database.

createLogin()
checkLogin()
switchMode()
"""
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QDialog,
    QGridLayout,
    QGroupBox,
    QLabel,
    QLineEdit,
    QPushButton,
)
import sys
import Gui.Config.staticSettings as settings


class QtLoginDialog(QDialog):
    """Use to create login screen to open GUI and connect to database."""

    login_signal = pyqtSignal(dict)

    def __init__(self):
        """Initialize class."""
        super().__init__()
        self.expertMode = False

        self.mainLayout = QGridLayout()
        print("inside QtLoginDialog")
        self.createLogin()
        self.setLayout(self.mainLayout)

    def createLogin(self):
        """Use to create window for login displayed when starting the GUI."""
        login_groupbox = QGroupBox("")
        login_groupbox.setCheckable(False)

        title_label = QLabel(
            '<font size="12">Phase2 Pixel Module\
                                Test </font>'
        )
        title_label.setFont(QFont("Courier"))
        title_label.setMaximumHeight(30)

        username_label = QLabel("Username:")
        self.username_edit = QLineEdit("")
        self.username_edit.setEchoMode(QLineEdit.Normal)
        self.username_edit.setPlaceholderText("Your username")
        self.username_edit.setMinimumWidth(220)
        self.username_edit.setMaximumWidth(260)
        self.username_edit.setMaximumHeight(30)

        password_label = QLabel("Password:")
        self.password_edit = QLineEdit("")
        self.password_edit.setEchoMode(QLineEdit.Password)
        self.password_edit.setPlaceholderText("Your password")
        self.password_edit.setMinimumWidth(220)
        self.password_edit.setMaximumWidth(260)
        self.password_edit.setMaximumHeight(30)

        host_label = QLabel("HostName:")

        host_edit = QLineEdit("128.146.38.1")
        host_edit.setEchoMode(QLineEdit.Normal)
        host_edit.setMinimumWidth(220)
        host_edit.setMaximumWidth(260)
        host_edit.setMaximumHeight(30)

        database_edit = QLineEdit("SampleDB")
        database_edit.setEchoMode(QLineEdit.Normal)
        database_edit.setMinimumWidth(220)
        database_edit.setMaximumWidth(260)
        database_edit.setMaximumHeight(30)

        self.disable_checkbox = QCheckBox("&Do not connect to DB")
        self.disable_checkbox.setMaximumHeight(30)
        if not self.expertMode:
            self.host_name = QComboBox()
            self.host_name.addItems(settings.dblist)
            host_label.setBuddy(self.host_name)
            self.database_combo = QComboBox()
            self.DBNames = self.host_name.currentText() + ".All_list"
            self.database_combo.addItems([self.DBNames])
            self.database_combo.setCurrentIndex(0)
            self.disable_checkbox.toggled.connect(host_edit.setDisabled)
            self.disable_checkbox.toggled.connect(database_edit.setDisabled)
        else:
            host_label.setText("HostIPAddr")
            self.disableCheckBox.toggled.connect(self.host_name.setDisabled)
            self.disableCheckBox.toggled.connect(self.database_combo.setDisabled)

        button_login = QPushButton("&Login")
        button_login.setDefault(True)
        button_login.clicked.connect(self.checkLogin)

        layout = QGridLayout()
        layout.setSpacing(20)
        layout.addWidget(title_label, 0, 1, 1, 3, Qt.AlignCenter)
        layout.addWidget(username_label, 1, 1, 1, 1, Qt.AlignRight)
        layout.addWidget(self.username_edit, 1, 2, 1, 2)
        layout.addWidget(password_label, 2, 1, 1, 1, Qt.AlignRight)
        layout.addWidget(self.password_edit, 2, 2, 1, 2)
        layout.addWidget(button_login, 5, 1, 1, 3)

        layout.setRowMinimumHeight(6, 50)

        layout.setColumnStretch(0, 1)
        layout.setColumnStretch(1, 1)
        layout.setColumnStretch(2, 2)
        layout.setColumnStretch(3, 1)

        login_groupbox.setLayout(layout)
        self.logo_groupbox = QGroupBox("")

        self.mainLayout.addWidget(login_groupbox, 0, 0)
        print("Finshed Create QtLogin")

    def checkLogin(self):
        """
        Use to check credentials used in login dialog.

        will emit signal that needs to be connected to determine
        whether to launch simplied or main GUI
        """
        if self.username_edit.text() in settings.ExpertUserList:
            self.expertMode = True

        connection_parameters = {
            "username": self.username_edit.text(),
            "password": self.password_edit.text(),
            "address": self.host_name.currentText() + ".DBIP",
            "database": self.database_combo.currentText(),
            "expert": self.expertMode,
        }

        self.login_signal.emit(connection_parameters)

    # Currently doesn't work
    def switchMode(self):
        """Use to switch between expert and simplified mode."""
        if self.expertMode:
            self.expertMode = False
        else:
            self.expertMode = True
        self.destroyLogin()
        self.createLogin()


if __name__ == "__main__":
    app = QApplication([])
    login_window = QtLoginDialog()
    login_window.login_signal.connect(lambda x: print(x.items()))
    sys.exit(app.exec_())
