from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QWidget, QPushButton, QApplication, QListWidget, QGridLayout, QLabel
from PyQt5.QtCore import QTimer, QDateTime 
import sys
class Peltier(QWidget):
    def __init__(self, parent=None):
        super(Peltier, self).__init__(parent)
        self.setWindowTitle('QTimer example')

        self.listFile=QListWidget()
        self.label = QLabel('Label')
        self.startBtn = QPushButton('Start')
        self.endBtn = QPushButton('Stop')

        layout = QGridLayout()

        self.timer = QTimer()
        self.timer.timeout.connect(self.showTime)

        layout.addWidget(self.label, 0,0,1,2)
        layout.addWidget(self.startBtn, 1,0)
        layout.addWidget(self.endBtn, 1, 1)

        self.startBtn.clicked.connect(self.startTimer)
        self.endBtn.clicked.connect(self.endTimer)

        self.setLayout(layout)

    def showTime(self):
            time = QDateTime.currentDateTime()
            timeDisplay = time.toString('yyyy-MM-dd hh:mm:ss dddd')
            self.label.setText(timeDisplay)

    def startTimer(self):
            self.timer.start(1000)
            self.startBtn.setEnabled(False)
            self.endBtn.setEnabled(True)

    def endTimer(self):
            self.timer.stop()
            self.startBtn.setEnabled(True)
            self.endBtn.setEnabled(False)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    form = Peltier()
    form.show()
    sys.exit(app.exec())

