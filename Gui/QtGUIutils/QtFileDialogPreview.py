from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPixmap
from PyQt5.QtWidgets import (
    QFileDialog,
    QLabel,
    QVBoxLayout,
)


# from Gui.GUIutils.DBConnection import *
# from Gui.GUIutils.guiUtils import *


class QtFileDialogPreview(QFileDialog):
    def __init__(self, *args, **kwargs):
        QFileDialog.__init__(self, *args, **kwargs)
        self.setOption(QFileDialog.DontUseNativeDialog, True)

        box = QVBoxLayout()

        self.setFixedSize(self.width() + 250, self.height())

        self.mpPreview = QLabel("Preview", self)
        self.mpPreview.setFixedSize(250, 250)
        self.mpPreview.setAlignment(Qt.AlignCenter)
        self.mpPreview.setObjectName("labelPreview")
        box.addWidget(self.mpPreview)

        box.addStretch()

        self.layout().addLayout(box, 1, 3, 1, 1)

        self.currentChanged.connect(self.onChange)
        self.fileSelected.connect(self.onFileSelected)
        self.filesSelected.connect(self.onFilesSelected)

        self._fileSelected = None
        self._filesSelected = None

    def onChange(self, path):
        pixmap = QPixmap(path)

        if pixmap.isNull():
            self.mpPreview.setText("Preview")
        else:
            self.mpPreview.setPixmap(
                pixmap.scaled(
                    self.mpPreview.width(),
                    self.mpPreview.height(),
                    Qt.KeepAspectRatio,
                    Qt.SmoothTransformation,
                )
            )

    def onFileSelected(self, file):
        self._fileSelected = file

    def onFilesSelected(self, files):
        self._filesSelected = files

    def getFileSelected(self):
        return self._fileSelected

    def getFilesSelected(self):
        return self._filesSelected
