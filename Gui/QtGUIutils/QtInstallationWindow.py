"""TFPX Module Installation Status -- Dee View.

Opened from the main window's "Installation GUI" button. Clicking a module
box opens a dialog to enter/edit the module at that position and start the
regular QtStartWindow prefilled for it. Data lives in
Gui.python.InstallationModel. Put osu_logo.jpg / cms_logo.png next to this
file (optional; the header falls back to text if they're missing).
"""
import html
import os
import re

from PyQt5.QtCore import Qt, QPointF, QRectF, QTimer, QUrl
from PyQt5.QtGui import (QColor, QDesktopServices, QFont, QPainter,
                         QPainterPath, QPen, QPixmap, QPolygonF, QTransform)
from PyQt5.QtWidgets import (QApplication, QCheckBox, QComboBox, QDialog,
                             QFormLayout, QFrame, QHBoxLayout, QLabel,
                             QLayout, QLineEdit, QMainWindow, QMessageBox,
                             QPushButton, QScrollArea, QToolTip, QVBoxLayout,
                             QWidget)

from Gui.GUIutils.settings import ModuleType

from Gui.python.InstallationModel import (A0, A1, CX, CY, DEE_LABEL, DEES,
                                          N_CARTRIDGES, SIDE_LABEL, SIDES,
                                          InstallationModel, module_id, polar)
from Gui.python.logging_config import get_logger
from Gui.python.PantheraHistory import REVIEW_PAGE, fetch_history_async

logger = get_logger(__name__)

HERE = os.path.dirname(os.path.abspath(__file__))

# ============================================================ CONFIG
COLORS = {"untested": "#e8d22a", "pass": "#33c14a", "fail": "#e23b3b"}
UNASSIGNED = "#b4b9c0"  # position with no serial assigned
CF, CF_EDGE = "#848d99", "#636b76"
VBW, VBH = 122, 220
RIN, ROUT = 20, 104
DEE_W, DEE_H = 156, 281

VTRX_LPGBT = {"v1": "0x70", "v2": "0x71", "v3": "0x72"}


# ============================================================ SCALING
# Everything below is laid out at a 1x base size (designed for ~1080p) and
# scaled up at runtime:
#   k  -- map scale: Dee/portcard drawings grow to fill the window.
#   kf -- UI scale (fonts, padding, margins): follows the screen resolution,
#         so text is the same physical size on a 1080p or 4K screen.
_PX = re.compile(r"(\d+(?:\.\d+)?)px")


def px(n, k):
    return max(1, int(round(n * k)))


def sq(text, k):
    """Scale every 'Npx' value in a style sheet / HTML snippet by k."""
    return _PX.sub(lambda mo: "%dpx" % px(float(mo.group(1)), k), text)


def screen_ui_scale(screen):
    """UI scale for a screen: 1.0 at <=1080p, ~2.0 at 4K."""
    if screen is None:
        return 1.0
    return min(4.0, max(1.0, screen.availableGeometry().height() / 1000.0))


def scale_tree(root, kf):
    """Scale style sheets, fixed sizes and layout margins/spacing of root and
    all its children by kf (Dee/portcard drawings scale themselves)."""
    if abs(kf - 1.0) < 0.01:
        return
    for w in [root] + root.findChildren(QWidget):
        if isinstance(w, (DeeWidget, PortcardWidget)):
            continue
        if w.styleSheet():
            w.setStyleSheet(sq(w.styleSheet(), kf))
        if w.minimumWidth() == w.maximumWidth() and w.minimumWidth() > 0:
            w.setFixedWidth(px(w.minimumWidth(), kf))
        if w.minimumHeight() == w.maximumHeight() and w.minimumHeight() > 0:
            w.setFixedHeight(px(w.minimumHeight(), kf))
    for lay in root.findChildren(QLayout):
        m = lay.contentsMargins()
        lay.setContentsMargins(*(int(round(v * kf)) for v in (m.left(), m.top(), m.right(), m.bottom())))
        if lay.spacing() > 0:
            lay.setSpacing(px(lay.spacing(), kf))


def _screen_of(widget):
    try:
        handle = widget.window().windowHandle()
        if handle is not None and handle.screen() is not None:
            return handle.screen()
    except Exception:
        pass
    return QApplication.primaryScreen()


def _fit_dialog(dlg, w, h):
    """Resize a dialog to (w, h) but never beyond 95% of its screen."""
    avail = _screen_of(dlg.parentWidget() or dlg).availableGeometry()
    dlg.resize(min(w, int(avail.width() * 0.95)), min(h, int(avail.height() * 0.95)))


# ============================================================ DEE WIDGET
class DeeWidget(QWidget):
    def __init__(self, mods, mode, on_click, k=1.0, parent=None):
        super().__init__(parent)
        self.mods, self.mode, self.on_click = mods, mode, on_click
        self.hover = None
        self.setFixedSize(px(DEE_W, k), px(DEE_H, k))
        self.scale = self.width() / VBW
        self.setMouseTracking(True)
        self.polys = []
        for m in mods:  # hit polygons in widget coords
            t = QTransform()
            t.scale(self.scale, self.scale)
            t.translate(m["cx"], m["cy"])
            t.rotate(m["rot"])
            self.polys.append(t.map(QPolygonF(QRectF(-m["w"] / 2, -m["h"] / 2, m["w"], m["h"]))))

    def _st(self, m):
        return m["status"] if self.mode == "cur" else m["prev"]

    def _clickable(self, m):
        # Current view: any box (empty ones can be assigned a module).
        # Previous view: display only.
        return self.mode == "cur"

    def _hit(self, pos):
        for i, p in enumerate(self.polys):
            if p.containsPoint(QPointF(pos), Qt.OddEvenFill):
                return i
        return None

    def mouseMoveEvent(self, e):
        i = self._hit(e.pos())
        if i != self.hover:
            self.hover = i
            clickable = i is not None and self._clickable(self.mods[i])
            self.setCursor(Qt.PointingHandCursor if clickable else Qt.ArrowCursor)
            self.update()
        if i is not None:
            m = self.mods[i]
            when = "Current" if self.mode == "cur" else "Before latest test"
            state = self._st(m).upper() if m["serial"] else "NO MODULE ASSIGNED (click to assign)"
            QToolTip.showText(e.globalPos(), "%s · Cartridge %d Dee %s · %s · Ring %d Module %d · %s: %s" % (
                m["serial"] or "—", m["cartridge"], m["dee"], SIDE_LABEL[m["side"]], m["ring"], m["slot"],
                when, state), self)
        else:
            QToolTip.hideText()

    def leaveEvent(self, _):
        self.hover = None
        self.update()

    def mousePressEvent(self, e):
        if e.button() == Qt.LeftButton:
            i = self._hit(e.pos())
            if i is not None and self._clickable(self.mods[i]):
                self.on_click(self.mods[i], self._st(self.mods[i]), self.mode)

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHints(QPainter.Antialiasing | QPainter.TextAntialiasing)
        p.scale(self.scale, self.scale)
        # Dee body
        ix1, iy1 = polar(RIN, A0)
        bx1, by1 = polar(ROUT, A0)
        ix2, iy2 = polar(RIN, A1)
        path = QPainterPath(QPointF(ix1, iy1))
        path.lineTo(bx1, by1)
        path.arcTo(QRectF(CX - ROUT, CY - ROUT, 2 * ROUT, 2 * ROUT), A0, A1 - A0)
        path.lineTo(ix2, iy2)
        path.arcTo(QRectF(CX - RIN, CY - RIN, 2 * RIN, 2 * RIN), A1, -(A1 - A0))
        path.closeSubpath()
        p.setPen(QPen(QColor(CF_EDGE), 1.5))
        p.setBrush(QColor(CF))
        p.drawPath(path)
        # modules
        font = QFont("Helvetica")
        font.setBold(True)
        font.setPixelSize(9)
        for i, m in enumerate(self.mods):
            col = QColor(COLORS[self._st(m)] if m["serial"] else UNASSIGNED)
            if i == self.hover:
                col = col.lighter(110)
            p.save()
            p.translate(m["cx"], m["cy"])
            p.rotate(m["rot"])
            p.setPen(QPen(QColor("#20242b"), 1.4 if i == self.hover else 0.6))
            p.setBrush(col)
            p.drawRoundedRect(QRectF(-m["w"] / 2, -m["h"] / 2, m["w"], m["h"]), 1.3, 1.3)
            p.restore()
            p.setFont(font)
            p.setPen(QColor("#1a0d0d"))
            p.drawText(QRectF(m["cx"] - 10, m["cy"] - 6, 20, 12), Qt.AlignCenter, str(m["slot"]))


# ============================================================ PORTCARD
class PortcardWidget(QWidget):
    def __init__(self, cart, pc, k=1.0, parent=None):
        super().__init__(parent)
        self.k = k
        self.setFixedSize(px(40, k), px(40, k))
        self.setToolTip("Cartridge %d · PC%d · VTRx1→%s · VTRx2→%s · VTRx3→%s" % (
            cart, pc, VTRX_LPGBT["v1"], VTRX_LPGBT["v2"], VTRX_LPGBT["v3"]))

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        p.scale(self.width() / 40.0, self.height() / 40.0)
        p.setPen(QPen(QColor("#8a1f1f"), 1.5))
        p.setBrush(QColor(CF))
        p.drawRoundedRect(QRectF(0.75, 0.75, 38.5, 38.5), 4, 4)
        pad, gap = 1.5 + 4, 2
        cell = (40 - 2 * pad - 2 * gap) / 3
        p.setPen(Qt.NoPen)
        p.setBrush(QColor(COLORS["untested"]))
        for col, row in ((0, 0), (2, 1), (0, 2)):  # v3, v2, v1
            p.drawRect(QRectF(pad + col * (cell + gap), pad + row * (cell + gap), cell, cell))


# ============================================================ DIALOGS
def logo_label(fname, height):
    lab = QLabel()
    pm = QPixmap(os.path.join(HERE, fname))
    if pm.isNull():
        lab.setText("OSU" if "osu" in fname else "CMS")
        lab.setStyleSheet("font-weight:bold")
    else:
        lab.setPixmap(pm.scaledToHeight(height, Qt.SmoothTransformation))
    return lab


MODULE_QSS = """
QWidget {background:#fff;color:#1c1c1c;font-size:13px}
QLabel, QCheckBox {color:#1c1c1c;background:transparent}
QLineEdit, QComboBox {background:#fff;color:#1c1c1c;border:1px solid #bbb;border-radius:3px;padding:3px 6px}
QComboBox QAbstractItemView {background:#fff;color:#1c1c1c}
QPushButton {background:#e9e9e9;color:#1c1c1c;border:1px solid #bbb;border-radius:4px;padding:6px 12px}
QPushButton:disabled {color:#9a9a9a}
QPushButton#start {background:#1f7a1f;color:#fff;border:none}
QPushButton#start:disabled {background:#a7c9a7}
"""
AUTO_TYPE = "(auto from DB)"


class ModuleDialog(QDialog):
    """Enter/edit the module at one Dee position and optionally start a test."""

    def __init__(self, m, window):
        super().__init__(window)
        self.m, self.inst = m, window
        self.mid = module_id(m)
        self.setWindowTitle("Module position %s" % self.mid)
        self.setStyleSheet(MODULE_QSS)
        self.kf = getattr(window, "kf", 1.0)
        v = QVBoxLayout(self)

        pos = QLabel("<b>Cartridge %d · %s · %s · Ring %d · Module %d</b>" % (
            m["cartridge"], DEE_LABEL[m["dee"]], SIDE_LABEL[m["side"]], m["ring"], m["slot"]))
        v.addWidget(pos)
        st = m["status"] if m["serial"] else "unassigned"
        col = COLORS.get(st, UNASSIGNED)
        status = QLabel("Status: <span style='background:%s;color:#1c1c1c'>&nbsp;%s&nbsp;</span>"
                        % (col, st.upper()))
        v.addWidget(status)

        form = QFormLayout()
        self.serial = QLineEdit(m["serial"] or "")
        self.serial.setPlaceholderText("e.g. RH0123")
        self.port = QLineEdit("" if m["fmc_port"] is None else str(m["fmc_port"]))
        self.port.setPlaceholderText("e.g. 0")
        self.type = QComboBox()
        self.type.addItems([AUTO_TYPE] + list(ModuleType.values()))
        if m["type"]:
            self.type.setCurrentText(m["type"])
        form.addRow("Serial number:", self.serial)
        form.addRow("FMC port:", self.port)
        form.addRow("Type:", self.type)
        v.addLayout(form)
        note = QLabel("Chip settings, test choice and anything else are set in the test window "
                      "that opens next.")
        note.setWordWrap(True)
        note.setStyleSheet("color:#6b7280;font-size:11px")
        v.addWidget(note)

        btns = QHBoxLayout()
        self.history_btn = QPushButton("Panthera history")
        self.history_btn.clicked.connect(self._history)
        btns.addWidget(self.history_btn)
        self.clear_btn = QPushButton("Remove module")
        self.clear_btn.setEnabled(bool(m["serial"]))
        self.clear_btn.clicked.connect(self._clear)
        btns.addWidget(self.clear_btn)
        btns.addStretch()
        cancel = QPushButton("Cancel")
        cancel.clicked.connect(self.reject)
        save = QPushButton("Save")
        save.clicked.connect(self._save_only)
        self.start_btn = QPushButton("Save && Start Test")
        self.start_btn.setObjectName("start")
        self.start_btn.setDefault(True)
        self.start_btn.clicked.connect(self._save_and_start)
        for b in (cancel, save, self.start_btn):
            btns.addWidget(b)
        v.addLayout(btns)

        self.serial.textChanged.connect(self._update_buttons)
        self._update_buttons()
        scale_tree(self, self.kf)
        self.setMinimumWidth(px(640, self.kf))

    def _panthera_login(self):
        master = self.inst.master
        if not getattr(master, "panthera_connected", False):
            return None
        user, pw = getattr(master, "username", None), getattr(master, "password", None)
        return (user, pw) if user and pw else None

    def _update_buttons(self):
        has_serial = bool(self.serial.text().strip())
        self.start_btn.setEnabled(has_serial)
        logged_in = self._panthera_login() is not None
        self.history_btn.setEnabled(has_serial and logged_in)
        self.history_btn.setToolTip(
            "" if logged_in else "Log in to the GUI with your Panthera account to see history.")

    def _history(self):
        login = self._panthera_login()
        serial = self.serial.text().strip().upper()
        if login and serial:
            PantheraHistoryDialog(serial, login, self).exec_()

    def _clear(self):
        if QMessageBox.question(self, "Remove module",
                                "Remove %s from this position?" % self.m["serial"],
                                QMessageBox.Yes | QMessageBox.No, QMessageBox.No) == QMessageBox.Yes:
            if not self.inst.model.assign(self.mid, ""):
                self._save_failed()
            self.accept()

    def _save(self):
        """Validate and store the entry. Returns True on success."""
        model = self.inst.model
        serial = self.serial.text().strip().upper()
        port_txt = self.port.text().strip()
        if port_txt and not port_txt.isdigit():
            QMessageBox.warning(self, "Invalid FMC port", "FMC port must be a whole number.")
            return False
        port = int(port_txt) if port_txt else None
        mtype = self.type.currentText()
        mtype = None if mtype == AUTO_TYPE else mtype

        others = [mid for mid in model.find_by_serial(serial) if mid != self.mid] if serial else []
        if others:
            ans = QMessageBox.question(
                self, "Serial already placed",
                "%s is already assigned to %s.\nMove it here (that position becomes empty)?"
                % (serial, ", ".join(others)),
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if ans != QMessageBox.Yes:
                return False
            for mid in others:
                if not model.assign(mid, ""):
                    self._save_failed()
        if (self.m["serial"] and serial != self.m["serial"].upper()
                and self.m["status"] != "untested"):
            ans = QMessageBox.question(
                self, "Replace tested module",
                "This position holds %s (%s). Replace it with %s? Its status will reset to untested."
                % (self.m["serial"], self.m["status"].upper(), serial),
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if ans != QMessageBox.Yes:
                return False
        if not model.assign(self.mid, serial, port, mtype):
            self._save_failed()
        return True

    def _save_failed(self):
        QMessageBox.warning(
            self, "Not saved to disk",
            "The change is shown on the map but could not be written to\n%s\n"
            "It will be lost when the GUI closes. See the log for details."
            % self.inst.model.state_path)

    def _save_only(self):
        if self._save():
            self.accept()

    def _save_and_start(self):
        if not self.serial.text().strip():
            return
        if self._save() and self.inst.start_test(self.m):
            self.accept()


HISTORY_QSS = MODULE_QSS + """
QFrame#seq {border:1px solid #d7dae0;border-radius:6px;background:#fafbfc}
QFrame#seq QLabel {background:transparent}
QScrollArea, QScrollArea > QWidget > QWidget {background:#fff}
"""


def _esc(text):
    return html.escape(str(text or ""))


class PantheraHistoryDialog(QDialog):
    """Real test history of one module, read live from Panthera (read-only)."""

    def __init__(self, serial, login, parent):
        super().__init__(parent)
        self.serial = serial
        self.kf = getattr(parent, "kf", 1.0)
        self._alive = True
        self.setWindowTitle("Panthera history — %s" % serial)
        self.setStyleSheet(HISTORY_QSS)
        v = QVBoxLayout(self)

        self.status = QLabel("Loading %s from Panthera…" % _esc(serial))
        self.status.setWordWrap(True)
        v.addWidget(self.status)

        self.area = QScrollArea()
        self.area.setWidgetResizable(True)
        self.area.setFrameShape(QFrame.NoFrame)
        self.area.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        v.addWidget(self.area, 1)

        btns = QHBoxLayout()
        web = QPushButton("Open Panthera review page")
        web.clicked.connect(lambda: QDesktopServices.openUrl(QUrl(REVIEW_PAGE)))
        btns.addWidget(web)
        btns.addStretch()
        close = QPushButton("Close")
        close.clicked.connect(self.accept)
        btns.addWidget(close)
        v.addLayout(btns)

        scale_tree(self, self.kf)
        _fit_dialog(self, px(900, self.kf), px(700, self.kf))
        self._worker = fetch_history_async(login[0], login[1], serial, self._loaded, self._failed)

    def done(self, r):
        self._alive = False
        super().done(r)

    def _failed(self, message):
        if self._alive:
            self.status.setText("<span style='color:#b21d1d'>%s</span>" % _esc(message))

    def _loaded(self, hist):
        if not self._alive:
            return
        if hist.get("module_id") is None:
            self.status.setText("%s was not found on Panthera." % _esc(self.serial))
            return
        seqs = hist["sequences"]
        msg = "%d test sequence%s on Panthera (newest first)." % (len(seqs), "" if len(seqs) == 1 else "s")
        if hist.get("truncated"):
            msg += " Only the first %d are shown; see the Panthera page for the rest." % len(seqs)
        self.status.setText(msg)

        body = QWidget()
        bl = QVBoxLayout(body)
        bl.setContentsMargins(0, 6, 0, 0)
        info = [(k, val) for k, val in hist["module"].items() if val]
        if info:
            lab = QLabel("<br>".join("<b>%s:</b> %s" % (_esc(k), _esc(val)) for k, val in info))
            lab.setTextInteractionFlags(Qt.TextSelectableByMouse)
            bl.addWidget(lab)
        for q in seqs:
            bl.addWidget(self._sequence_frame(q))
        if not seqs:
            bl.addWidget(QLabel("No test sequences uploaded for this module yet."))
        bl.addStretch()
        scale_tree(body, self.kf)
        self.area.setWidget(body)

    def _sequence_frame(self, q):
        f = QFrame()
        f.setObjectName("seq")
        g = QVBoxLayout(f)
        g.setContentsMargins(10, 8, 10, 8)
        head = "<b>Sequence: %s</b> &nbsp; Location: <span title='%s'>%s</span> &nbsp; Upload Time: %s" % (
            _esc(q["name"]), _esc(q["location_full"]), _esc(q["location"]), _esc(q["upload_time"]))
        if q["reviewed"]:
            head += " &nbsp; <b style='color:%s'>%s</b>" % (_esc(q["reviewed_color"] or "#1c1c1c"),
                                                              _esc(q["reviewed"]))
        if q["is_bad"]:
            head += " &nbsp; <span style='background:%s;color:#fff'>&nbsp;%s&nbsp;</span>" % (
                _esc(q["is_bad_color"] or "#666"), _esc(q["is_bad"]))
        top = QLabel(head)
        top.setWordWrap(True)
        top.setToolTip("SequenceID %s" % q["id"])
        g.addWidget(top)
        lines = []
        if q["grades"]:
            lines.append("<b>Grades:</b> %s" % _esc(q["grades"]))
        for r in q["results"]:
            extra = ", ".join(x for x in (r["test_type"], r["time"], ("by " + r["by"]) if r["by"] else "") if x)
            lines.append("&bull; %s%s" % (_esc(r["name"]), (" <span style='color:#6b7280'>(%s)</span>" % _esc(extra))
                                          if extra else ""))
        reviewed = re.sub(r"(Latest Review Time|Last Reviewed By):", "", q["review_info"]).strip()
        meta = " · ".join(q["versions"] + ([q["review_info"]] if reviewed else []))
        if meta:
            lines.append("<span style='color:#6b7280;font-size:11px'>%s</span>" % _esc(meta))
        if lines:
            body = QLabel(sq("<br>".join(lines), self.kf))
            body.setWordWrap(True)
            body.setTextInteractionFlags(Qt.TextSelectableByMouse)
            g.addWidget(body)
        return f


# ============================================================ MAIN WINDOW
# The app-wide theme (Assets/*.qss, often dark) cascades into this window;
# reset it to the light look the map is designed for.
WINDOW_QSS = """
QWidget {background:transparent;color:#20242b;font-size:13px}
QMainWindow, QScrollArea, QScrollArea > QWidget > QWidget {background:#e7e9ec}
QCheckBox {color:#20242b;font-size:13px}
QCheckBox::indicator {width:14px;height:14px}
QComboBox {background:#fff;color:#20242b;border:1px solid #c4c8cf;border-radius:3px;padding:2px 4px}
QComboBox QAbstractItemView {background:#fff;color:#20242b}
QToolTip {background:#fffbe6;color:#20242b;border:1px solid #c4c8cf}
"""
ROW_QSS = "QFrame#row{background:#fff;border:1px solid #d7dae0;border-radius:12px}"
PREV_QSS = "QFrame#row{background:#fff;border:1px dashed #d7dae0;border-radius:12px}"
PC_QSS = "QFrame#row{background:#eef1f6;border:1px solid #d7dae0;border-radius:12px}"


class QtInstallationWindow(QMainWindow):
    BASE_FIXED_W = 345    # UI-scaled width per cartridge row (margins, label column, ...)
    BASE_FIXED_H = 200    # UI-scaled height (header, padding, captions)
    DEE_ROW_W = 4 * DEE_W  # map-scaled width: four Dee drawings per row

    def __init__(self, master):
        super().__init__()
        self.master = master
        self.setWindowTitle("TFPX Module Installation Status — Dee View")
        self.model = InstallationModel()
        self.model.statusChanged.connect(self.refresh)
        self._screen_hooked = False
        self._rescale_timer = QTimer(self)
        self._rescale_timer.setSingleShot(True)
        self._rescale_timer.timeout.connect(self._maybe_rescale)

        self.fit_to_screen(_screen_of(master) if master is not None else None)
        self.k, self.kf = self._scales()
        self._build()

    # ---- sizing
    def fit_to_screen(self, screen=None):
        """Size the window to most of the given screen and centre it there."""
        screen = screen or self._current_screen()
        self._target_screen = screen
        avail = screen.availableGeometry()
        w, h = int(avail.width() * 0.92), int(avail.height() * 0.90)
        self.resize(w, h)
        self.move(avail.x() + (avail.width() - w) // 2, avail.y() + (avail.height() - h) // 2)

    def _current_screen(self):
        handle = self.windowHandle()
        if self.isVisible() and handle is not None and handle.screen() is not None:
            return handle.screen()
        return getattr(self, "_target_screen", None) or QApplication.primaryScreen()

    def _scales(self):
        kf = screen_ui_scale(self._current_screen())
        scrollbar = 20 * kf
        kw = (self.width() - scrollbar - self.BASE_FIXED_W * kf) / self.DEE_ROW_W
        kh = (self.height() - self.BASE_FIXED_H * kf) / DEE_H
        return max(0.75, min(kw, kh, 6.0)), kf

    def showEvent(self, e):
        super().showEvent(e)
        if not self._screen_hooked and self.windowHandle() is not None:
            self.windowHandle().screenChanged.connect(self._on_screen_changed)
            self._screen_hooked = True

    def _on_screen_changed(self, screen):
        self.fit_to_screen(screen)
        self._rescale_timer.start(150)

    def resizeEvent(self, e):
        super().resizeEvent(e)
        self._rescale_timer.start(150)  # debounce while the user drags

    def _maybe_rescale(self):
        k, kf = self._scales()
        if abs(k - self.k) / self.k > 0.04 or abs(kf - self.kf) > 0.01:
            self.k, self.kf = k, kf
            self._build()

    def _build(self):
        """(Re)create all content at the current scale, keeping view state."""
        show_prev = self.toggle.isChecked() if hasattr(self, "toggle") else False
        old_scroll = getattr(self, "scroll_area", None)
        frac = 0.0
        if old_scroll is not None:
            bar = old_scroll.verticalScrollBar()
            frac = bar.value() / bar.maximum() if bar.maximum() else 0.0

        self.setStyleSheet(sq(WINDOW_QSS, self.kf))
        self.prev_rows = []
        self.dee_widgets = []
        self.count_labels = {}

        central = QWidget()
        v = QVBoxLayout(central)
        v.setContentsMargins(0, 0, 0, 0)
        v.setSpacing(0)
        v.addWidget(self._header())

        self.scroll_area = scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.NoFrame)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        v.addWidget(scroll)
        main = QWidget()
        ml = QVBoxLayout(main)
        ml.setContentsMargins(18, 22, 18, 60)
        ml.setAlignment(Qt.AlignTop)
        scroll.setWidget(main)

        hint = QLabel(
            "Click any module box to enter/edit its module info (serial, FMC port, type) and start a test "
            "prefilled with it. Yellow = assigned but not yet tested; Green/Red = latest test passed/failed; "
            "Gray = no module assigned yet. \"Show previous results\" adds each module's status from before "
            "its latest test. Each cartridge has 2 Dees (Dee 1, Dee 2), each Dee has its own "
            "Side A and Side B, plus a shared ring of 10 portcards (PC10→PC1). Side B's rings (2 & 4) carry 8 "
            "modules each; Side A's rings (1 & 3) carry 5 and 6.")
        hint.setWordWrap(True)
        hint.setStyleSheet("font-size:12px;color:#6b7280")
        ml.addWidget(hint)
        for cart in range(1, N_CARTRIDGES + 1):
            ml.addWidget(self._cartridge_block(cart))

        scale_tree(central, self.kf)
        self.setCentralWidget(central)  # deletes the previous content
        self.toggle.setChecked(show_prev)
        self._set_prev_visible(show_prev)
        self._update_counts()
        if frac:
            QTimer.singleShot(0, lambda: scroll.verticalScrollBar().setValue(
                int(frac * scroll.verticalScrollBar().maximum())))

    # ---- header
    def _header(self):
        h = QFrame()
        h.setStyleSheet("QFrame#hdr{background:#fff;border-bottom:1px solid #d7dae0}")
        h.setObjectName("hdr")
        l = QHBoxLayout(h)
        l.setContentsMargins(22, 14, 22, 14)
        l.setSpacing(18)
        l.addWidget(logo_label("osu_logo.jpg", px(34, self.kf)))
        l.addWidget(logo_label("cms_logo.png", px(34, self.kf)))
        t = QVBoxLayout()
        title = QLabel("TFPX Module Installation Status — Dee View")
        title.setStyleSheet("font-size:18px;font-weight:600")
        sub = QLabel("8 cartridges × 2 Dees × 2 sides (A/B) · 10 portcards/cartridge · "
                     "click a module to open its interface")
        sub.setStyleSheet("color:#6b7280;font-size:12px")
        t.addWidget(title)
        t.addWidget(sub)
        l.addLayout(t)
        l.addStretch()
        self.toggle = QCheckBox("Show previous results")
        self.toggle.toggled.connect(self._set_prev_visible)
        l.addWidget(self.toggle)
        for key, name in (("untested", "Untested"), ("pass", "Pass"), ("fail", "Fail")):
            sw = QLabel()
            sw.setFixedSize(14, 14)
            sw.setStyleSheet("background:%s;border:1px solid rgba(0,0,0,.25);border-radius:3px" % COLORS[key])
            l.addWidget(sw)
            lb = QLabel()
            self.count_labels[key] = (lb, name)
            l.addWidget(lb)
        return h

    def _update_counts(self):
        counts = self.model.counts()
        for key, (lb, name) in self.count_labels.items():
            lb.setText(sq("%s <span style='color:#6b7280;font-size:11px'>(%d)</span>", self.kf)
                       % (name, counts[key]))

    def _set_prev_visible(self, on):
        for r in self.prev_rows:
            r.setVisible(on)

    # ---- cartridge block
    def _row_frame(self, qss, label_text, small, extra=None):
        f = QFrame()
        f.setObjectName("row")
        f.setStyleSheet(qss)
        h = QHBoxLayout(f)
        h.setContentsMargins(16, 12, 16, 12)
        lw = QWidget()
        lw.setFixedWidth(175)
        ll = QVBoxLayout(lw)
        ll.setContentsMargins(0, 0, 0, 0)
        ll.setSpacing(3)
        t = QLabel("<b>%s</b>" % label_text)
        t.setStyleSheet("border:none;font-size:14px")
        s = QLabel(small)
        s.setStyleSheet("border:none;color:#6b7280;font-size:11px")
        ll.addWidget(t)
        ll.addWidget(s)
        if extra is not None:
            ll.addWidget(extra)
        ll.addStretch()
        h.addWidget(lw, 0, Qt.AlignTop)
        return f, h

    def _dee_row(self, cart, mode):
        w = QWidget()
        h = QHBoxLayout(w)
        h.setContentsMargins(0, 0, 0, 0)
        h.setSpacing(22)
        for dee, dlabel in DEES:
            pair = QFrame()
            ph = QHBoxLayout(pair)
            ph.setContentsMargins(8, 0, 8, 0)
            ph.setSpacing(6)
            if dee == "2":
                pair.setStyleSheet("QFrame{border-left:1px solid #d7dae0}")
            for side, slabel in SIDES:
                col = QVBoxLayout()
                col.setSpacing(4)
                dw = DeeWidget(self.model.cart_data[(cart, dee, side)], mode, self.open_module, self.k)
                self.dee_widgets.append(dw)
                col.addWidget(dw, 0, Qt.AlignHCenter)
                cap = QLabel("%s · %s" % (dlabel, slabel))
                cap.setAlignment(Qt.AlignCenter)
                cap.setStyleSheet("color:#6b7280;font-size:11px;border:none")
                col.addWidget(cap)
                ph.addLayout(col)
            h.addWidget(pair)
        h.addStretch()
        return w

    def _cartridge_block(self, cart):
        block = QWidget()
        bl = QVBoxLayout(block)
        bl.setContentsMargins(0, 0, 0, 0)
        bl.setSpacing(8)

        cur, ch = self._row_frame(ROW_QSS, "Cartridge %d" % cart, "Current results")
        ch.addWidget(self._dee_row(cart, "cur"), 1)
        bl.addWidget(cur)

        prev, ph = self._row_frame(PREV_QSS, "Cartridge %d" % cart, "Before latest test")
        ph.addWidget(self._dee_row(cart, "prev"), 1)
        self.prev_rows.append(prev)
        bl.addWidget(prev)

        pc, pl = self._row_frame(PC_QSS, "Cartridge %d" % cart, "Portcards")
        pcw = QWidget()
        pch = QHBoxLayout(pcw)
        pch.setContentsMargins(0, 0, 0, 0)
        pch.setSpacing(9)
        for n in range(10, 0, -1):
            col = QVBoxLayout()
            col.setSpacing(3)
            col.addWidget(PortcardWidget(cart, n, self.k), 0, Qt.AlignHCenter)
            cap = QLabel("PC%d" % n)
            cap.setAlignment(Qt.AlignCenter)
            cap.setStyleSheet("color:#6b7280;font-size:10px;border:none")
            col.addWidget(cap)
            pch.addLayout(col)
        pch.addStretch()
        pl.addWidget(pcw, 1)
        bl.addWidget(pc)
        return block

    # ---- module click
    def open_module(self, m, status, mode="cur"):
        ModuleDialog(m, self).exec_()

    def start_test(self, m):
        """Open the regular QtStartWindow prefilled for module m. Returns True if opened."""
        if getattr(self.master, "ProcessingTest", False):
            QMessageBox.information(self, "Test in progress",
                                    "A test is already open or running. Finish or close it first.")
            return False
        if not (self.master.ActiveFC7s or self.master.ui_testing):
            QMessageBox.information(self, "No FC7 selected",
                                    "Select an FC7 with its \"Use\" button in the main window first.")
            return False
        try:
            self.master.openNewTest(install_module=m)
            return True
        except Exception:
            logger.error("Failed to open test for %s" % m["serial"], exc_info=True)
            return False

    def closeEvent(self, e):
        # Every edit is already saved as it happens; this is a final safety net.
        self.model.save()
        super().closeEvent(e)

    def refresh(self, mid=None):
        for dw in self.dee_widgets:
            dw.update()
        self._update_counts()
