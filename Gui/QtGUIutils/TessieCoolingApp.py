from PyQt5 import QtWebEngine
from PyQt5.QtWidgets import (
    QWidget, QPushButton, QVBoxLayout, QHBoxLayout, QGridLayout, 
    QLabel, QGroupBox, QFrame
)
from PyQt5.QtWebEngineWidgets import QWebEngineView
from PyQt5.QtCore import QUrl, QTimer, pyqtSignal
from PyQt5.QtGui import QFont
from Gui.python.logging_config import get_logger
from Gui.siteSettings import tessie_url, alternative_tessie_url
import threading
import time

logger = get_logger(__name__)


class TessieCoolingApp(QWidget):
    """Widget for monitoring and controlling Tessie (PSI Coldbox) TEC temperatures."""
    
    temp_update_signal = pyqtSignal(list)
    
    def __init__(self, master=None):
        super(TessieCoolingApp, self).__init__()
        self.master = master
        self.instruments = None
        self._temp_thread = None
        self._temp_thread_stop = False
        
        # Initialize UI
        self.initUI()
        
        # Connect signals
        self.temp_update_signal.connect(self.update_temperature_display)
        
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
            temp_label.setStyleSheet("QLabel { color: blue; }")
            
            # Add to layout (4 channels per row)
            row = i // 4
            col = (i % 4) * 2
            
            temp_layout.addWidget(tec_label, row, col)
            temp_layout.addWidget(temp_label, row, col + 1)
            
            self.tec_labels[channel] = tec_label
            self.temp_labels[channel] = temp_label
        
        self.temp_group.setLayout(temp_layout)
        main_layout.addWidget(self.temp_group)
        
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
        
        self.web_interface_button = QPushButton("Open Web Interface")
        self.web_interface_button.clicked.connect(self.open_web_interface)
        control_layout.addWidget(self.web_interface_button)
        
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
        """Start the background thread for temperature monitoring."""
        if self._temp_thread is not None:
            self.stop_temperature_monitoring()
        
        def temp_monitor_worker():
            logger.info("Starting Tessie temperature monitoring thread...")
            while not self._temp_thread_stop:
                try:
                    if self.instruments:
                        coldbox = self.instruments.get_instruments().get("cb", None)
                        if coldbox is not None:
                            # Read all 8 TEC temperatures
                            temperatures = coldbox.read("TEMPERATURE_MEASURED")
                            if isinstance(temperatures, list) and len(temperatures) == 8:
                                self.temp_update_signal.emit(temperatures)
                            else:
                                logger.warning(f"Unexpected temperature data format: {temperatures}")
                        else:
                            logger.warning("Coldbox not found in instruments")
                    else:
                        logger.warning("No instruments available")
                except Exception as e:
                    logger.error(f"Error reading temperatures: {e}")
                
                # Wait 2 seconds between updates
                time.sleep(2)
            
            logger.info("Tessie temperature monitoring thread stopped.")
        
        self._temp_thread_stop = False
        self._temp_thread = threading.Thread(target=temp_monitor_worker, daemon=True)
        self._temp_thread.start()
        
        self.status_label.setText("Monitoring active")
    
    def stop_temperature_monitoring(self):
        """Stop the temperature monitoring thread."""
        if self._temp_thread is not None:
            self._temp_thread_stop = True
            self._temp_thread = None
        
        self.status_label.setText("Monitoring stopped")
    
    def update_temperature_display(self, temperatures):
        """Update the temperature display with new values."""
        try:
            for i, temp in enumerate(temperatures):
                channel = i + 1
                if channel in self.temp_labels:
                    if isinstance(temp, (int, float)):
                        temp_str = f"{temp:.2f}°C"
                        # Color coding based on temperature
                        if temp < -30:
                            color = "blue"
                        elif temp < 0:
                            color = "cyan"
                        elif temp < 25:
                            color = "green"
                        elif temp < 40:
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
    
    def open_web_interface(self):
        """Open the Tessie web interface in a new window."""
        try:
            web_window = TessieWebInterface()
            web_window.show()
        except Exception as e:
            logger.error(f"Error opening web interface: {e}")
            self.status_label.setText("Failed to open web interface")
    
    def set_instruments(self, instruments):
        """Set the instrument cluster reference."""
        self.instruments = instruments
        self.update_connection_status()
        
        if instruments:
            self.start_temperature_monitoring()
        else:
            self.stop_temperature_monitoring()
    
    def closeEvent(self, event):
        """Handle widget close event."""
        self.stop_temperature_monitoring()
        event.accept()


class TessieWebInterface(QWidget):
    """Separate window for Tessie web interface."""
    
    def __init__(self):
        super(TessieWebInterface, self).__init__()
        self.setWindowTitle("Tessie Web Interface")
        self.setGeometry(100, 100, 1200, 800)
        
        layout = QVBoxLayout()
        
        # Web view
        self.web_view = QWebEngineView()
        
        # Try primary URL first, fallback to alternative
        try:
            self.web_view.load(QUrl(tessie_url))
        except:
            try:
                self.web_view.load(QUrl(alternative_tessie_url))
            except:
                logger.error("Failed to load Tessie web interface URLs")
        
        layout.addWidget(self.web_view)
        
        # Control buttons
        button_layout = QHBoxLayout()
        
        refresh_btn = QPushButton("Refresh")
        refresh_btn.clicked.connect(self.web_view.reload)
        button_layout.addWidget(refresh_btn)
        
        close_btn = QPushButton("Close")
        close_btn.clicked.connect(self.close)
        button_layout.addWidget(close_btn)
        
        button_layout.addStretch()
        layout.addLayout(button_layout)
        
        self.setLayout(layout)


# Alias for backward compatibility
Tessie = TessieCoolingApp