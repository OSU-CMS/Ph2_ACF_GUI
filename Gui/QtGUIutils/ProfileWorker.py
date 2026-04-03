from PyQt5.QtCore import QObject, pyqtSignal

class ProfileWorker(QObject):
    finished = pyqtSignal(dict, str)

    def __init__(self, chamber):
        super().__init__()
        self.chamber = chamber

    def run(self):
        result = self.chamber.query_profiles(force_refresh=True)
        if result is None:
            profiles, source = {}, "error"
        else:
            profiles, source = result
            
        self.finished.emit(profiles, source)