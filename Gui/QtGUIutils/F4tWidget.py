from PyQt5 import QtWidgets
from PyQt5.QtCore import pyqtSignal

class F4TModuleInputWidget(QtWidgets.QWidget):
    input_modules_signal = pyqtSignal(list)
    def __init__(self, initial_inputs=5, parent=None):
        super().__init__(parent)

        # Main layout for the widget
        main_layout = QtWidgets.QVBoxLayout(self)

        # Scroll area setup
        self.scroll_area = QtWidgets.QScrollArea(self)
        self.scroll_area.setWidgetResizable(True)
        main_layout.addWidget(self.scroll_area)

        # Inner widget that holds the input fields
        self.inner_widget = QtWidgets.QWidget()
        self.inner_layout = QtWidgets.QVBoxLayout(self.inner_widget)
        self.inner_layout.setContentsMargins(5, 5, 5, 5)
        self.inner_layout.setSpacing(1)

        # Add button
        self.add_button = QtWidgets.QPushButton("Add Input Field")
        self.add_button.clicked.connect(self.add_input_field)
        main_layout.addWidget(self.add_button)

        # Store references to input fields
        self.input_fields = []

        # Add initial fields
        for _ in range(initial_inputs):
            self.add_input_field()

        self.scroll_area.setWidget(self.inner_widget)

    def add_input_field(self):
        """Adds a new input field."""
        line_edit = QtWidgets.QLineEdit()
        line_edit.setPlaceholderText("Enter ID (e.g., SH0101)")
        self.inner_layout.addWidget(line_edit)
        self.input_fields.append(line_edit)

    def get_all_inputs(self):
        """Return a list of all entered values."""
        self.input_modules_signal.emit([field.text() for field in self.input_fields])
