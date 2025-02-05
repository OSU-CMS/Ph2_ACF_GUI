from PyQt5.QtWidgets import QWidget
from PyQt5.QtCore import Qt, QTimer, QThread, pyqtSignal
from PyQt5.QtGui import QPainter, QPen

class LoadingThread(QThread):
    finished_signal = pyqtSignal()
    def __init__(self,function,interval,*args):
        super(LoadingThread, self).__init__()
        self.function = function
        self.args = args
        self.timer = QTimer()
        self.timer.setInterval(interval)

    def run(self):
        self.function(*self.args)
        self.finished_signal.emit()

class LoadingWheel(QWidget):
    def __init__(self):  # Accepts parent widget
        super().__init__()  # Pass parent to QWidget
        self.setFixedSize(30, 30)  # Set a fixed size for embedding
        self.angle = 0

    def update_spinner(self):
        self.angle = (self.angle + 10) % 360  # Rotate by 10 degrees
        self.update()  # Trigger repaint

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        painter.translate(self.width() / 2, self.height() / 2)  # Center rotation
        painter.rotate(self.angle)

        pen = QPen(Qt.white, 2, Qt.SolidLine, Qt.RoundCap)
        painter.setPen(pen)

        # Draw arc as spinner
        painter.drawArc(-10, -10, 20, 20, 0, 270 * 16)  # 270 degrees (3/4 circle)

