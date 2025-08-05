from PyQt5 import QtWebEngine
from PyQt5.QtWidgets import QWidget, QPushButton, QVBoxLayout
from PyQt5.QtWebEngineWidgets import QWebEngineView
from PyQt5.QtCore import QUrl
from Gui.python.logging_config import get_logger
logger = get_logger(__name__)

from Gui.siteSettings import tessie_url, alternative_tessie_url



class Tessie(QWidget):
    def __init__(self, dimension=None):
        super(Tessie, self).__init__()
        layout = QVBoxLayout()
        self.setLayout(layout)

        open_button = QPushButton("Open Tessie Webpage in Browser")
        open_button.clicked.connect(self.launch_tessie_webpage)

        QtWebEngine.QtWebEngine.initialize()
        web_view = QWebEngineView()
        web_view.load(QUrl(alternative_tessie_url))
        web_view.setZoomFactor(0.8)

        # Define your CSS as a JavaScript string
        css = """
        body {
            background-color: #222;
            color: #e0e0e0;
        }
        """
        # Convert CSS into a <style> tag and inject it using JavaScript
        js = f"""
        var style = document.createElement('style');
        style.type = 'text/css';
        style.innerText = `{css}`;
        document.head.appendChild(style);
        """

        web_view.loadFinished.connect(lambda: self.inject_css(web_view, js))

        layout.addWidget(open_button)
        layout.addWidget(web_view)

    def launch_tessie_webpage(self):
        """
        Launch new window that will display full webpage using QWebEngine.
        
        Note: We can't make use of default browser on system since we are running within docker image.
        """
        print("Inside launch_tessie_webpage")
        # Create widget
        self.browser = QWidget()
        layout = QVBoxLayout()

        web_view = QWebEngineView()
        web_view.load(QUrl(tessie_url))
        
        layout.addWidget(web_view)
        self.browser.setLayout(layout)
        self.browser.show()


    def inject_css(self, web_view, js):
        web_view.page().runJavaScript(js)


