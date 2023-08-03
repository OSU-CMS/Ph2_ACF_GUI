"""
Use to launch in to Ph2_ACF GUI.

Will launch QtLoginDialog which will then connect to the simplified
GUI or the expert GUI. The rest of the execution of the code will
continue from these widgets.
"""
from PyQt5.QtWidgets import QApplication, QWidget
import sys
from Gui.QtGUIutils.QtLoginDialog import QtLoginDialog
from Gui.GUIutils.DBConnection import connect_to_database
from Gui.QtGUIutils.QtExpertWindow import QtExpertWindow
from Gui.QtGUIutils.QtSimplifiedWindow import QtSimplifiedWindow


def login_to_gui(connection_parameters: dict) -> QWidget:
    """
    Control login to GUI.

    Will attempt to connect to database if connection succeeds.
    Will launch either the simplified GUI or the expert GUI
    depending on the value of connection_parameters["expert"]
    """

    connect_to_database(
        connection_parameters["username"],
        connection_parameters["password"],
        connection_parameters["address"],
        connection_parameters["database"],
    )

    if connection_parameters["expert"]:
        main_widget = QtExpertWindow
    elif not connection_parameters["expert"]:
        main_widget = QtSimplifiedWindow
    else:
        print("Unexpected value set given to connection parameters")
    return main_widget


if __name__ == "__main__":
    app = QApplication([])
    dimension = app.screens()[0].size()
    login_window = QtLoginDialog()

    login_window.login_signal.conect(lambda x: login_to_gui(x))

    sys.exit(app.exec_())
