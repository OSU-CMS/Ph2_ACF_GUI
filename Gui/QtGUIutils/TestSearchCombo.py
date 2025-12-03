import traceback
from PyQt5.QtCore import Qt, QSortFilterProxyModel, QStringListModel, QEvent, QTimer, QObject
from PyQt5.QtWidgets import QCompleter


def _setup_combo_basic_properties(combo, desired_visible):
    """Set up basic properties for the combo box."""
    combo.setEditable(True)
    combo.setEditText("")
    
    # Set focus policy to ensure tab events are received
    combo.setFocusPolicy(Qt.StrongFocus)
    le = combo.lineEdit()
    if le is not None:
        le.setFocusPolicy(Qt.StrongFocus)
    
    try:
        combo.setMaxVisibleItems(desired_visible)
    except Exception:
        pass


def _safe_log(parent, method_name, exception):
    """Safely log an exception if parent has a logger."""
    try:
        if parent and hasattr(parent, 'logger') and parent.logger:
            parent.logger.debug(f"{method_name} failed:\n{exception}")
    except Exception:
        pass


class FuzzyFilterProxy(QSortFilterProxyModel):
    """A proxy model that provides fuzzy filtering for combo box items."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self._filter = ""

    def setFilterString(self, filter_text: str):
        """Set the filter string and invalidate the current filter."""
        self._filter = filter_text or ""
        self.invalidateFilter()

    def is_subsequence(self, needle: str, haystack: str) -> bool:
        """Check if needle is a subsequence of haystack (case insensitive)."""
        if not needle:
            return True
        
        haystack_iter = iter(haystack.lower())
        for char in needle.lower():
            if not any(h == char for h in haystack_iter):
                return False
        return True

    def filterAcceptsRow(self, source_row, source_parent):
        """Determine if a row should be accepted by the filter."""
        try:
            if not self._filter:
                return True
                
            idx = self.sourceModel().index(source_row, 0, source_parent)
            text = str(self.sourceModel().data(idx, Qt.DisplayRole) or "")
            
            # Try subsequence match first, then substring match
            return (self.is_subsequence(self._filter, text) or 
                    self._filter.lower() in text.lower())
        except Exception:
            return False


def _setup_completer(proxy, combo, parent):
    """Set up and configure the QCompleter with proper settings."""
    completer = QCompleter(proxy, parent)
    
    # Set case insensitive matching
    try:
        completer.setCaseSensitivity(Qt.CaseInsensitive)
    except Exception:
        pass
    
    # Set completion mode - prefer UnfilteredPopupCompletion
    try:
        completer.setCompletionMode(QCompleter.UnfilteredPopupCompletion)
    except Exception:
        try:
            completer.setCompletionMode(QCompleter.PopupCompletion)
        except Exception:
            pass

    # Attach completer to combo but detach widget to prevent auto-completion
    try:
        combo.setCompleter(completer)
        # Detach widget to prevent automatic inline completion while typing
        le = combo.lineEdit()
        if le is not None:
            completer.setWidget(None)
    except Exception:
        pass
    
    return completer


class TestComboController(QObject):
    """Controller that handles tab cycling and other interactive behavior for the combo box."""
    
    def __init__(self, combo, test_list, completer, proxy, parent=None, desired_visible=12):
        super().__init__(parent)
        self.combo = combo
        self.test_list = list(test_list)
        self.completer = completer
        self.proxy = proxy
        self.parent = parent
        self.desired_visible = desired_visible
        self._tab_state = None

    def apply_best_test_completion(self):
        try:
            text = self.combo.currentText().strip()
            if not text:
                return
            idx = self.combo.findText(text, Qt.MatchExactly)
            if idx != -1:
                self.combo.setCurrentIndex(idx)
                return
            lower = text.lower()
            prefix_matches = [t for t in self.test_list if t.lower().startswith(lower)]
            if prefix_matches:
                match = prefix_matches[0]
            else:
                substr_matches = [t for t in self.test_list if lower in t.lower()]
                match = substr_matches[0] if substr_matches else None
            if match:
                self.combo.setEditText(match)
                idx = self.combo.findText(match, Qt.MatchExactly)
                if idx != -1:
                    self.combo.setCurrentIndex(idx)
        except Exception as e:
            _safe_log(self.parent, "apply_best_test_completion", traceback.format_exc())

    def on_completer_activated(self, text: str):
        try:
            if not text:
                return
            self.combo.setEditText(text)
            idx = self.combo.findText(text, Qt.MatchExactly)
            if idx != -1:
                self.combo.setCurrentIndex(idx)
        except Exception as e:
            _safe_log(self.parent, "on_completer_activated", traceback.format_exc())

    def on_lineedit_return_pressed(self):
        try:
            comp = self.completer
            if comp is not None:
                popup = None
                try:
                    popup = comp.popup()
                except Exception:
                    popup = None
                current = None
                try:
                    current = comp.currentCompletion()
                except Exception:
                    try:
                        if popup is not None:
                            idx = popup.currentIndex()
                            current = idx.data() if idx.isValid() else None
                    except Exception:
                        current = None
                if current:
                    self.combo.setEditText(current)
                    idx = self.combo.findText(current, Qt.MatchExactly)
                    if idx != -1:
                        self.combo.setCurrentIndex(idx)
                    return
            self.apply_best_test_completion()
        except Exception as e:
            _safe_log(self.parent, "on_lineedit_return_pressed", traceback.format_exc())

    def adjust_completer_popup_height(self, desired: int = None):
        try:
            if desired is None:
                desired = self.desired_visible
            comp = self.completer
            if comp is None:
                return
            popup = comp.popup()
            if popup is None:
                return
            model = popup.model()
            if model is None:
                return
            rows = model.rowCount()
            if rows <= 0:
                rows = 1
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
        except Exception as e:
            _safe_log(self.parent, "adjust_completer_popup_height", traceback.format_exc())

    def _get_popup_matches(self):
        out = []
        try:
            popup = self.completer.popup()
            if popup is None:
                return out
            model = popup.model()
            if model is None:
                return out
            for r in range(model.rowCount()):
                idx = model.index(r, 0)
                val = idx.data() if idx.isValid() else None
                if val:
                    out.append(str(val))
        except Exception:
            pass
        return out

    def eventFilter(self, obj, event):
        try:
            le = self.combo.lineEdit()
            if le is not None and obj is le:
                if event.type() == QEvent.KeyPress:
                    key = event.key()
                   
                    if key == Qt.Key_Tab or key == Qt.Key_Backtab:
                        # Immediately prevent default tab behavior
                        event.accept()
                        
                        # Get current text
                        current_text = le.text() or ""
                        
                        # Check if we're continuing an existing tab cycle or starting a new one
                        if (self._tab_state is not None and 
                            self._tab_state.get("base") is not None and
                            current_text in self._tab_state.get("matches", [])):
                            # We're continuing a tab cycle - use the original search base
                            base = self._tab_state["base"]
                            matches = self._tab_state["matches"]
                        else:
                            # Starting a new tab cycle - use current text as base
                            base = current_text
                            self.proxy.setFilterString(base)
                            
                            # Build match list from proxy model (filtered)
                            matches = []
                            for r in range(self.proxy.rowCount()):
                                idx = self.proxy.index(r, 0)
                                if idx.isValid():
                                    val = self.proxy.data(idx)
                                    if val:
                                        matches.append(str(val))
                            
                            # Initialize new tab cycle state
                            self._tab_state = {"base": base, "matches": matches, "index": -1}

                        # Show popup
                        try:
                            self.completer.setWidget(le)
                            self.completer.complete()
                            QTimer.singleShot(0, lambda: (
                                self.adjust_completer_popup_height(self.desired_visible), 
                                le.setFocus()
                            ))
                        except Exception:
                            pass

                        # Cycle through matches
                        if self._tab_state and self._tab_state.get("matches"):
                            if key == Qt.Key_Backtab:
                                self._tab_state["index"] = (self._tab_state["index"] - 1) % len(self._tab_state["matches"])
                            else:
                                self._tab_state["index"] = (self._tab_state["index"] + 1) % len(self._tab_state["matches"])
                            
                            sel = self._tab_state["matches"][self._tab_state["index"]]
                            self.combo.setEditText(sel)
                                                  
                        return True  # Event handled
                    
                    # Handle other printable characters
                    try:
                        txt = event.text()
                        if txt and txt.isprintable():
                            # Reset tab state when user types
                            self._tab_state = None
                            self.completer.setWidget(None)
                            QTimer.singleShot(0, lambda: self.adjust_completer_popup_height(self.desired_visible))
                    except Exception:
                        pass
                        
                elif event.type() == QEvent.FocusOut:
                    try:
                        self.apply_best_test_completion()
                    except Exception:
                        pass
                        
            # Handle popup events
            try:
                popup = self.completer.popup()
                if popup is not None and obj is popup:
                    if event.type() in (QEvent.Show, QEvent.ShowToParent):
                        QTimer.singleShot(0, lambda: self.adjust_completer_popup_height(self.desired_visible))
                    elif event.type() == QEvent.KeyPress:
                        key = event.key()
                        if key in (Qt.Key_Tab, Qt.Key_Backtab):
                            # Forward tab events from popup to line edit
                            return self.eventFilter(le, event)
            except Exception:
                pass
                
        except Exception as e:
            _safe_log(self.parent, "TestComboController.eventFilter", traceback.format_exc())
        
        return False  # Let other events pass through


def _connect_signals_and_filters(combo, completer, proxy, controller, parent):
    """Connect all signals and install event filters for the combo box."""
    le = combo.lineEdit()
    
    # Connect line edit signals
    if le is not None:
        try:
            le.textEdited.connect(lambda s: proxy.setFilterString(s))
            le.editingFinished.connect(controller.apply_best_test_completion)
            le.returnPressed.connect(controller.on_lineedit_return_pressed)
        except Exception:
            pass

    # Connect completer activated signal
    try:
        completer.activated[str].connect(controller.on_completer_activated)
    except Exception:
        try:
            completer.activated.connect(controller.on_completer_activated)
        except Exception:
            pass

    # Install event filters - this is critical for tab cycling to work
    if le is not None:
        try:
            le.installEventFilter(controller)
            if hasattr(parent, 'logger') and parent.logger:
                parent.logger.debug("Successfully installed event filter on line edit")
        except Exception as e:
            _safe_log(parent, "_connect_signals_and_filters", f"Failed to install lineedit eventFilter: {e}")

    try:
        popup = completer.popup()
        if popup is not None:
            popup.installEventFilter(controller)
    except Exception as e:
        _safe_log(parent, "_connect_signals_and_filters", f"Failed to install popup eventFilter: {e}")


def configure_test_combo(combo, test_list, parent=None, desired_visible=16):
    """Configure a QComboBox with fuzzy search and tab cycling functionality.
    
    Args:
        combo: QComboBox to configure
        test_list: List of test names for the dropdown
        parent: Parent widget for logging
        desired_visible: Maximum visible items in dropdown
        
    Returns:
        tuple: (completer, proxy, controller) or (None, None, None) on failure
    """
    try:
        _setup_combo_basic_properties(combo, desired_visible)
        le = combo.lineEdit()
        if le is not None:
            le.setPlaceholderText("Select a test...")

        # Set up the filtering proxy model and completer
        base_model = QStringListModel(test_list, parent)
        proxy = FuzzyFilterProxy(parent)
        proxy.setSourceModel(base_model)
        
        completer = _setup_completer(proxy, combo, parent)

        # Create controller instance
        controller = TestComboController(combo, test_list, completer, proxy, parent, desired_visible)
        _connect_signals_and_filters(combo, completer, proxy, controller, parent)

        return completer, proxy, controller
    except Exception as e:
        _safe_log(parent, "configure_test_combo", traceback.format_exc())
        return None, None, None
