"""
Controller for Tessie. Meant to be paired with TessieCoolingApp.py

"""
from PyQt5.QtCore import pyqtSignal

from icicle.psi_coldbox import PSIColdbox

class TessieHandler():
    temperature = pyqtSignal(list[float])
    humidity = pyqtSignal(float)
    status = pyqtSignal(int)

    def __init__(self):
        # Setup icicle connection to Tessie
        self.coldbox = PSIColdbox()

    def get_temperatures():...
    def get_humidity():...
    def get_status():...

    def set_temperature():...



        
