"""Backend model for the TFPX Installation GUI (Dee view).

Holds the per-position module data (geometry + assignment + test status),
persists it to ``$GUI_dir/Gui/jsonFiles/installation_state.json`` and emits
``statusChanged(module_id)`` whenever a module's status changes.

Modules are assigned to positions from the GUI (click a box). On the very
first start only (no installation_state.json yet), an optional
``$GUI_dir/Gui/jsonFiles/module_map.json`` can seed assignments, keyed by module id
(see ``module_id``)::

    {"c1d1Br2m3": {"serial": "RH0123", "fmc_port": 0, "type": "TFPX CROC 1x2"}}

Positions without a serial are drawn gray.
"""
import json
import math
import os
import traceback

from PyQt5.QtCore import QObject, pyqtSignal

from Gui.python.logging_config import get_logger

logger = get_logger(__name__)

# ============================================================ GEOMETRY
RING_RADIUS = {1: 46, 2: 46, 3: 82, 4: 82}
RING_SIZE = {1: (11, 18), 2: (11, 18), 3: (22, 36), 4: (22, 36)}  # (tangential, radial)
RING_COUNT = {1: 5, 2: 8, 3: 6, 4: 8}
RING_NUMBERING = {"B": [2, 4], "A": [1, 3]}
A0, A1 = 96, 264
CX, CY = 112, 109

SIDES = [("B", "Side B"), ("A", "Side A")]
DEES = [("1", "Dee 1"), ("2", "Dee 2")]
SIDE_LABEL = dict(SIDES)
DEE_LABEL = dict(DEES)
N_CARTRIDGES = 8

STATE_FILENAME = "installation_state.json"
MAP_FILENAME = "module_map.json"
STATUSES = ("untested", "pass", "fail")


def polar(r, deg):
    a = math.radians(deg)
    return CX + r * math.cos(a), CY - r * math.sin(a)


def module_id(m):
    return f"c{m['cartridge']}d{m['dee']}{m['side']}r{m['ring']}m{m['slot']}"


def build_dee_modules(cart, side, dee):
    """Return list of module dicts (with geometry in viewBox coords) for one (Dee, Side)."""
    mods = []
    for ring in RING_NUMBERING[side]:
        n = RING_COUNT[ring]
        r_mid = RING_RADIUS[ring]
        w, h = RING_SIZE[ring]
        slot = (A1 - A0) / n
        for k in range(n):
            ang = A0 + slot * (k + 0.5)
            cx, cy = polar(r_mid, ang)
            num = (n - k) if side == "B" else (k + 1)
            mods.append(dict(
                serial="", status="untested", prev="untested",
                cartridge=cart, dee=dee, side=side, ring=ring, slot=num,
                cx=cx, cy=cy, w=w, h=h, rot=90 - ang,
                fmc_port=None, type=None))
    return mods


def _storage_dir():
    # Gui/jsonFiles is bind-mounted by run_Docker.sh, so files written there
    # survive the (--rm) container; the top of $GUI_dir does not.
    gui_dir = os.environ.get("GUI_dir") or os.path.dirname(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    return os.path.join(gui_dir, "Gui", "jsonFiles")


# ============================================================ MODEL
class InstallationModel(QObject):
    statusChanged = pyqtSignal(str)

    def __init__(self, base_dir=None, parent=None):
        super().__init__(parent)
        base_dir = base_dir or _storage_dir()
        self.state_path = os.path.join(base_dir, STATE_FILENAME)
        self.map_path = os.path.join(base_dir, MAP_FILENAME)

        self.cart_data = {}
        self.by_id = {}
        for cart in range(1, N_CARTRIDGES + 1):
            for dee, _ in DEES:
                for side, _ in SIDES:
                    mods = build_dee_modules(cart, side, dee)
                    self.cart_data[(cart, dee, side)] = mods
                    for m in mods:
                        self.by_id[module_id(m)] = m

        self._load()

    # ---- persistence
    @staticmethod
    def _read_json(path):
        if not os.path.isfile(path):
            return {}
        try:
            with open(path) as f:
                data = json.load(f)
            if not isinstance(data, dict):
                raise ValueError("top-level JSON must be an object")
            return data
        except Exception:
            logger.error(f"Could not read {path}:\n{traceback.format_exc()}")
            return {}

    def _load(self):
        # module_map.json only seeds the very first start. Once
        # installation_state.json exists, it (edited via the GUI) is the record.
        if os.path.isfile(self.state_path):
            source, keys = self.state_path, ("serial", "fmc_port", "type", "status", "prev")
        else:
            source, keys = self.map_path, ("serial", "fmc_port", "type")
        for mid, entry in self._read_json(source).items():
            m = self.by_id.get(mid)
            if m is None:
                logger.warning(f"Unknown module position '{mid}' in {source}; ignored")
                continue
            for key in keys:
                if key not in entry:
                    continue
                if key in ("status", "prev") and entry[key] not in STATUSES:
                    continue
                m[key] = entry[key]
            m["serial"] = str(m["serial"] or "").strip()

        self._warn_duplicates()

    def _warn_duplicates(self):
        seen = {}
        for mid, m in self.by_id.items():
            s = (m["serial"] or "").upper()
            if s:
                seen.setdefault(s, []).append(mid)
        for s, mids in seen.items():
            if len(mids) > 1:
                logger.warning(f"Serial {s} is assigned to multiple positions: {', '.join(mids)}")

    def save(self):
        """Write the state file. Returns True on success."""
        data = {
            mid: {k: m[k] for k in ("serial", "fmc_port", "type", "status", "prev")}
            for mid, m in self.by_id.items()
            if m["serial"]
        }
        tmp = self.state_path + ".tmp"
        try:
            with open(tmp, "w") as f:
                json.dump(data, f, indent=2, sort_keys=True)
            os.replace(tmp, self.state_path)
            return True
        except Exception:
            logger.error(f"Could not save {self.state_path}:\n{traceback.format_exc()}")
            return False

    # ---- queries
    def module_info(self, mid):
        m = self.by_id[mid]
        return dict(serial=m["serial"], fmc_port=m["fmc_port"], type=m["type"])

    def find_by_serial(self, serial):
        s = (serial or "").strip().upper()
        if not s:
            return []
        return [mid for mid, m in self.by_id.items() if (m["serial"] or "").upper() == s]

    def status_of(self, serial):
        mids = self.find_by_serial(serial)
        return self.by_id[mids[0]]["status"] if mids else None

    def counts(self):
        """Status counts over positions that have a module assigned."""
        c = {k: 0 for k in STATUSES}
        for m in self.by_id.values():
            if m["serial"]:
                c[m["status"]] += 1
        return c

    # ---- updates
    def assign(self, mid, serial, fmc_port=None, mtype=None):
        """Put a module (or nothing, if serial is empty) at a position.

        A new serial at the position resets its status to untested.
        """
        m = self.by_id[mid]
        serial = (serial or "").strip()
        if serial.upper() != (m["serial"] or "").upper():
            m["status"] = m["prev"] = "untested"
        m["serial"] = serial
        m["fmc_port"] = fmc_port if serial else None
        m["type"] = (mtype or None) if serial else None
        ok = self.save()
        self.statusChanged.emit(mid)
        return ok

    def record_result(self, serial, passed, prev=None):
        """Set the module with this serial to pass/fail.

        ``prev`` overrides the value stored as the previous status (used when a
        composite test reports several times during one run).
        """
        mids = self.find_by_serial(serial)
        if not mids:
            logger.warning(f"record_result: serial {serial} is not on the installation map")
            return False
        if len(mids) > 1:
            logger.warning(
                f"record_result: serial {serial} is at multiple positions {mids}; updating {mids[0]} only")
        mid = mids[0]
        m = self.by_id[mid]
        m["prev"] = prev if prev in STATUSES else m["status"]
        m["status"] = "pass" if passed else "fail"
        self.save()
        self.statusChanged.emit(mid)
        return True
