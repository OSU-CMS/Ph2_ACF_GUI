from PyQt5 import QtWidgets

class F4TModuleInputWidget(QtWidgets.QDialog):
    def __init__(self, initial_inputs=5, parent=None):
        super().__init__(parent)

        self.setWindowTitle("Enter Module IDs")
        self.resize(400, 300)

        main_layout = QtWidgets.QVBoxLayout(self)

        # Scroll area setup
        self.scroll_area = QtWidgets.QScrollArea(self)
        self.scroll_area.setWidgetResizable(True)
        main_layout.addWidget(self.scroll_area)

        # Inner widget that holds the input fields
        self.inner_widget = QtWidgets.QWidget()
        self.inner_layout = QtWidgets.QVBoxLayout(self.inner_widget)

        self.scroll_area.setWidget(self.inner_widget)

        # Store references to input fields
        self.input_fields = []

        # Add initial fields
        for _ in range(initial_inputs):
            self.add_input_field()

        # Add Input button
        self.add_button = QtWidgets.QPushButton("Add Input Field")
        self.add_button.clicked.connect(self.add_input_field)
        main_layout.addWidget(self.add_button)

        # OK/Cancel buttons
        button_box = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel
        )
        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.reject)
        main_layout.addWidget(button_box)

    def add_input_field(self):
        """Adds a new input field."""
        line_edit = QtWidgets.QLineEdit()
        line_edit.setPlaceholderText("Enter ID (e.g., SH0101)")
        self.inner_layout.addWidget(line_edit)
        self.input_fields.append(line_edit)

    def get_all_inputs(self):
        """Return non-empty values from input fields."""
        return [f.text().strip() for f in self.input_fields if f.text().strip()]
