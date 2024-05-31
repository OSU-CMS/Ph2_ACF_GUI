from PyQt5.QtCore import *
from PyQt5.QtGui import QImage, QFont, QPixmap
from PyQt5.QtWidgets import (
    QAbstractItemView,
    QApplication,
    QCheckBox,
    QComboBox,
    QDateTimeEdit,
    QDial,
    QDialog,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QProgressBar,
    QPushButton,
    QRadioButton,
    QScrollBar,
    QSizePolicy,
    QSlider,
    QSpinBox,
    QStyleFactory,
    QTableWidget,
    QTabWidget,
    QTextEdit,
    QHBoxLayout,
    QVBoxLayout,
    QWidget,
    QMainWindow,
    QMessageBox,
)

import sys
import os
import numpy

from Gui.GUIutils.DBConnection import (
    mysql,
    retrieveWithConstraint,
    )
#from Gui.GUIutils.guiUtils import *
from Gui.QtGUIutils.QtDBTableWidget import QtDBTableWidget
#from Gui.QtGUIutils.QtImageViewer import *
from Gui.python.logging_config import logger


class QtImageViewerTab(QWidget):
    def __init__(self, master):
        super(QtImageViewerTab, self).__init__()
        self.master = master
        self.connection = self.master.connection

        self.mainlayout = QGridLayout()
        self.searchBox()
        self.viewBox()
        self.setLayout(self.mainlayout)

    def searchBox(self):
        self.SearchBox = QGroupBox()
        self.SearchLayout = QGridLayout()

        self.ImageViewer = QLabel('<font size="12"> Image Viewer </font>')

        self.ModuleIDLabel = QLabel("Module ID:")
        self.ModuleIDEdit = QLineEdit()
        self.ModuleIDEdit.setEchoMode(QLineEdit.Normal)

        self.SearchButton = QPushButton("Search")
        self.SearchButton.clicked.connect(self.showSearchResult)

        self.FeedBackLabel = QLabel("")

        self.SearchLayout.addWidget(self.ImageViewer, 0, 0, 1, 4, Qt.AlignTop)

        self.SearchLayout.addWidget(self.ModuleIDLabel, 1, 0, 1, 1)
        self.SearchLayout.addWidget(self.ModuleIDEdit, 1, 1, 1, 3)

        self.SearchLayout.addWidget(self.FeedBackLabel, 2, 0, 1, 6)
        self.SearchLayout.addWidget(self.SearchButton, 2, 6, 1, 1)

        self.SearchBox.setLayout(self.SearchLayout)
        self.mainlayout.addWidget(self.SearchBox, 0, 0, 1, 8)

    def viewBox(self, dataList=[]):
        self.ViewBox = QGroupBox()
        self.ViewLayout = QGridLayout()

        try:
            self.proxy = QtDBTableWidget(dataList, 0, True)

            lineEdit = QLineEdit()
            lineEdit.textChanged.connect(self.proxy.on_lineEdit_textChanged)
            self.view = QTableView()
            self.view.setSortingEnabled(True)
            comboBox = QComboBox()
            if len(dataList) > 0:
                comboBox.addItems(["{0}".format(x) for x in dataList[0]])
            comboBox.currentIndexChanged.connect(
                self.proxy.on_comboBox_currentIndexChanged
            )
            label = QLabel()
            label.setText("Regex Filter")

            self.view.setModel(self.proxy)
            self.view.setSelectionBehavior(QAbstractItemView.SelectRows)
            self.view.setSelectionMode(QAbstractItemView.MultiSelection)
            self.view.setEditTriggers(QAbstractItemView.NoEditTriggers)

            for row in range(len(self.proxy.dataBody)):
                DetailButton = QPushButton("&Show...")
                DetailButton.clicked.connect(
                    lambda state, x="{0}".format(
                        self.proxy.dataBody[row][0]
                    ): self.openImage(x)
                )
                self.view.setIndexWidget(self.proxy.index(row, 0), DetailButton)

            self.ViewLayout.addWidget(lineEdit, 0, 1, 1, 1)
            self.ViewLayout.addWidget(self.view, 1, 0, 1, 3)
            self.ViewLayout.addWidget(comboBox, 0, 2, 1, 1)
            self.ViewLayout.addWidget(label, 0, 0, 1, 1)

        except Exception as error:
            print(error)
            print("Error: failed to create viewBox")

        self.ViewBox.setLayout(self.ViewLayout)
        self.mainlayout.addWidget(self.ViewBox, 1, 0, 3, 5)

        self.PlotBox = QGroupBox("Image Display")
        self.PlotLayout = QGridLayout()

        self.PlotLabel = QLabel()

        self.PlotLayout.addWidget(self.PlotLabel)

        self.PlotBox.setLayout(self.PlotLayout)
        self.mainlayout.addWidget(self.PlotBox, 1, 5, 3, 3)

    def showSearchResult(self):
        try:
            columns = ["id", "part_id", "date", "caption", "username"]
            dataList = [columns] + retrieveWithConstraint(
                self.connection,
                "images",
                part_id=self.ModuleIDEdit.text(),
                columns=columns,
            )
        except mysql.connector.Error as error:
            self.FeedBackLabel.setText(
                "Failed retrieving MySQL table: {}".format(error)
            )
            return

        if len(dataList) <= 1:
            self.FeedBackLabel.setStyleSheet("color:red")
            self.FeedBackLabel.setText("No Record Found")
        else:
            self.FeedBackLabel.setText("")
        self.ViewBox.deleteLater()
        self.mainlayout.removeWidget(self.ViewBox)
        self.viewBox(dataList)

    def openImage(self, id):
        try:
            columns = ["part_id", "caption", "image"]
            data = retrieveWithConstraint(
                self.connection, "images", id=int(id), columns=columns
            )
        except:
            QMessageBox().information(
                None, "Warning", "Database connection broken", QMessageBox.Ok
            )

        try:
            # viewer = QtImageViewer(self,data[0])
            self.image = QImage()
            self.image.loadFromData(data[0][2])
            self.PlotMap = QPixmap.fromImage(self.image).scaled(
                QSize(350, 450), Qt.KeepAspectRatio, Qt.SmoothTransformation
            )
            self.PlotLabel.setPixmap(self.PlotMap)
        except Exception as error:
            print(error)
