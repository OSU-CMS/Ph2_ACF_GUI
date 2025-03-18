from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QWidget
from PyQt5.QtCore import Qt

class Tessie(QWidget):
    def __init__(self, dimension):
        super(Tessie, self).__init__()
        self.setupUi()
        self.show()

    def setupUi(self):

        self.gridLayout = QtWidgets.QGridLayout(self)

        self.upperWidget = QWidget()
        self.upperGridLayout = QtWidgets.QGridLayout(self.upperWidget)  # Set layout here
        self.upperWidget.setLayout(self.upperGridLayout)

        self.lowerWidget = QWidget()
        self.lowerGridLayout = QtWidgets.QGridLayout(self.lowerWidget)  # Set layout here
        self.lowerWidget.setLayout(self.lowerGridLayout)

        self.gridLayout.addWidget(self.upperWidget, 0, 0)
        self.upperGridLayout.addItem(QtWidgets.QSpacerItem(0, 10))
        self.gridLayout.addWidget(self.lowerWidget, 1, 0)

        self.setLayout(self.gridLayout)  # Set layout for the main window

        self.TessieLabel = QtWidgets.QLabel("TESSIE (2024/06/19-06)")
        self.CANbus_errors_label = QtWidgets.QLabel("CANbus errors")
        self.I2C_errors_label = QtWidgets.QLabel("I2C errors")
        self.runtimeLabel = QtWidgets.QLabel("Runtime")
        self.statusLabel = QtWidgets.QLabel("Status")

        self.upperGridLayout.addWidget(self.TessieLabel, 0,0,1,2)
        self.upperGridLayout.addWidget(self.CANbus_errors_label, 1,0,1,1)
        self.upperGridLayout.addWidget(self.I2C_errors_label, 2,0,1,1)
        self.upperGridLayout.addWidget(self.runtimeLabel, 3,0,1,1)
        self.upperGridLayout.addWidget(self.statusLabel, 4,0,1,1)

        self.CANbus_errors_edit = QtWidgets.QLabel("0")
        self.I2C_errors_edit = QtWidgets.QLabel("0")
        self.runtimeEdit = QtWidgets.QLabel("10346")
        self.statusEdit = QtWidgets.QLabel("no problem")

        self.upperGridLayout.addWidget(self.CANbus_errors_edit, 1,1,1,1)
        self.upperGridLayout.addWidget(self.I2C_errors_edit, 2,1,1,1)
        self.upperGridLayout.addWidget(self.runtimeEdit, 3,1,1,1)
        self.upperGridLayout.addWidget(self.statusEdit, 4,1,1,1)
        self.upperGridLayout.setColumnMinimumWidth(1, 75)


        self.airLabel = QtWidgets.QLabel("Air [° C]")
        self.waterLabel = QtWidgets.QLabel("Water [° C]")
        self.lid_status_label = QtWidgets.QLabel("Lid Status")

        self.upperGridLayout.addWidget(self.airLabel, 1,2,1,1)
        self.upperGridLayout.addWidget(self.waterLabel, 2,2,1,1)
        self.upperGridLayout.addWidget(self.lid_status_label, 3,2,1,1)

        self.airEdit = QtWidgets.QLabel("23.25")
        self.waterEdit = QtWidgets.QLabel("21.34")
        self.lid_status_edit = QtWidgets.QLabel("locked")

        self.airEdit.setStyleSheet("QLabel { background-color: green; color: black; }")
        self.waterEdit.setStyleSheet("QLabel { background-color: green; color: black; }")
        self.lid_status_edit.setStyleSheet("QLabel { background-color: green; color: black; }")

        self.upperGridLayout.addWidget(self.airEdit, 1,3,1,1)
        self.upperGridLayout.addWidget(self.waterEdit, 2,3,1,1)
        self.upperGridLayout.addWidget(self.lid_status_edit, 3,3,1,1)
        self.upperGridLayout.setColumnMinimumWidth(3, 50)

        self.ref_hum_label = QtWidgets.QLabel("Ref. Hum.")
        self.dew_point_label = QtWidgets.QLabel("Dew Point")

        self.upperGridLayout.addWidget(self.ref_hum_label, 1,4,1,1)
        self.upperGridLayout.addWidget(self.dew_point_label, 2,4,1,1)

        self.ref_hum_edit = QtWidgets.QLabel("51.92")
        self.dew_point_edit = QtWidgets.QLabel("12.83")

        self.ref_hum_edit.setStyleSheet("QLabel { background-color: yellow; color: black; }")
        self.dew_point_edit.setStyleSheet("QLabel { background-color: green; color: black; }")

        self.upperGridLayout.addWidget(self.ref_hum_edit, 1,5,1,1)
        self.upperGridLayout.addWidget(self.dew_point_edit, 2,5,1,1)
        self.upperGridLayout.setColumnMinimumWidth(5, 50)

        self.stop_all_button = QtWidgets.QPushButton("STOP ALL")
        self.stop_all_button.setStyleSheet("QPushButton {background-color: red; color: black}")
        self.upperGridLayout.addWidget(self.stop_all_button, 4, 2, 1, 4)

        self.flushButton = QtWidgets.QPushButton("Flush")
        self.rinseButton = QtWidgets.QPushButton("Rinse")

        self.lowerGridLayout.addWidget(self.flushButton, 0,0)
        self.lowerGridLayout.addWidget(self.rinseButton, 1,0)

        self.setTemperatures = [1]*8
        self.TEC_labels = [QtWidgets.QLabel(f'TEC {i}:') for i in range(1,9)]
        self.TEC_edits = [QtWidgets.QLabel(f'{self.setTemperatures[i]}') for i in range(8)]

        for i in range(4):
            self.lowerGridLayout.addWidget(self.TEC_labels[8-i-1],0,2*i+1,1,1,Qt.AlignRight)
            self.lowerGridLayout.addWidget(self.TEC_edits[8-i-1],0,2*i+2)
            self.lowerGridLayout.addWidget(self.TEC_labels[i], 1, 2*i+1,1,1,Qt.AlignRight)
            self.lowerGridLayout.addWidget(self.TEC_edits[i], 1, 2*i+2)

        self.setLayout(self.gridLayout)


if __name__ == "__main__":
    import sys

    app = QtWidgets.QApplication(sys.argv)
    ui = Tessie(500)
    sys.exit(app.exec_())
