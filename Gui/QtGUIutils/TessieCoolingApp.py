import webbrowser
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

        if dimension:
            self.setFixedSize(*dimension)

    def launch_tessie_webpage(self):
        webbrowser.open(tessie_url)

    def inject_css(self, web_view, js):
        web_view.page().runJavaScript(js)


