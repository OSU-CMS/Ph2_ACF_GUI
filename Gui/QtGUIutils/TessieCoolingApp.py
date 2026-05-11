from PyQt5.QtWidgets import (
    QWidget, QPushButton, QVBoxLayout, QHBoxLayout, QGridLayout, 
    QLabel, QGroupBox, QFrame
)
from PyQt5.QtCore import QTimer, pyqtSignal, QThread, QObject, pyqtSlot,QMetaObject, Qt
from PyQt5.QtGui import QFont
from Gui.python.logging_config import get_logger
import time

logger = get_logger(__name__)


class TessieMonitorWorker(QObject):
    """Worker object that polls PSIColdbox in a QThread via QTimer."""

    temp_update = pyqtSignal(list)
    env_update = pyqtSignal(float, float)
    set_update = pyqtSignal(list)
    status_msg = pyqtSignal(str)

    def __init__(self, instruments=None, interval_ms: int = 2000):
        super().__init__()
        self._instruments = instruments
        self._timer = None
        self._interval_ms = interval_ms

    @pyqtSlot()
    def start(self):
        # Create the timer in this thread context
        if self._timer is None:
            self._timer = QTimer(self)
            self._timer.timeout.connect(self.poll)
        self._timer.start(self._interval_ms)
        self.status_msg.emit("Monitoring active")

    @pyqtSlot()
    def stop(self):
        if self._timer is not None:
            self._timer.stop()
            self.status_msg.emit("Monitoring stopped")

    @pyqtSlot()
    def poll(self):
        try:
            if not self._instruments:
                return
            coldbox = self._instruments.get_instruments().get("cb", None)
            if coldbox is None:
                return

            # Temperatures
            try:
                temperatures = coldbox.read("TEMPERATURE_MEASURED")
                if isinstance(temperatures, list) and len(temperatures) == 8:
                    self.temp_update.emit(temperatures)
            except Exception as e:
                logger.debug(f"Worker temp read issue: {e}")

            # Temperature setpoints
            try:
                setpoints = coldbox.read("TEMPERATURE_SET")
                if isinstance(setpoints, list) and len(setpoints) == 8:
                    self.set_update.emit(setpoints)
            except Exception as e:
                logger.debug(f"Worker setpoint read issue: {e}")

            # Environment
            rh = None
            dp = None
            try:
                rh = coldbox.read("RELATIVE_HUMIDITY")
            except Exception:
                pass
            try:
                dp = coldbox.read("DEW_POINT")
            except Exception:
                pass

            if not isinstance(rh, (int, float)) or not isinstance(dp, (int, float)):
                # fallback to explicit query
                try:
                    rh_q = coldbox.query("RELATIVE_HUMIDITY")
                    dp_q = coldbox.query("DEW_POINT")
                    rh = rh if isinstance(rh, (int, float)) else rh_q
                    dp = dp if isinstance(dp, (int, float)) else dp_q
                except Exception:
                    pass

            if isinstance(rh, (int, float)) and isinstance(dp, (int, float)):
                self.env_update.emit(float(rh), float(dp))
        except Exception as e:
            logger.error(f"Worker poll error: {e}")

    @pyqtSlot(object)
    def set_instruments(self, instruments):
        self._instruments = instruments


class TessieCoolingApp(QWidget):
    """Widget for monitoring and controlling Tessie (PSI Coldbox) TEC temperatures."""
    
    temp_update_signal = pyqtSignal(list)
    # (rh in %, dew point in C)
    env_update_signal = pyqtSignal(float, float)
    set_update_signal = pyqtSignal(list)
    
    def __init__(self, master=None):
        super(TessieCoolingApp, self).__init__()
        self.master = master
        self.instruments = None
        self._thread = None
        self._worker = None
        # Latest cached values
        self._latest_temperatures = None  # type: list
        self._last_temp_update_ts = 0.0
        self._latest_rh = None  # type: float
        self._latest_dp = None  # type: float
        self._last_env_update_ts = 0.0
        self._latest_setpoints = None  # type: list
        self._last_set_update_ts = 0.0
        
        # Initialize UI
        self.initUI()
        
        # Connect signals (widget-owned signals kept for compatibility)
        self.temp_update_signal.connect(self.update_temperature_display)
        self.env_update_signal.connect(self.update_environment_display)
        self.set_update_signal.connect(self.update_setpoints_cache)
        
        # Start temperature monitoring if instruments are available
        if self.master and hasattr(self.master, 'instruments'):
            self.instruments = self.master.instruments
            self.start_temperature_monitoring()
    
    def initUI(self):
        """Initialize the user interface."""
        main_layout = QVBoxLayout()
        
        # Title
        title_label = QLabel("Tessie Coldbox Monitor")
        title_label.setFont(QFont("Arial", 14, QFont.Bold))
        main_layout.addWidget(title_label)
        
        # Temperature display section
        self.temp_group = QGroupBox("TEC Temperatures")
        temp_layout = QGridLayout()
        
        # Create labels for each TEC channel (1-8)
        self.tec_labels = {}
        self.temp_labels = {}
        
        for i in range(8):
            channel = i + 1
            
            # TEC channel label
            tec_label = QLabel(f"TEC {channel}:")
            tec_label.setFont(QFont("Arial", 10, QFont.Bold))
            
            # Temperature value label
            temp_label = QLabel("--°C")
            temp_label.setFont(QFont("Courier", 10))
            temp_label.setStyleSheet("QLabel { color: orange; }")
            
            # Add to layout (4 channels per row)
            if i < 4:
                row = 2
                col = 2 * i + 1
            else:
                row = 1
                if ((i + 1) % 4) == 0:
                    col = 1
                else:
                    col = 2*(( 4 - (i + 1) % 4)) + 1
            
            temp_layout.addWidget(tec_label, row, col)
            temp_layout.addWidget(temp_label, row, col + 1)
            
            self.tec_labels[channel] = tec_label
            self.temp_labels[channel] = temp_label
        
        self.temp_group.setLayout(temp_layout)
        main_layout.addWidget(self.temp_group)

        # Environment display section (Relative Humidity and Dew Point)
        self.env_group = QGroupBox("Environment")
        env_layout = QGridLayout()

        rh_title = QLabel("Rel. Humidity:")
        rh_title.setFont(QFont("Arial", 10, QFont.Bold))
        self.rh_value_label = QLabel("-- %")
        self.rh_value_label.setFont(QFont("Courier", 10))
        self.rh_value_label.setStyleSheet("QLabel { color: orange; }")

        dp_title = QLabel("Dew Point:")
        dp_title.setFont(QFont("Arial", 10, QFont.Bold))
        self.dp_value_label = QLabel("-- °C")
        self.dp_value_label.setFont(QFont("Courier", 10))
        self.dp_value_label.setStyleSheet("QLabel { color: orange; }")

        env_layout.addWidget(rh_title, 0, 0)
        env_layout.addWidget(self.rh_value_label, 0, 1)
        env_layout.addWidget(dp_title, 1, 0)
        env_layout.addWidget(self.dp_value_label, 1, 1)

        self.env_group.setLayout(env_layout)
        main_layout.addWidget(self.env_group)
        
        # Status section
        self.status_group = QGroupBox("Status")
        status_layout = QVBoxLayout()
        
        self.status_label = QLabel("Initializing...")
        self.status_label.setFont(QFont("Arial", 9))
        status_layout.addWidget(self.status_label)
        
        self.connection_label = QLabel("Connection: Unknown")
        self.connection_label.setFont(QFont("Arial", 9))
        status_layout.addWidget(self.connection_label)
        
        self.status_group.setLayout(status_layout)
        main_layout.addWidget(self.status_group)
        
        # Control buttons section
        control_layout = QHBoxLayout()
        
        self.refresh_button = QPushButton("Refresh")
        self.refresh_button.clicked.connect(self.refresh_temperatures)
        control_layout.addWidget(self.refresh_button)
        
        control_layout.addStretch()
        main_layout.addLayout(control_layout)
        
        # Add separator line
        line = QFrame()
        line.setFrameShape(QFrame.HLine)
        line.setFrameShadow(QFrame.Sunken)
        main_layout.addWidget(line)
        
        self.setLayout(main_layout)
        
        # Set initial status
        self.update_connection_status()
    
    def start_temperature_monitoring(self):
        """Start the QThread-based background monitoring."""
        if self._thread is not None:
            self.stop_temperature_monitoring()

        self._worker = TessieMonitorWorker(self.instruments, interval_ms=2000)
        self._thread = QThread(self)
        self._worker.moveToThread(self._thread)

        # Wire signals
        self._thread.started.connect(self._worker.start)
        self._worker.status_msg.connect(self.status_label.setText)
        # Bridge worker signals to existing slots
        self._worker.temp_update.connect(self.update_temperature_display)
        self._worker.env_update.connect(self.update_environment_display)
        self._worker.set_update.connect(self.update_setpoints_cache)

        # Clean-up when thread finishes
        self._thread.finished.connect(self._worker.deleteLater)
        self._thread.start()
        self.status_label.setText("Monitoring active")
    
    def stop_temperature_monitoring(self):
        """Stop the QThread-based monitoring."""
        if self._worker is not None:
            try:
                QMetaObject.invokeMethod(self._worker, "stop", Qt.QueuedConnection)
            except Exception as e:
                logger.debug(f"Failed to queue worker.stop(): {e}")
        if self._thread is not None:
            self._thread.quit()
            self._thread.wait(2000)
            self._thread = None
        self._worker = None
        self.status_label.setText("Monitoring stopped")
    
    def update_temperature_display(self, temperatures):
        """Update the temperature display with new values."""
        try:
            # cache latest temperatures and timestamp
            if isinstance(temperatures, list) and len(temperatures) == 8:
                self._latest_temperatures = [float(t) if isinstance(t, (int, float)) else t for t in temperatures]
                self._last_temp_update_ts = time.time()
            for i, temp in enumerate(temperatures):
                channel = i + 1
                if channel in self.temp_labels:
                    if isinstance(temp, (int, float)):
                        temp_str = f"{temp:.2f}°C"
                        # Color coding based on temperature
                        if temp < 10:
                            color = "green"
                        elif temp < 25:
                            color = "orange"
                        else:
                            color = "red"
                        
                        self.temp_labels[channel].setText(temp_str)
                        self.temp_labels[channel].setStyleSheet(f"QLabel {{ color: {color}; }}")
                    else:
                        self.temp_labels[channel].setText("N/A")
                        self.temp_labels[channel].setStyleSheet("QLabel { color: gray; }")
            
            self.status_label.setText(f"Last update: {time.strftime('%H:%M:%S')}")
            
        except Exception as e:
            logger.error(f"Error updating temperature display: {e}")
            self.status_label.setText(f"Display error: {str(e)}")

    def update_environment_display(self, rh_value: float, dew_point: float):
        """Update the environment display with RH (%) and Dew Point (°C)."""
        try:
            # cache latest env values and timestamp
            if isinstance(rh_value, (int, float)):
                self._latest_rh = float(rh_value)
                self._last_env_update_ts = time.time()
            if isinstance(dew_point, (int, float)):
                self._latest_dp = float(dew_point)
                self._last_env_update_ts = time.time()
            # Relative Humidity formatting and color coding
            if isinstance(rh_value, (int, float)):
                rh_str = f"{rh_value:.1f} %"
                if rh_value < 30:
                    rh_color = "green"
                elif rh_value < 60:
                    rh_color = "orange"
                else:
                    rh_color = "red"
                self.rh_value_label.setText(rh_str)
                self.rh_value_label.setStyleSheet(f"QLabel {{ color: {rh_color}; }}")
            else:
                self.rh_value_label.setText("N/A")
                self.rh_value_label.setStyleSheet("QLabel { color: gray; }")

            # Dew Point formatting (keep color neutral/informational)
            if isinstance(dew_point, (int, float)):
                dp_str = f"{dew_point:.2f} °C"
                self.dp_value_label.setText(dp_str)
                self.dp_value_label.setStyleSheet("QLabel { color: cornflowerblue; }")
            else:
                self.dp_value_label.setText("N/A")
                self.dp_value_label.setStyleSheet("QLabel { color: gray; }")
        except Exception as e:
            logger.error(f"Error updating environment display: {e}")

    def update_setpoints_cache(self, setpoints):
        """Cache the latest temperature setpoints list (length 8)."""
        try:
            if isinstance(setpoints, list) and len(setpoints) == 8:
                self._latest_setpoints = [float(s) if isinstance(s, (int, float)) else s for s in setpoints]
                self._last_set_update_ts = time.time()
        except Exception as e:
            logger.debug(f"Error caching setpoints: {e}")

    # --- Public getters for other components (e.g., TestHandler) ---
    def get_latest_temperatures(self, with_timestamp: bool = False):
        """Return a copy of the latest temperatures [8] or None. If with_timestamp, also return the unix ts."""
        temps = list(self._latest_temperatures) if isinstance(self._latest_temperatures, list) else None
        if with_timestamp:
            return temps, self._last_temp_update_ts
        return temps

    def get_latest_env(self, with_timestamp: bool = False):
        """Return (rh, dew_point) or (rh, dew_point, ts) if with_timestamp."""
        rh = self._latest_rh
        dp = self._latest_dp
        if with_timestamp:
            return rh, dp, self._last_env_update_ts
        return rh, dp
    
    def get_latest_setpoints(self, with_timestamp: bool = False):
        """Return list of 8 temperature setpoints or None. If with_timestamp, also return ts."""
        s = list(self._latest_setpoints) if isinstance(self._latest_setpoints, list) else None
        if with_timestamp:
            return s, self._last_set_update_ts
        return s
    
    def refresh_temperatures(self):
        """Manually refresh temperature readings."""
        try:
            if self.instruments:
                coldbox = self.instruments.get_instruments().get("cb", None)
                if coldbox is not None:
                    # Force a fresh query
                    temperatures = coldbox.query("TEMPERATURE_MEASURED", force_query=True)
                    if isinstance(temperatures, list) and len(temperatures) == 8:
                        self.update_temperature_display(temperatures)
                        # Also refresh environment values
                        try:
                            rh = coldbox.query("RELATIVE_HUMIDITY", force_query=True)
                            dp = coldbox.query("DEW_POINT", force_query=True)
                            if isinstance(rh, (int, float)) and isinstance(dp, (int, float)):
                                self.update_environment_display(float(rh), float(dp))
                        except Exception as e:
                            logger.debug(f"Environment manual refresh issue: {e}")
                        self.status_label.setText("Manual refresh completed")
                    else:
                        self.status_label.setText("Invalid temperature data received")
                else:
                    self.status_label.setText("Coldbox not available")
            else:
                self.status_label.setText("No instruments connected")
        except Exception as e:
            logger.error(f"Error during manual refresh: {e}")
            self.status_label.setText(f"Refresh failed: {str(e)}")
    
    def update_connection_status(self):
        """Update the connection status display."""
        try:
            if self.instruments:
                coldbox = self.instruments.get_instruments().get("cb", None)
                if coldbox is not None:
                    self.connection_label.setText("Connection: Connected")
                    self.connection_label.setStyleSheet("QLabel { color: green; }")
                else:
                    self.connection_label.setText("Connection: Coldbox not found")
                    self.connection_label.setStyleSheet("QLabel { color: orange; }")
            else:
                self.connection_label.setText("Connection: No instruments")
                self.connection_label.setStyleSheet("QLabel { color: red; }")
        except Exception as e:
            logger.error(f"Error checking connection status: {e}")
            self.connection_label.setText("Connection: Error")
            self.connection_label.setStyleSheet("QLabel { color: red; }")
    
    def set_instruments(self, instruments):
        """Set the instrument cluster reference."""
        self.instruments = instruments
        self.update_connection_status()
        
        if instruments:
            # Update or start worker
            if self._worker is not None:
                self._worker.set_instruments(instruments)
            else:
                self.start_temperature_monitoring()
        else:
            self.stop_temperature_monitoring()
    
    def closeEvent(self, event):
        """Handle widget close event."""
        self.stop_temperature_monitoring()
        event.accept()


# Alias for backward compatibility
Tessie = TessieCoolingApp