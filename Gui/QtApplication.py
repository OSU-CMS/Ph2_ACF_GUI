"""
Use to launch in to Ph2_ACF GUI.

Will launch QtLoginDialog which will then connect to the simplified
GUI or the expert GUI. The rest of the execution of the code will
continue from these widgets.
"""
from PyQt5.QtWidgets import QApplication, QWidget, QMessageBox
import sys
import os
from Gui.QtGUIutils.QtLoginDialog import QtLoginDialog
from Gui.GUIutils.DBConnection import connect_to_database
from Gui.QtGUIutils.QtExpertWindow import QtExpertWindow
import Gui.Config.siteConfig as site_config

# from Gui.QtGUIutils.QtSimplifiedWindow import QtSimplifiedWindow


def login_to_gui(connection_parameters: dict) -> QWidget:
    """
    Control login to GUI.

    Will attempt to connect to database if connection succeeds.
    Will launch either the simplified GUI or the expert GUI
    depending on the value of connection_parameters["expert"]
    """
    print("in login_to_gui")
    connection = connect_to_database(
        connection_parameters["username"],
        connection_parameters["password"],
        connection_parameters["address"],
        connection_parameters["database"],
    )
    print(connection_parameters)
    if connection_parameters["expert"]:
        main_widget = QtExpertWindow(connection)
    # elif not connection_parameters["expert"]:
    #     main_widget = QtSimplifiedWindow(connection)
    else:
        print("Unexpected value set given to connection parameters")
        raise Exception("Unexpected user state encountered")
    return main_widget


def initLog():
    """Use to create log files for FC7s. Logs will be stored in $GUI_dir fc7_name."""

    for index, fc7_name in enumerate(site_config.FC7List.keys()):
        LogFileName = "{0}/Gui/.{1}.log".format(os.environ.get("GUI_dir"), fc7_name)
        try:
            logFile = open(LogFileName, "w")
            logFile.close()
        except:
            QMessageBox(
                None, "Error", "Can not create log files: {}".format(LogFileName)
            )


if __name__ == "__main__":
    app = QApplication([])
    initLog()
    login_window = QtLoginDialog()

    login_window.login_signal.connect(lambda connection: login_to_gui(connection))

    sys.exit(app.exec_())
