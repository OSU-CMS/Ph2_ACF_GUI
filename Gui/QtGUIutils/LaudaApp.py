from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QWidget
import Gui.siteSettings as site_settings
from icicle.icicle.lauda import Lauda
import os


class LaudaWidget(QWidget):

    def __init__(self, dimension):
        
        self.myLauda = Lauda(resource=site_settings.lauda_resource)

        super(LaudaWidget, self).__init__()
        self.Ph2ACFDirectory = os.getenv("GUI_dir")
        self.setupUi()
        self.show()

    def setupUi(self):
        kMinimumWidth = 150
        kMaximumWidth = 150
        kMinimumHeight = 30
        kMaximumHeight = 100

        self.StartChillerButton = QtWidgets.QPushButton("Start")
        self.StartChillerButton.setMinimumWidth(kMinimumWidth)
        self.StartChillerButton.setMaximumWidth(kMaximumWidth)
        self.StartChillerButton.setMinimumHeight(kMinimumHeight)
        self.StartChillerButton.setMaximumHeight(kMaximumHeight)
        self.StartChillerButton.clicked.connect(self.startChiller)
        self.StartChillerButton.setCheckable(True)

        self.StopChillerButton = QtWidgets.QPushButton("Stop")
        self.StopChillerButton.setMinimumWidth(kMinimumWidth)
        self.StopChillerButton.setMaximumWidth(kMaximumWidth)
        self.StopChillerButton.setMinimumHeight(kMinimumHeight)
        self.StopChillerButton.setMaximumHeight(kMaximumHeight)
        self.StopChillerButton.clicked.connect(self.stopChiller)
        self.StopChillerButton.setCheckable(True)

        self.SetTempButton = QtWidgets.QPushButton("Set Temperature")
        self.SetTempButton.setMinimumWidth(kMinimumWidth)
        self.SetTempButton.setMaximumWidth(kMaximumWidth)
        self.SetTempButton.setMinimumHeight(kMinimumHeight)
        self.SetTempButton.setMaximumHeight(kMaximumHeight)
        self.SetTempButton.clicked.connect(self.setTemperature)
        self.SetTempButton.setCheckable(True)

        self.SetTempEdit = QtWidgets.QLineEdit("")
        self.SetTempEdit.setMinimumWidth(140)
        self.SetTempEdit.setEchoMode(QtWidgets.QLineEdit.Normal)
        self.SetTempEdit.setPlaceholderText("Set Temperature")
        self.SetTempEdit.textChanged.connect(lambda : self.SetTempButton.setChecked(False))

        self.ChillerLayout = QtWidgets.QGridLayout(self)
        self.ChillerLayout.addWidget(self.StartChillerButton, 0, 0, 1, 1)
        self.ChillerLayout.addWidget(self.StopChillerButton,1,0,1,1)
        self.ChillerLayout.addWidget(self.SetTempButton,2,0,1,1)
        self.ChillerLayout.addWidget(self.SetTempEdit,2,1,1,2)

        self.setLayout(self.ChillerLayout)

    def resourceExists(self):
        return False if self.lauda_resource == None else True

    def startChiller(self):
        self.StartChillerButton.setChecked(True)
        self.StopChillerButton.setChecked(False)

        self.myLauda.set("START","START")

    def stopChiller(self):
        self.StartChillerButton.setChecked(False)
        self.StopChillerButton.setChecked(True)

        self.myLauda.set("STOP", "STOP")

    def setTemperature(self):
        self.SetTempButton.setChecked(True)
        
        try:
            self.myLauda.set("TEMPERATURE_TARGET",float(self.setTempEdit.text()))
        except ValueError:
            print("Temperature target must be a float.")

if __name__ == "__main__":
    import sys

    app = QtWidgets.QApplication(sys.argv)
    ui = LaudaWidget(500)
    sys.exit(app.exec_())
