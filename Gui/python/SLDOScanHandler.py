"""
Class to perform the SLDO curve scanning
"""

from PyQt5.QtCore import QObject, QThread, pyqtSignal

class sldo_curve_worker(QObject):
    finished = pyqtSignal()

    def run(self):
        # Turn off instruments

        # ramp up LV

        # Measure current and voltage from LV

        # Measure values from multimeter

        # ramp down LV

        # Store Iin, Vin, Vmeas where Vmeas is the average of the voltages
        # from the multimeter, Vin is the voltage measured from LV, and Iin
        # is current measured from LV.
        # NOTE to get multimeter list, looks like you'll need to read trace data

        # Repeat above steps for all pins
