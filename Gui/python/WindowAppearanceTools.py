"""
List of functions used to affect the appearance of windows in pyQT.

Functions:
apply_dark_mode
create_logo_widget
"""

from PyQt5.QtGui import QPalette, QColor, QImage, QPixmap
from PyQt5.QtWidgets import QGroupBox, QHBoxLayout, QLabel
from PyQt5.QtCore import Qt, QSize


def apply_dark_mode(palette: QPalette) -> QPalette:
    """
    Change the theme of a window to dark mode.

    returns QPalette to be added to window
    """
    palette.setColor(QPalette.Window, QColor(53, 53, 53))

    palette.setColor(QPalette.WindowText, Qt.white)
    palette.setColor(QPalette.Base, QColor(25, 25, 25))
    palette.setColor(QPalette.AlternateBase, QColor(53, 53, 53))
    palette.setColor(QPalette.ToolTipBase, Qt.darkGray)
    palette.setColor(QPalette.ToolTipText, Qt.white)
    palette.setColor(QPalette.Text, Qt.white)
    palette.setColor(QPalette.Button, QColor(53, 53, 53))
    palette.setColor(QPalette.ButtonText, Qt.white)
    palette.setColor(QPalette.BrightText, Qt.red)
    palette.setColor(QPalette.Link, QColor(42, 130, 218))
    palette.setColor(QPalette.Highlight, QColor(42, 130, 218))
    palette.setColor(QPalette.HighlightedText, Qt.black)

    palette.setColor(QPalette.Disabled, QPalette.Window, Qt.lightGray)
    palette.setColor(QPalette.Disabled, QPalette.WindowText, Qt.gray)
    palette.setColor(QPalette.Disabled, QPalette.Base, Qt.darkGray)
    palette.setColor(QPalette.Disabled, QPalette.ToolTipBase, Qt.darkGray)
    palette.setColor(QPalette.Disabled, QPalette.ToolTipText, Qt.white)
    palette.setColor(QPalette.Disabled, QPalette.Text, Qt.gray)
    palette.setColor(QPalette.Disabled, QPalette.Button, QColor(73, 73, 73))
    palette.setColor(QPalette.Disabled, QPalette.ButtonText, Qt.lightGray)
    palette.setColor(QPalette.Disabled, QPalette.BrightText, Qt.lightGray)
    palette.setColor(QPalette.Disabled, QPalette.Highlight, Qt.lightGray)
    palette.setColor(QPalette.Disabled, QPalette.HighlightedText, Qt.gray)
    return palette


def create_logo_widget(*image_paths: str) -> QGroupBox:
    """Use to create widget to display logo."""
    logo_groupbox = QGroupBox("")
    logo_groupbox.setCheckable(False)
    logo_groupbox.setMaximumHeight(100)
    logo_layout = QHBoxLayout()
    print("In create_logo_widget")
    for i, path in enumerate(image_paths):
        logo_label = QLabel()
        image = QImage(path).scaled(
            QSize(200, 60), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        pixmap = QPixmap.fromImage(image)
        logo_label.setPixmap(pixmap)
        logo_layout.addWidget(logo_label)

        # Only add spacers in between logos
        if i != len(image_paths) - 1:
            logo_layout.addStretch(1)

    logo_groupbox.setLayout(logo_layout)
    return logo_groupbox
