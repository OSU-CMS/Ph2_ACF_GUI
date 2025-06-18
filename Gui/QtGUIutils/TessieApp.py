from PyQt5 import QtCore, QtGui, QtWidgets

NUMBER_OF_TECS: int = 8
NUMBER_OF_TECS_PER_COLUMN: int = 2


class MonitoringWidget(QtWidgets.QWidget):
    def __init__(self, label: str, parent=None):
        super().__init__(parent)
        layout = QtWidgets.QHBoxLayout()
        layout.setSpacing(1)
        layout.setContentsMargins(0, 0, 0, 0)
        self.monitor_value = QtWidgets.QLabel("N/a")
        self.monitor_value.setStyleSheet("""
                    QLabel {
                        background-color: #f0f0f0;
                        border: 1px solid #ccc;
                        padding: 2px 4px;
                        border-radius: 3px;
                    }
                """)
        monitor_label = QtWidgets.QLabel(label)

        layout.addWidget(self.monitor_value)
        layout.addWidget(monitor_label)
        self.setLayout(layout)

    def set_value(self, value: str):
        self.monitor_value.setText(value)

    def get_value(self) -> str:
        return self.monitor_value.text()


class TessieWidget(QtWidgets.QWidget):

    def __init__(self):
        super().__init__()


        self.redledpixmap = QtGui.QPixmap.fromImage(
            QtGui.QImage("icons/led-red-on.png").scaled(
                QtCore.QSize(60, 10),
                QtCore.Qt.KeepAspectRatio,
                QtCore.Qt.SmoothTransformation,
            )
        )
        self.greenledpixmap = QtGui.QPixmap.fromImage(
            QtGui.QImage("icons/led-green-on.png").scaled(
                QtCore.QSize(60, 10),
                QtCore.Qt.KeepAspectRatio,
                QtCore.Qt.SmoothTransformation,
            )
        )


        self.setupUi()
        self.setupUx()

    def setupUi(self):
        layout = QtWidgets.QGridLayout(self)

        self.startButton = QtWidgets.QPushButton("Start Cooling", self)

        self.currentSetTemp = MonitoringWidget("Current Set Temperature", self)
        self.currentHumidity = MonitoringWidget("Current Humidity", self)
        self.statusLight = MonitoringWidget("Status", self)

        # Setup TEC Temperature Container
        tec_container = QtWidgets.QGroupBox()
        tec_container_layout = QtWidgets.QGridLayout()

        self.temp_monitoring = (
            MonitoringWidget(f"TEC {i}", tec_container) for i in range(NUMBER_OF_TECS)
        )

        for i, temp_monitor in enumerate(self.temp_monitoring): 
            tec_container_layout.addWidget(temp_monitor,i%NUMBER_OF_TECS_PER_COLUMN, i//NUMBER_OF_TECS_PER_COLUMN)

        tec_container.setLayout(tec_container_layout)

        layout.addWidget(self.currentSetTemp, 0, 0, 1, 2)
        layout.addWidget(self.currentHumidity, 1, 0,1, 2)
        layout.addWidget(self.statusLight, 0, 2, 1, 2)
        layout.addWidget(self.startButton, 1, 2, 1, 2)
        layout.addWidget(tec_container, 2, 0, 2, 4)

    def setupUx(self): ...

if __name__ == "__main__":
    import sys

    app = QtWidgets.QApplication(sys.argv)
    window = TessieWidget()
    window.setWindowTitle("Tessie Monitoring Panel")
    window.resize(600, 400)
    window.show()
    sys.exit(app.exec_())
