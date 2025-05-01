import webbrowser
from PyQt5 import QtWebEngine
QtWebEngine.QtWebEngine.initialize()
from PyQt5.QtWidgets import QWidget, QPushButton, QVBoxLayout
from PyQt5.QtWebEngineWidgets import QWebEngineView
from PyQt5.QtCore import QUrl

from Gui.siteSettings import tessie_url, alternative_tessie_url

class Tessie(QWidget):
    def __init__(self, dimension=None):
        super(Tessie, self).__init__()
        layout = QVBoxLayout()
        self.setLayout(layout)

        open_button = QPushButton("Open Tessie Webpage in Browser")
        open_button.clicked.connect(self.launch_tessie_webpage)

        web_view = QWebEngineView()
        web_view.load(QUrl(alternative_tessie_url))
        web_view.setZoomFactor(0.8)

        layout.addWidget(open_button)
        layout.addWidget(web_view)

    def launch_tessie_webpage(self):
        webbrowser.open(tessie_url)
