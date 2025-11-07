import os
import subprocess
from Gui.python.logging_config import get_logger
import requests
import re
import traceback

from PyQt5.QtCore import QSize, Qt, pyqtSignal, QEvent, QTimer, QSortFilterProxyModel, QStringListModel
from PyQt5.QtGui import QPixmap, QImage
from PyQt5.QtWidgets import (
    QComboBox,
    QGridLayout,
    QGroupBox,
    QLabel,
    QPushButton,
    QHBoxLayout,
    QVBoxLayout,
    QCheckBox,
    QWidget,
    QMessageBox,
    QLineEdit,
    QRadioButton,
    QCompleter,
)
from Gui.QtGUIutils.Loading import LoadingThread
from Gui.QtGUIutils.QtFwCheckDetails import QtFwCheckDetails
from Gui.python.CustomizedWidget import BeBoardBox
from Gui.GUIutils.settings import firmware_image, ModuleLaneMap
from Gui.siteSettings import (
    FC7List,
    ModuleCurrentMap,
    icicle_instrument_setup,
    WorkingChannels,
    json_setup,
    cooler
)
from icicle.icicle.instrument_cluster import DummyInstrument
from InnerTrackerTests.TestSequences import TestList



logger = get_logger(__name__)


# from Gui.QtGUIutils.QtApplication import *

# from Gui.python.Firmware import *
# from Gui.GUIutils.DBConnection import *

class SummaryBox(QWidget):
    def __init__(self, master, module, index=0):
        super(SummaryBox, self).__init__()
        self.module = module
        self.master = master
        self.index = index
        self.result = False
        self.verboseResult = {}
        self.chipSwitches = {}

        self.mainLayout = QGridLayout()

        self.initResult()
        if icicle_instrument_setup is not None:
            self.measureFwPar()
        # self.checkFwPar()
        self.setLayout(self.mainLayout)

    def initResult(self):
        for i in ModuleLaneMap[self.module.getType()].keys():
            self.verboseResult[i] = {}
            self.chipSwitches[i] = True

    def measureFwPar(self):
        for index, (key, value) in enumerate(self.verboseResult.items()):
            value["Power-up Mode"] = "SLDO" #self.PowerModeCombo.currentText()
            # Fixme
            measureList = [
                "Set Bias Voltage (V)",
                "Set LV Current (A)",
            ]
            for item in measureList:
                if "LV Current" in item:
                    value[item] = ModuleCurrentMap[self.module.getType()]
                if "Bias Voltage" in item:
                    value[item] = icicle_instrument_setup["instrument_dict"]["hv"][
                        "default_voltage"
                    ]
                    # assumes only 1 HV titled 'hv' in instruments.json
            self.verboseResult[key] = value

    @staticmethod
    def checkFwPar(pfirmwareName, module_type, fc7_ip):
        # To be finished
        try:
            # self.result = True
            FWisPresent = False
            boardtype = "RD53B"
            if "CROC" in module_type:
                boardtype = "RD53B"
            else:
                boardtype = "RD53A"
            print("board type is: {0}".format(boardtype))
            # updating uri value in template xml file with correct fc7 ip address, as specified in siteSettings.py
            # fc7_ip = site_settings.FC7List[pfirmwareName] #Commented because I don't think we need it.  Remove line after test.
            print("The fc7 ip is: {0}".format(fc7_ip))
            uricmd = "sed -i -e 's/fc7-1/{0}/g' {1}/Gui/CMSIT_{2}.xml".format(
                fc7_ip, os.environ.get("GUI_dir"), boardtype
            )
            subprocess.call([uricmd], shell=True)
            print("updated the uri value")
            firmwareImage = firmware_image[module_type][
                os.environ.get("Ph2_ACF_VERSION")
            ]

            print("checking if firmware is on the SD card for {}".format(firmwareImage))
            fwlist = subprocess.run(
                [
                    "fpgaconfig",
                    "-c",
                    os.environ.get("GUI_dir") + "/Gui/CMSIT_{}.xml".format(boardtype),
                    "-l",
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

            # fwlist = subprocess.run(["fpgaconfig","-c",os.environ.get('PH2ACF_BASE_DIR')+'/test/CMSIT_{}.xml'.format(boardtype),"-l"],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            print("firmwarelist is {0}".format(fwlist.stdout.decode("UTF-8")))
            print("firmwareImage is {0}".format(firmwareImage))
            if firmwareImage in fwlist.stdout.decode("UTF-8"):
                FWisPresent = True
                print("firmware found")
            else:
                try:
                    print(
                        "Saving fw image {0} to SD card".format(
                            os.environ.get("GUI_dir")
                            + "/FirmwareImages/"
                            + firmwareImage
                        )
                    )
                    fwsave = subprocess.run(
                        [
                            "fpgaconfig",
                            "-c",
                            os.environ.get("GUI_dir")
                            + "/Gui/CMSIT_{}.xml".format(boardtype),
                            "-f",
                            "{}".format(
                                os.environ.get("GUI_dir")
                                + "/FirmwareImages/"
                                + firmwareImage
                            ),
                            "-i",
                            "{}".format(firmwareImage),
                        ],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                    )
                    # self.fw_process.start("fpgaconfig",["-c","CMSIT.xml","-f","{}".format(os.environ.get("GUI_dir")+'/FirmwareImages/' + self.firmwareImage),"-i","{}".format(self.firmwareImage)])
                    print(fwsave.stdout.decode("UTF-8"))
                    FWisPresent = True
                except OSError:
                    logger.error(
                        "unable to save {0} to FC7 SD card".format(
                            os.environ.get("GUI_dir")
                            + "/FirmwareImages/"
                            + firmwareImage
                        )
                    )
                    logger.error(traceback.format_exc())

            if FWisPresent:
                print("Loading FW image")
                fwload = subprocess.run(
                    [
                        "fpgaconfig",
                        "-c",
                        os.environ.get("GUI_dir")
                        + "/Gui/CMSIT_{}.xml".format(boardtype),
                        "-i",
                        "{}".format(firmwareImage),
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                print(fwload.stdout.decode("UTF-8"))
                print("resetting beboard")
                print(
                    f"command: CMSITminiDAQ -f {os.environ.get('GUI_dir') + '/Gui/CMSIT_{}.xml'.format(boardtype)} -r"
                )
                fwreset = subprocess.run(
                    [
                        "CMSITminiDAQ",
                        "-f",
                        os.environ.get("GUI_dir")
                        + "/Gui/CMSIT_{}.xml".format(boardtype),
                        "-r",
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                print(fwreset.stdout.decode("UTF-8"))
                print(fwreset.stderr.decode("UTF-8"))

                print("Firmware image is now loaded")
            logger.debug("Made it to turn on LV")
            return True
        except Exception:
            logger.error(traceback.format_exc())
            return False

    def getDetails(self):
        pass

    def showDetails(self):
        self.measureFwPar()
        self.occupied()
        self.DetailPage = QtFwCheckDetails(self)
        self.DetailPage.closedSignal.connect(self.release)

    def getResult(self):
        return self.result

    def occupied(self):
        self.DetailsButton.setDisabled(True)

    def release(self):
        self.DetailsButton.setDisabled(False)


class QtStartWindow(QWidget):
    onThreadFinishSignal = pyqtSignal()
    loaderSignal = pyqtSignal()
    openRunWindowSignal = pyqtSignal()
    def __init__(self, master, firmware):
        super(QtStartWindow, self).__init__()
        self.master = master
        self.firmware = firmware
        self.mainLayout = QGridLayout()
        self.setLayout(self.mainLayout)
        self.runFlag = False
        self.passCheck = False
        self.setLoginUI()
        self.createHead()
        self.createMain()
        self.createApp()
        self.occupied()

        self.closeFlag = False
        self.loading_counter = 0
        self.loaderSignal.connect(self.loader)
        self.onThreadFinishSignal.connect(self.onThreadFinish)
        self.openRunWindowSignal.connect(self.openRunWindow)

    def setLoginUI(self):
        self.setGeometry(400, 400, 400, 400)
        self.setMinimumWidth(1000)
        self.setWindowTitle("Start a new test")
        self.show()

    def createHead(self):
        self.TestBox = QGroupBox()
        testlayout = QGridLayout()
        TestLabel = QLabel("Test:")
        self.TestCombo = QComboBox()
        self.TestList = TestList
        if not self.master.instruments:
            if "AllScan" in self.TestList:
                self.TestList.remove("AllScan")
            if "QuickTest" in self.TestList:
                self.TestList.remove("QuickTest")
            if "FullSequence" in self.TestList:
                self.TestList.remove("FullSequence")

        self.TestCombo.addItems(self.TestList)
        try:
            self.TestCombo.setEditable(True)
            # Clear any current edit text so the field appears empty
            self.TestCombo.setEditText("")
            self.TestCombo.setMaxVisibleItems(12)

            le = self.TestCombo.lineEdit()
            if le is not None:
                le.setPlaceholderText("Select a test...")
        except Exception:
            logger.debug("Failed to clear TestCombo default text or set placeholder")
        # Make the combo searchable: allow typing and provide a substring-matching completer
        # Create a fuzzy-filter proxy so the completer supports subsequence
        try:
            class _FuzzyProxy(QSortFilterProxyModel):
                def __init__(self, parent=None):
                    super(_FuzzyProxy, self).__init__(parent)
                    self._filter = ""

                def setFilterString(self, s: str):
                    self._filter = s or ""
                    self.invalidateFilter()

                def is_subsequence(self, needle: str, hay: str) -> bool:
                    # case insensitive subsequence match
                    if not needle:
                        return True
                    it = iter(hay.lower())
                    for ch in needle.lower():
                        found = False
                        for h in it:
                            if h == ch:
                                found = True
                                break
                        if not found:
                            return False
                    return True

                def filterAcceptsRow(self, source_row, source_parent):
                    try:
                        if not self._filter:
                            return True
                        idx = self.sourceModel().index(source_row, 0, source_parent)
                        text = str(self.sourceModel().data(idx, Qt.DisplayRole) or "")
                        # Accept if subsequence or contains (fallback)
                        if self.is_subsequence(self._filter, text):
                            return True
                        return self._filter.lower() in text.lower()
                    except Exception:
                        return False

            base_model = QStringListModel(self.TestList, self)
            proxy = _FuzzyProxy(self)
            proxy.setSourceModel(base_model)

            completer = QCompleter(proxy, self)
            # keep a reference so handlers can query popup/currentCompletion
            self._test_completer = completer
            completer.setCaseSensitivity(Qt.CaseInsensitive)
            # Use substring matching so typing any part of the test name will match
            completer.setFilterMode(Qt.MatchContains)
            completer.setCompletionMode(QCompleter.UnfilteredPopupCompletion)

            self.TestCombo.setCompleter(completer)
            # also intercept keys on the completer's popup so Tab can be handled
            try:
                popup = completer.popup()
                if popup is not None:
                    # height to show ~12 items and avoid excessive scrolling.
                    try:
                        desired = 12
                        rowh = popup.sizeHintForRow(0)
                        if not rowh or rowh <= 0:
                            # fallback to font metrics estimate
                            fm = popup.fontMetrics()
                            rowh = fm.height() + 6
                        popup.setMaximumHeight(rowh * desired + 2 * popup.frameWidth())
                    except Exception:
                        logger.debug("Could not set completer popup height:\n" + traceback.format_exc())
                    popup.installEventFilter(self)
            except Exception:
                logger.debug("Failed to install event filter on completer.popup()")
            # When a completion is chosen from the popup (click or Enter), apply it
            # Connect to the string overload of activated to ensure we get a text
            # value when the user chooses a completion (click or Enter on popup).
            try:
                completer.activated[str].connect(self._on_completer_activated)
            except Exception:
                logger.debug("Failed to connect completer.activated[str]")

            # When the user leaves the combo (or presses Enter), try to auto-fill the
            # top match from the available tests if the typed text doesn't exactly match.
            try:
                lineedit = self.TestCombo.lineEdit()
                if lineedit is not None:
                    # handle focus-out and editing finished
                    lineedit.editingFinished.connect(self._apply_best_test_completion)
                    # handle explicit Return/Enter press with a handler that
                    # prefers the completer's currently highlighted popup item
                    try:
                        lineedit.returnPressed.connect(self._on_lineedit_return_pressed)
                    except Exception:
                        logger.debug("Failed to connect returnPressed to handler")
                    # update proxy filter as the user types so fuzzy filtering
                    try:
                        proxy_ref = proxy
                        lineedit.textEdited.connect(lambda s: proxy_ref.setFilterString(s))
                    except Exception:
                        pass
                    # install an event filter so clicking away (focus out) is caught reliably
                    lineedit.installEventFilter(self)
            except Exception:
                logger.debug("Failed to attach handlers to TestCombo.lineEdit()")
        except Exception:
            logger.debug("Failed to attach QCompleter to TestCombo")
        TestLabel.setBuddy(self.TestCombo)

        testlayout.addWidget(TestLabel, 0, 0, 1, 1)
        testlayout.addWidget(self.TestCombo, 0, 1, 1, 1)
        self.TestBox.setLayout(testlayout)

        for beboard in self.firmware:
            beboard.removeModules()
            beboard.removeAllOpticalGroups()

        self.BeBoardWidget = BeBoardBox(self.master, self.firmware)  # FLAG
        self.mainLayout.addWidget(self.TestBox, 0, 0, 1, 2)
        self.mainLayout.addWidget(self.BeBoardWidget, 1, 0, 1, 2)

    def createMain(self):
        ## To be finished
        self.ModuleList = []
        for i, module in enumerate(self.BeBoardWidget.ModuleList):
            ModuleSummaryBox = SummaryBox(master=self.master, module=module)
            self.ModuleList.append(ModuleSummaryBox)
        self.BeBoardWidget.updateList()  ############FIXME:  This may not work for multiple modules at a time.

        # Main vertical layout
        self.txt_box = QGroupBox()
        main_txt_layout = QVBoxLayout()

        # Create the top row as a horizontal layout
        top_row_layout = QHBoxLayout()

        # Create info button
        self.info_button = QPushButton("ℹ️")
        self.info_button.setToolTip("More information")
        self.info_button.setFixedSize(30, 30)
        self.info_button.clicked.connect(lambda:
            QMessageBox.information(self, "Info",
            "<ul><li>Log in to https://panthera.fit.edu/</li>"
            "<li>Go to 'Find Modules'</li>"
            "<li>Enter your module in 'Module Name', press Enter, hit 'Search'</li>"
            "<li>Find the .txt you want, right click, choose 'Copy Link Text'</li>"
            "<li>Enter chip .txt's or enter Panthera url to the right to autofill. Leave blank for default.</li>"
            "<li>Chip .txt URLs will look like:<br>"
            "panthera.fit.edu/panthera_storage/results/<br>"
            "ModuleID**/SequenceID***/ResultID****/<br>"
            "CMSIT_RD53_{ModuleName}_{Port #}_{ChipID}_{IN/OUT}.txt</li>"
            "</ul>")
        )

        # Create other widgetsQt.Checked
        self.customTxtLabel = QLabel("Use custom .txt files:")
        self.customTxtCheck = QCheckBox()

        self.customTxtCheck.setChecked(False)
        self.customTxtCheck.stateChanged.connect(lambda state: self.useCustomTxts(state==Qt.Checked))

        self.txt_entry = QLineEdit()
        self.txt_entry.setPlaceholderText("Panthera .txt's (panthera.fit.edu/panthera_storage/results/...)")
        self.txt_entry.editingFinished.connect(lambda: self.change_chip_txts(self.txt_entry.text().replace(" ", "")))

        # Add widgets to the horizontal row
        top_row_layout.addWidget(self.info_button)
        top_row_layout.addWidget(self.customTxtLabel)
        top_row_layout.addWidget(self.customTxtCheck)
        top_row_layout.addWidget(self.txt_entry)

        # Add top row to main layout
        main_txt_layout.addLayout(top_row_layout)

        # Radio buttons
        radio_layout = QHBoxLayout()
        self.out_radio = QRadioButton("OUT")
        self.in_radio = QRadioButton("IN")
        self.out_radio.setLayoutDirection(Qt.RightToLeft)
        self.in_radio.setLayoutDirection(Qt.RightToLeft)
        self.out_radio.setChecked(True)
        self.out_radio.toggled.connect(lambda checked: checked and self.radio_selected(replaceArgs=("_IN.txt", "_OUT.txt") ))
        self.in_radio.toggled.connect(lambda checked: checked and self.radio_selected(replaceArgs=("_OUT.txt", "_IN.txt") ))

        radio_layout.addWidget(self.out_radio)
        radio_layout.addWidget(self.in_radio)
        radio_layout.addStretch()

        main_txt_layout.addLayout(radio_layout)
        self.txt_box.setLayout(main_txt_layout)
        self.mainLayout.addWidget(self.txt_box, 2, 0, 1, 1)

        for module in self.BeBoardWidget.getModules():
            module.SerialEdit.editingFinished.connect(self.txt_entry.clear)
            module.SerialEdit.editingFinished.connect(lambda:self.customTxtCheck.setChecked(False))

    def _apply_best_test_completion(self):
        """If the current text isn't an exact test name, pick the first reasonable match.

        Preference: exact match -> prefix match -> substring match. If a candidate is
        found, update the combo's edit text and current index.
        """
        try:
            text = self.TestCombo.currentText().strip()
            if not text:
                return

            # If already an exact match, select that index
            idx = self.TestCombo.findText(text, Qt.MatchExactly)
            if idx != -1:
                self.TestCombo.setCurrentIndex(idx)
                return

            # Try prefix match (case-insensitive)
            prefix_matches = [t for t in self.TestList if t.lower().startswith(text.lower())]
            if prefix_matches:
                match = prefix_matches[0]
            else:
                # Fallback to substring match
                substr_matches = [t for t in self.TestList if text.lower() in t.lower()]
                match = substr_matches[0] if substr_matches else None

            if match:
                self.TestCombo.setEditText(match)
                idx = self.TestCombo.findText(match, Qt.MatchExactly)
                if idx != -1:
                    self.TestCombo.setCurrentIndex(idx)
        except Exception:
            logger.debug("_apply_best_test_completion failed:\n" + traceback.format_exc())

    def _adjust_completer_popup_height(self, desired: int = 12):
        """Adjust the completer popup height to show up to `desired` rows.

        This is safe to call frequently; it checks for popup and model presence
        and logs failures.
        """
        try:
            comp = getattr(self, "_test_completer", None)
            if comp is None:
                return
            popup = comp.popup()
            if popup is None:
                return

            # Use the model's current rowCount (filtered matches) so the
            # popup can shrink when fewer matches remain.
            model = popup.model()
            if model is None:
                return
            rows = model.rowCount()
            if rows <= 0:
                rows = 1

            # Limit to desired rows
            rows = min(desired, rows)

            rowh = 0
            try:
                rowh = popup.sizeHintForRow(0)
            except Exception:
                rowh = 0
            if not rowh or rowh <= 0:
                fm = popup.fontMetrics()
                rowh = fm.height() + 6

            popup.setFixedHeight(rowh * rows + 2 * popup.frameWidth())
        except Exception:
            logger.debug("_adjust_completer_popup_height failed:\n" + traceback.format_exc())

    def _on_completer_activated(self, text: str):
        """Handle completer activation (user selected a completion)."""
        try:
            if not text:
                return
            self.TestCombo.setEditText(text)
            idx = self.TestCombo.findText(text, Qt.MatchExactly)
            if idx != -1:
                self.TestCombo.setCurrentIndex(idx)
        except Exception:
            logger.debug("_on_completer_activated failed:\n" + traceback.format_exc())

    def _on_lineedit_return_pressed(self):
        """Handle Return/Enter: prefer the completer's highlighted popup item if present.

        If the completer popup is visible and has a current completion, use it.
        Otherwise fall back to the substring/prefix matching helper.
        """
        try:
            comp = getattr(self, "_test_completer", None)
            if comp is not None:
                popup = comp.popup()
                # If the popup is visible, prefer the current highlighted completion
                current = None
                try:
                    if popup is not None and popup.isVisible():
                        # QCompleter.currentCompletion() returns the highlighted text
                        current = comp.currentCompletion()
                except Exception:
                    logger.debug("Could not query completer popup/currentCompletion:\n" + traceback.format_exc())

                if current:
                    # apply the completion like the activated handler
                    self._on_completer_activated(current)
                    return

            # fallback
            self._apply_best_test_completion()
        except Exception:
            logger.debug("_on_lineedit_return_pressed failed:\n" + traceback.format_exc())

    def eventFilter(self, obj, event):
        """Catch focus-out on the combo's line edit to apply completion reliably."""
        try:
            lineedit = None
            try:
                lineedit = self.TestCombo.lineEdit()
            except Exception:
                pass

            # Intercept Tab to perform shell-like common-prefix completion without
            # moving focus. Also handle focus-out to apply best completion.
            if lineedit is not None and obj is lineedit:
                # Key press (Tab) handling
                if event.type() == QEvent.KeyPress:
                    try:
                        key = event.key()
                        # reset cycle state on any *printable* non-tab key so a new
                        # cycle starts when the user actually types. Avoid
                        # resetting for control keys which have empty event.text().
                        try:
                            is_printable = bool(event.text())
                        except Exception:
                            is_printable = False
                        if is_printable and key not in (Qt.Key_Tab, Qt.Key_Backtab):
                            try:
                                self._tab_state = None
                            except Exception:
                                pass
                            # Adjust the completer popup after the event loop has
                            # processed the filter so the rowCount reflects the
                            # current matches (use singleShot(0)).
                            try:
                                QTimer.singleShot(0, lambda: self._adjust_completer_popup_height())
                            except Exception:
                                pass

                        if key in (Qt.Key_Tab, Qt.Key_Backtab):
                            # Use the original typed base (before Tab) as anchor so
                            # cycling goes through all matches that start with that base.
                            text = lineedit.text() or ""
                            cur = text.strip()

                            state = getattr(self, '_tab_state', None)
                            # Initialize state only if missing; do not re-init just
                            # because the line edit text changed due to completion.
                            if state is None:
                                base = cur
                                prefix_matches = [t for t in self.TestList if t.lower().startswith(base.lower())]
                                matches = prefix_matches if prefix_matches else [t for t in self.TestList if base.lower() in t.lower()]
                                self._tab_state = {'base': base, 'matches': matches, 'index': -1}
                                state = self._tab_state

                            matches = state.get('matches', [])
                            base = state.get('base', cur)

                            if not matches:
                                # no matches: swallow Tab and keep focus
                                event.accept()
                                return True

                            # advance or go back in cycle
                            if key == Qt.Key_Tab:
                                state['index'] = (state['index'] + 1) % len(matches)
                            else:
                                state['index'] = (state['index'] - 1) % len(matches)

                            match = matches[state['index']]
                            # apply match and select appended portion beyond the original base
                            lineedit.setText(match)
                            try:
                                lineedit.setSelection(len(base), max(0, len(match) - len(base)))
                            except Exception:
                                pass

                            # Don't force the completer to re-open here. Calling
                            # "complete()" can cause Qt to reset the edit text or
                            # popup selection and break our anchored tab cycle.

                            event.accept()
                            return True
                    except Exception:
                        logger.debug("Tab completion failed:\n" + traceback.format_exc())

                # apply completion when the lineedit loses focus
                if event.type() == QEvent.FocusOut:
                    self._apply_best_test_completion()
            # If completer popup receives key events, handle Tab there similar to the
            # lineedit so pressing Tab while popup is visible doesn't close it or move focus.
            try:
                comp = getattr(self, "_test_completer", None)
                popup = comp.popup() if comp is not None else None
                # If the completer popup is being shown, resize it so it will
                # display up to ~12 rows before showing a scrollbar. Do this on
                # the Show event so the current filtered rowCount is accurate.
                if popup is not None and obj is popup and event.type() in (QEvent.Show, QEvent.ShowToParent):
                    try:
                        desired = 12
                        rowh = popup.sizeHintForRow(0)
                        if not rowh or rowh <= 0:
                            fm = popup.fontMetrics()
                            rowh = fm.height() + 6
                        rows = max(1, popup.model().rowCount())
                        rows = min(desired, rows)
                        popup.setFixedHeight(rowh * rows + 2 * popup.frameWidth())
                    except Exception:
                        logger.debug("Could not set dynamic completer popup height on show:\n" + traceback.format_exc())
                if popup is not None and obj is popup and event.type() == QEvent.KeyPress:
                    try:
                        key = event.key()
                        # reset cycle state on printable non-tab keys only
                        try:
                            is_printable = bool(event.text())
                        except Exception:
                            is_printable = False
                        if is_printable and key not in (Qt.Key_Tab, Qt.Key_Backtab):
                            try:
                                self._tab_state = None
                            except Exception:
                                pass
                            try:
                                QTimer.singleShot(0, lambda: self._adjust_completer_popup_height())
                            except Exception:
                                pass

                        if key in (Qt.Key_Tab, Qt.Key_Backtab):
                            # Use existing tab state if present, otherwise initialize
                            text = lineedit.text() if lineedit is not None else ""
                            cur = text.strip()
                            state = getattr(self, '_tab_state', None)
                            if state is None:
                                base = cur
                                prefix_matches = [t for t in self.TestList if t.lower().startswith(base.lower())]
                                matches = prefix_matches if prefix_matches else [t for t in self.TestList if base.lower() in t.lower()]
                                self._tab_state = {'base': base, 'matches': matches, 'index': -1}
                                state = self._tab_state

                            matches = state.get('matches', [])
                            base = state.get('base', cur)

                            if not matches:
                                event.accept()
                                return True

                            if key == Qt.Key_Tab:
                                state['index'] = (state['index'] + 1) % len(matches)
                            else:
                                state['index'] = (state['index'] - 1) % len(matches)

                            match = state['matches'][state['index']]
                            if lineedit is not None:
                                lineedit.setText(match)
                                try:
                                    lineedit.setSelection(len(base), max(0, len(match) - len(base)))
                                except Exception:
                                    pass
                                # avoid calling comp.complete() here for the same
                                # reason as above
                            event.accept()
                            return True
                    except Exception:
                        logger.debug("Popup Tab handling failed:\n" + traceback.format_exc())
            except Exception:
                # ignore failures querying popup
                logger.debug("eventFilter failed:\n" + traceback.format_exc())

        except Exception:
            logger.debug("eventFilter caught unexpected error:\n" + traceback.format_exc())

        return super(QtStartWindow, self).eventFilter(obj, event)

    def radio_selected(self, replaceArgs:tuple):
        erroredFlag = False
        for moduleBox in self.BeBoardWidget.getModules():
            for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                item = self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0)
                if item is not None:
                    chiplineedit = item.widget()
                    if chiplineedit is not None:
                        chiplineedit.setText(chiplineedit.text().replace(*replaceArgs))
                        if "panthera.fit.edu" in chiplineedit.text():
                            try:
                                response = requests.head(chiplineedit.text(), allow_redirects=True)  # or .get() if you need content
                                if response.status_code in (404, 0, 400, 403):
                                    logger.info(f"Panthera file doesn't exist:  {chiplineedit.text()}")
                                    if not erroredFlag:
                                        self.master.errorMessageBoxSignal.emit("One or more of the chip txt pages don't exist!")
                                        erroredFlag=True
                            except requests.exceptions.RequestException:
                                logger.error(traceback.format_exc())
                                if not erroredFlag:
                                    self.master.errorMessageBoxSignal.emit("Could not access one or more of the chip txt pages!")
                                    erroredFlag=True


    def useCustomTxts(self, state:bool):
        if state:
            for moduleBox in self.BeBoardWidget.getModules():
                for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                    ChipTxtEdit = QLineEdit()
                    ChipTxtEdit.setPlaceholderText("Prebuilt chip .txt file")
                    self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].addWidget(ChipTxtEdit, 1, 0, 1, 7)
        else:
            self.txt_entry.setText("")
            for moduleBox in self.BeBoardWidget.getModules():
                for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                    item = self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0)
                    if item is not None:
                        widget = item.widget()
                        if widget is not None:
                            self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].removeWidget(widget)
                            widget.setParent(None) 
                            widget.deleteLater()  # Optional: safely schedule widget for deletion
        

    def change_chip_txts(self, url:str):
        links = {}
        if url != "":
            if not self.customTxtCheck.isChecked():
                self.customTxtCheck.setChecked(True)
                moduleBox = self.BeBoardWidget.getModules()[0]
                chipid = tuple(self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys())[0]
                if self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0) is None:
                    self.useCustomTxts(True)

            erroredFlag=False

            if len(url)< 8 or ("https://"!=url[:8] and "http://"!=url[:7]):
                url = "https://"+url
                self.txt_entry.setText(url)

            pantheraURL=False
            idx = url.find("ResultID")
            if "panthera.fit.edu" in url and idx != -1:
                i = idx+len("ResultID")
                while i < len(url) and url[i].isdigit():
                    i += 1
                url = url[:i+1]
                if url[-1]!='/':
                    url = url+'/'
                self.txt_entry.setText(url)
                pantheraURL=True

            outOrIn = 'OUT' if self.out_radio.isChecked() else 'IN'
            for moduleBox in self.BeBoardWidget.getModules():
                
                moduleName = moduleBox.getSerialNumber()
                port = moduleBox.getFMCPort()
                if moduleName == "" or port == "":
                    self.master.errorMessageBoxSignal.emit("Please enter a Serial Number and FMC Port to autofill .txt files.")
                    return

                for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                    if pantheraURL:
                        fileLink = re.sub(r'_+', '_', url+"CMSIT_RD53_{0}_{1}_{2}_{3}.txt".format(
                                moduleName, port, chipid, outOrIn
                            )) #The re function replaces any instance of multiple underscores with just one.
                    else:
                        fileLink = url
                    
                    #Check if the Panthera file actually exists.
                    try:
                        response = requests.head(fileLink, allow_redirects=True)  # or .get() if you need content
                        if response.status_code in (404, 0, 400, 403):
                            logger.info(f"Panthera file doesn't exist:  {fileLink}")
                            if not erroredFlag:
                                self.master.errorMessageBoxSignal.emit("One or more of the chip txt pages don't exist!")
                                erroredFlag=True
                    except requests.exceptions.RequestException:
                        logger.error(traceback.format_exc())
                        if not erroredFlag:
                            self.master.errorMessageBoxSignal.emit("Could not access one or more of the chip txt pages!")
                            erroredFlag=True

                    links[moduleName, moduleBox.getFMCPort(), chipid] = fileLink
        
        #Do this at the end so that there's no autofill unless all files pass the check above.
        for moduleBox in self.BeBoardWidget.getModules():
            moduleName = moduleBox.getSerialNumber()
            for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                item = self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0)
                if item is not None:
                    chiplineedit = item.widget()
                    if chiplineedit is not None:
                        chiplineedit.setText(links[moduleName, moduleBox.getFMCPort(), chipid])

    def createApp(self):
        self.AppOption = QGroupBox()
        self.StartLayout = QHBoxLayout()

        self.CancelButton = QPushButton("&Cancel")
        self.CancelButton.clicked.connect(self.release)
        self.CancelButton.clicked.connect(self.closeWindow)

        self.ResetButton = QPushButton("&Reset")
        self.ResetButton.clicked.connect(self.createMain)

        self.CheckButton = QPushButton("&Check")
        self.CheckButton.clicked.connect(self.checkFwPar)

        self.NextButton = QPushButton("&Next")
        self.NextButton.setDefault(True)
        self.NextButton.clicked.connect(self.openRunWindow_starter)

        self.StartLayout.addStretch(1)
        self.StartLayout.addWidget(self.CancelButton)
        # self.StartLayout.addWidget(self.ResetButton)
        # self.StartLayout.addWidget(self.CheckButton)
        self.StartLayout.addWidget(self.NextButton)
        self.AppOption.setLayout(self.StartLayout)

        self.LogoGroupBox = QGroupBox("")
        self.LogoGroupBox.setCheckable(False)
        self.LogoGroupBox.setMaximumHeight(100)

        self.LogoLayout = QHBoxLayout()
        OSULogoLabel = QLabel()
        OSUimage = QImage("icons/osuicon.jpg").scaled(
            QSize(200, 60), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        OSUpixmap = QPixmap.fromImage(OSUimage)
        OSULogoLabel.setPixmap(OSUpixmap)
        CMSLogoLabel = QLabel()
        CMSimage = QImage("icons/cmsicon.png").scaled(
            QSize(200, 60), Qt.KeepAspectRatio, Qt.SmoothTransformation
        )
        CMSpixmap = QPixmap.fromImage(CMSimage)
        CMSLogoLabel.setPixmap(CMSpixmap)
        self.LogoLayout.addWidget(OSULogoLabel)
        self.LogoLayout.addStretch(1)
        self.LogoLayout.addWidget(CMSLogoLabel)

        self.LogoGroupBox.setLayout(self.LogoLayout)

        self.mainLayout.addWidget(self.AppOption, 3, 0, 1, 2)
        self.mainLayout.addWidget(self.LogoGroupBox, 4, 0, 1, 2)

    def closeWindow(self):
        self.close()

    def occupied(self):
        self.master.ProcessingTest = True

    def release(self):
        self.master.ProcessingTest = False
        self.master.NewTestButton.setDisabled(False)
        self.master.LogoutButton.setDisabled(False)
        self.master.ExitButton.setDisabled(False)

    def checkFwPar(self, pfirmwareName):
        GlobalCheck = True
        for item in self.ModuleList:
            # item.checkFwPar(pfirmwareName, item.module.getType())
            GlobalCheck = GlobalCheck and item.checkFwPar(
                pfirmwareName, item.module.getType(), FC7List[pfirmwareName]
            )
            # GlobalCheck = GlobalCheck and item.getResult()
        self.passCheck = GlobalCheck
        return GlobalCheck

    def setupBeBoard(self):
        # Setup the BeBoard
        pass

    def loader(self):
        self.NextButton.setText(". " * (self.loading_counter + 1))
        self.loading_counter = (self.loading_counter + 1) % 3

    def onThreadFinish(self):
        if self.closeFlag:
            self.close()
        else:
            self.NextButton.setText("&Next")
            self.NextButton.setDisabled(False)

    def openRunWindow_starter(self):
        self.NextButton.setDisabled(True)
        self.NextButton.setText(". . .")
        # Only show the waiting popup if coldbox is present
        if cooler == "Tessie":
            self.waiting_popup = QMessageBox(self)
            self.waiting_popup.setWindowTitle("Waiting for Coldbox")
            self.waiting_popup.setText("Waiting for Coldbox to reach target temperature...")
            self.waiting_popup.setStandardButtons(QMessageBox.NoButton)
            self.waiting_popup.setModal(True)
            self.waiting_popup.show()
        else:
            self.waiting_popup = None
        # Start the thread to open the Run Window
        self.run_window_thread = LoadingThread(self.openRunWindow, 500)
        self.run_window_thread.finished.connect(self.onThreadFinishSignal)
        self.run_window_thread.timer.timeout.connect(self.loaderSignal)
        self.run_window_thread.timer.start()
        self.run_window_thread.start()

    def openRunWindow(self):
        # if not os.access(os.environ.get('GUI_dir'),os.W_OK):
        # 	QMessageBox.warning(None, "Error",'write access to GUI_dir is {0}'.format(os.access(os.environ.get('GUI_dir'),os.W_OK)), QMessageBox.Ok)
        # 	return
        # if not os.access("{0}/test".format(os.environ.get('PH2ACF_BASE_DIR')),os.W_OK):
        # 	QMessageBox.warning(None, "Error",'write access to Ph2_ACF is {0}'.format(os.access(os.environ.get('PH2ACF_BASE_DIR'),os.W_OK)), QMessageBox.Ok)
        # 	return
        
        files = {} #Maybe this block could be combined with change_chip_txts.
        if self.customTxtCheck.isChecked():
            for moduleBox in self.BeBoardWidget.getModules():
                moduleName = moduleBox.getSerialNumber()
                for chipid in self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict.keys():
                    text = self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0).widget().text().replace(" ", "")
                    if text != "":
                        if 'panthera.fit.edu' in text:
                            try:
                                txt_index = text.rfind(".txt")
                                destination = os.environ.get("PH2ACF_BASE_DIR") + "/test/" + text[text.rfind("/", 0, txt_index)+1:txt_index]
                                os.system(f"wget -O {destination} {text}")
                                self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0).widget().setText(destination)
                            except Exception:
                                logger.error(traceback.format_exc())
                                self.master.errorMessageBoxSignal.emit(f"Could not Chip{chipid} file from Panthera!")
                                return
                        elif not os.path.exists(text):
                            self.master.errorMessageBoxSignal.emit(f"Chip{chipid}'s .txt file doesn't exist!")
                            return
                        files[moduleName, moduleBox.getFMCPort(), chipid] = self.BeBoardWidget.ChipWidgetDict[moduleBox].ChipGroupBoxDict[chipid].itemAtPosition(1,0).widget().text()

        # NOTE This is not the best way to do this, we should be emitting a signal to change
        # the module type but ModuleBox is not publically accessible so we have to go through BeBoardWidget
        self.master.module_in_use = self.BeBoardWidget.getModules()[0].getType()

        self.update_instrument_cluster()


        for module in self.BeBoardWidget.getModules():
            if module.getSerialNumber() == "":
                self.master.errorMessageBoxSignal.emit(
                    "No valid serial number!",
                )  # Needs to be in a signal or QThread throws an error
                return
            if module.getFMCPort() == "":
                self.master.errorMessageBoxSignal.emit("No valid ID!")
                return

        self.firmwareDescription, message = self.BeBoardWidget.getFirmwareDescription()

        if (
            not self.firmwareDescription
        ):  # firmware description returns none if no modules are entered
            self.master.errorMessageBoxSignal.emit(message)
            return

        for fw in self.firmwareDescription:
            self.checkFwPar(fw.getBoardName())
        if not self.passCheck:
            reply = QMessageBox().question(  # For some reason this isn't an issue for QThread
                None,
                "Error",
                "Front-End parameter check failed, forced to continue?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No,
            )
            if reply == QMessageBox.No:
                return

        for beboard in self.firmwareDescription:
            print(beboard)

        self.info = self.TestCombo.currentText()

        self.runFlag = True
        self.master.BeBoardWidget = self.BeBoardWidget
        self.set_default_temperature()
                # Close the waiting popup if it exists
        if hasattr(self, 'waiting_popup') and self.waiting_popup:
            self.waiting_popup.done(0)
            self.waiting_popup = None

        self.master.openRunWindowSignal.emit(self.info, self.firmwareDescription, files)
        self.closeFlag = True

    def update_instrument_cluster(self):
        if hasattr(self.master, 'instruments') and 'auto' in json_setup:
            logger.info("Automatically setting instrument cluster channels.")
            self._update_module_dict(UsedChannels=self._get_used_channels())

    def _get_used_channels(self):
        UsedChannels = WorkingChannels[:len(self.BeBoardWidget.getModules())]
        logger.info(f"Working channels being used: {UsedChannels}")
        return UsedChannels   

    def _update_module_dict(self, UsedChannels):
        keys_to_remove = [key for key in self.master.instruments._module_dict.keys()]
        for key in UsedChannels:
            if str(key-1) in keys_to_remove:
                keys_to_remove.remove(str(key-1))
        for key in keys_to_remove:
            self.master.instruments._module_dict.pop(key)
        logger.info(f"Module Dict:",self.master.instruments.get_modules())
        logger.info(f"Instruments:",self.master.instruments.get_instruments())

    def set_default_temperature(self):
        """Set the temperature of every active TEC to the default temperature if 'Tessie' is chosen as the cooler."""
        if cooler == "Tessie":
            try:
                default_temperature = icicle_instrument_setup["instrument_dict"]["cb"]["default_temperature"]
                self.master.instruments.cb_on(temperature = default_temperature)
            except KeyError:
                logger.error("Default temperature or coldbox configuration is missing in the instrument setup.")
            except Exception:
                logger.error(traceback.format_exc())

    def closeEvent(self, event):
        if self.runFlag:
            event.accept()

        else:
            reply = QMessageBox.question(
                self,
                "Window Close",
                "Are you sure you want to quit the test?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No,
            )

            if reply == QMessageBox.Yes:
                event.accept()
                self.release()
                # This line was previosly commented
                try:
                    if self.master.instruments:
                        self.master.instruments.off(hv_delay=0.5, hv_step_size=10)

                        print("Window closed")
                    else:
                        logger.info(
                            " You are running in manual mode."
                            " You must turn off powers supplies yourself."
                        )
                except Exception:
                    print(
                        "Waring: Incident detected while trying to turn of power supply, please check power status"
                    )
                    logger.error(traceback.format_exc())
            else:
                event.ignore()
