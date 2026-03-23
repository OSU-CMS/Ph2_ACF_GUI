from PyQt5 import QtCore
from PyQt5.QtCore import Qt
from PyQt5.QtCore import pyqtSignal, QTimer
from PyQt5.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QGridLayout,
    QGroupBox,
    QLabel,
    QLineEdit,
    QMessageBox,
    QPushButton,
    QWidget,
    QVBoxLayout,
    QScrollArea,
)

import sys
import requests
from lxml import etree
import re
import traceback
import requests
import time
import threading

from icicle.icicle.instrument_cluster import DummyInstrument
import Gui.siteSettings as site_settings
from Gui.python.Firmware import (
    QtModule,
    QtOpticalGroup,
)
from Gui.GUIutils.settings import (
    ModuleLaneMap,
    ModuleLaneMap_Dict,
    ModuleType,
)
from InnerTrackerTests.FESettings import (
    FESettingsB,
)
# from Gui.GUIutils.FirmwareUtil import *
# from Gui.QtGUIutils.QtFwCheckDetails import *
from Gui.python.CentralDBInterface import ExtractChipData

from Gui.python.logging_config import get_logger

logger = get_logger(__name__)
# Global dictionary to store IREF values for each chip
chip_iref_db = {}
chip_data_cache = {}


class _AsyncCallSignals(QtCore.QObject):
    result = QtCore.pyqtSignal(object)
    error = QtCore.pyqtSignal(str)


class _AsyncCallRunnable(QtCore.QRunnable):
    def __init__(self, fn):
        super().__init__()
        self.fn = fn
        self.signals = _AsyncCallSignals()

    def run(self):
        try:
            result = self.fn()
            self.signals.result.emit(result)
        except Exception as e:
            self.signals.error.emit(str(e))


class ModuleRegistryDataClient:
    _CMS_BASE_URL = "https://cms-it-modules-registry.web.cern.ch"
    _REGISTRIES = {
        "quad": f"{_CMS_BASE_URL}/IT_Quad_Module.json",
        "dual": f"{_CMS_BASE_URL}/IT_Double_Module.json",
    }
    _PURDUE_URL = "https://www.physics.purdue.edu/cmsfpix/Phase2_Test/w.php?sn={module}"

    def __init__(self, ttl_seconds=900):
        self.session = requests.Session()
        self.ttl_seconds = ttl_seconds
        self.thread_pool = QtCore.QThreadPool.globalInstance()
        self._cache_lock = threading.RLock()
        self._network_lock = threading.Lock()
        self._registry_cache = {}
        self._module_json_cache = {}
        self._module_identity_cache = {}
        self._purdue_cache = {}

    def _normalize_module_name(self, moduleName):
        return moduleName.strip().upper() if moduleName else ""

    def _cache_get(self, cache, key):
        with self._cache_lock:
            item = cache.get(key)
            if not item:
                return None
            if (time.time() - item["ts"]) > self.ttl_seconds:
                cache.pop(key, None)
                return None
            return item["value"]

    def _cache_set(self, cache, key, value):
        with self._cache_lock:
            cache[key] = {"ts": time.time(), "value": value}

    def _get_json(self, url):
        with self._network_lock:
            resp = self.session.get(url, timeout=5)
        resp.raise_for_status()
        return resp.json()

    def run_async(self, fn, on_success=None, on_error=None):
        worker = _AsyncCallRunnable(fn)
        if on_success is not None:
            worker.signals.result.connect(on_success)
        if on_error is not None:
            worker.signals.error.connect(on_error)
        self.thread_pool.start(worker)
        return worker

    def get_registry(self, moduleType):
        cached = self._cache_get(self._registry_cache, moduleType)
        if cached is not None:
            return cached

        url = self._REGISTRIES[moduleType]
        registry = self._get_json(url)
        self._cache_set(self._registry_cache, moduleType, registry)
        return registry

    def find_module_identity(self, moduleName):
        moduleName_clean = self._normalize_module_name(moduleName)
        if not moduleName_clean:
            return None, None

        cached = self._cache_get(self._module_identity_cache, moduleName_clean)
        if cached is not None:
            return cached

        for moduleType in ["quad", "dual"]:
            try:
                registry = self.get_registry(moduleType)
            except Exception as e:
                logger.warning(
                    f"Error loading {moduleType} registry from {self._REGISTRIES[moduleType]}: {e}"
                )
                continue

            for entry in registry:
                serial = (entry.get("SERIAL_NUMBER") or "").strip().upper()
                if serial == moduleName_clean:
                    name_label = entry.get("NAME_LABEL")
                    result = (name_label, moduleType)
                    self._cache_set(self._module_identity_cache, moduleName_clean, result)
                    return result

        result = (None, None)
        self._cache_set(self._module_identity_cache, moduleName_clean, result)
        return result

    def get_module_json(self, name_label):
        if not name_label:
            return None

        cached = self._cache_get(self._module_json_cache, name_label)
        if cached is not None:
            return cached

        url = f"{self._CMS_BASE_URL}/{name_label}.json"
        data = self._get_json(url)
        self._cache_set(self._module_json_cache, name_label, data)
        return data

    def get_purdue_response(self, moduleName):
        moduleName_clean = self._normalize_module_name(moduleName)
        if not moduleName_clean:
            return None

        cached = self._cache_get(self._purdue_cache, moduleName_clean)
        if cached is not None:
            return cached

        url = self._PURDUE_URL.format(module=moduleName_clean)
        with self._network_lock:
            response = self.session.get(url, timeout=5)
        response.raise_for_status()
        self._cache_set(self._purdue_cache, moduleName_clean, response)
        return response

    def fetch_module_type(self, moduleName):
        name_label, moduleType = self.find_module_identity(moduleName)

        if name_label and moduleType:
            try:
                data = self.get_module_json(name_label)
                moduleversion = None
                for entry in data.get("bare_module_data", []):
                    version = entry.get("VERSION")
                    if version is not None:
                        moduleversion = version
                        break

                mv = _normalize_hdi_version(moduleversion, default="1")
                moduletype = _module_type_to_display(moduleType)
                logger.info(f"Module type from CMS database: {moduletype}, HDI version: {mv}")
                return {"type": moduletype, "HDIversion": f"{mv}"}
            except Exception as e:
                logger.warning(
                    f"Could not fetch module JSON for {name_label} from CMS: {e}. Trying Purdue..."
                )

        logger.warning(f"Module {moduleName} not found in CMS database. Trying Purdue...")
        try:
            response = self.get_purdue_response(moduleName)
        except Exception as e:
            logger.warning(f"Failed to access Purdue database: {e}")
            response = None

        if response:
            moduletype = None
            moduleversion = None

            part_match = re.search(r"Part\s*=\s*([^<\n\r]+)", response.text)
            version_match = re.search(r"Version\s*=\s*([^<\n\r]+)", response.text)
            if part_match:
                moduletype = part_match.group(1).strip()
            if version_match:
                moduleversion = version_match.group(1).strip()

            if moduletype and moduleversion:
                if moduletype.startswith("croc_1x2"):
                    moduletype = "TFPX CROC 1x2"
                elif moduletype.startswith("croc_2x2"):
                    moduletype = "TFPX CROC Quad"

                mv = _normalize_hdi_version(moduleversion, default=moduleversion)
                logger.info(f"Module type from Purdue database: {moduletype}, HDI version: {mv}")
                return {"type": moduletype, "HDIversion": f"{mv}"}

        return None

    def fetch_module_type_async(self, moduleName, on_success=None, on_error=None):
        return self.run_async(
            lambda: self.fetch_module_type(moduleName),
            on_success=on_success,
            on_error=on_error,
        )


module_registry_client = ModuleRegistryDataClient()


def _normalize_hdi_version(version, default="1"):
    if version is None:
        return default
    try:
        vstr = str(version).strip()
        if "." in vstr:
            return str(int(float(vstr)))
        try:
            return str(int(vstr))
        except Exception:
            return vstr
    except Exception:
        return default


def _module_type_to_display(moduleType):
    if moduleType == "dual":
        return "TFPX CROC 1x2"
    if moduleType == "quad":
        return "TFPX CROC Quad"
    return ""


def _fetch_module_type_from_databases(moduleName):
    return module_registry_client.fetch_module_type(moduleName)


class ClickOnlyComboBox(QComboBox):
    def __init__(self, parent=None):
        super().__init__(parent)

    def wheelEvent(self, event):
        event.ignore()


def debounce(wait):
    def decorator(fn):
        timer = None

        def debounced(*args, **kwargs):
            nonlocal timer
            if timer is not None:
                timer.stop()
            timer = QTimer()
            timer.setSingleShot(True)
            timer.timeout.connect(lambda: fn(*args, **kwargs))
            timer.start(wait)

        return debounced

    return decorator


class ModuleBox(QWidget):
    typechanged = pyqtSignal()

    def __init__(self, firmware):
        super(ModuleBox, self).__init__()
        self.mainLayout = QGridLayout()
        self.firmware = firmware
        self.createRow()
        self.setLayout(self.mainLayout)
        self.VDDDmap = {}
        self.VDDAmap = {}

    def createRow(self):
        SerialLabel = QLabel("SerialNumber:")
        self.SerialEdit = QLineEdit()
        self.SerialEdit.setMinimumWidth(55)

        PortLabel = QLabel("FMC port:")
        self.PortEdit = QLineEdit()

        TypeLabel = QLabel("Type:")
        self.TypeCombo = ClickOnlyComboBox()
        self.TypeCombo.addItems(ModuleType.values())
        TypeLabel.setBuddy(self.TypeCombo)

        FC7Label = QLabel("FC7:")
        self.FC7Combo = ClickOnlyComboBox()
        self.FC7Combo.addItems([board.getBoardName() for board in self.firmware])
        if self.FC7Combo.count() == 1:
            self.FC7Combo.setDisabled(True)

        VersionLabel = QLabel("CROC_Version:")
        self.VersionCombo = ClickOnlyComboBox()
        self.VersionCombo.addItems(["v1", "v2"])
        VersionLabel.setBuddy(self.VersionCombo)

        HDIVersionLabel = QLabel("HDI_Version:")
        self.HDIVersionCombo = ClickOnlyComboBox()
        self.HDIVersionCombo.addItems(["1", "2"])
        HDIVersionLabel.setBuddy(self.HDIVersionCombo)

        self.mainLayout.addWidget(SerialLabel, 0, 0, 1, 1)
        self.mainLayout.addWidget(self.SerialEdit, 0, 1, 1, 1)
        self.mainLayout.addWidget(FC7Label, 0, 4, 1, 1)
        self.mainLayout.addWidget(self.FC7Combo, 0, 5, 1, 1)
        self.mainLayout.addWidget(PortLabel, 0, 6, 1, 1)
        self.mainLayout.addWidget(self.PortEdit, 0, 7, 1, 1)
        self.mainLayout.addWidget(TypeLabel, 0, 8, 1, 1)
        self.mainLayout.addWidget(self.TypeCombo, 0, 9, 1, 1)
        # self.mainLayout.addWidget(VersionLabel, 0, 10, 1, 1)
        self.mainLayout.addWidget(self.VersionCombo, 0, 11, 1, 1)
        self.mainLayout.addWidget(HDIVersionLabel, 0, 12, 1, 1)
        self.mainLayout.addWidget(self.HDIVersionCombo, 0, 13, 1, 1)

    def setType(self):
        # this method is created to set moudle type under online mode and comboBox is hidden
        # This method is actually never used as far as I can tell. ~MJ 2025-01-16
        if self.SerialEdit.text().lower().startswith("rh"):
            chipType = "CROC 1x2"
            self.TypeCombo.setCurrentText(chipType)

        if self.SerialEdit.text().lower().startswith("sh"):
            chipType = "TFPX CROC Quad"
            self.TypeCombo.setCurrentText(chipType)
            numpart = "".join(filter(str.isdigit, self.SerialEdit.text()))
            if numpart.isdigit() and int(numpart) > 49:
                self.VersionCombo.setCurrentText(2)
            else:
                self.VersionCombo.setCurrentText(1)

    def checkPort(self, port):
        try:
            if port in range(0,2):
                return "L12"
            elif port in range (3,4):
                return "L8"
            else:
                logger.warning(f"Unknown port: {port}")
                return "L12"
        except ValueError:
            logger.error("Invalid port value. Port must be an integer.")
            return "L12"
        except Exception:
            logger.error(traceback.format_exc())
            return "L12"

    def getSerialNumber(self):
        return self.SerialEdit.text().upper()

    def getFMCID(self):
        return self.checkPort(int(self.getFMCPort()))

    def getFC7(self):
        return self.FC7Combo.currentText()

    def getFMCPort(self):
        return self.PortEdit.text()

    def getType(self):
        return self.TypeCombo.currentText()

    def getVersion(self):
        return self.VersionCombo.currentText()
    
    def getHDIVersion(self):
        return self.HDIVersionCombo.currentText()

    def getVDDD(self, pChipID):
        return self.VDDD[pChipID]


class ChipBox(QWidget):
    chipchanged = pyqtSignal(int, int)
    chipDataLoaded = pyqtSignal(dict)

    # adding default value to serialNumber="RH0009" can prevent ChipBox from crashing under online mode
    def __init__(self, master, pChipType, serialNumber="RH0009"):
        super().__init__()
        logger.debug("Inside ChipBox")
        self.master = master
        self.serialNumber = serialNumber
        self.chipType = pChipType
        logger.debug("the chip type passed to the chipbox is {0}".format(self.chipType))
        self.mainLayout = QVBoxLayout()
        self.ChipList = []  # chip id list for a single module
        # self.initList()
        self.createList()
        self.VDDAMap = {}
        self.VDDDMap = {}
        self.ChipGroupBoxDict = {}
        self.trimValues = None
        self.chipData = None
        self._data_loading = False

        # Create layout before fetching data
        self.mainLayout.addStretch()
        self.setLayout(self.mainLayout)
        
        # Show placeholder while loading data
        if self.master.purdue_connected and self.serialNumber != "":
            self._data_loading = True
            self._add_loading_indicator()
            # Fetch data asynchronously without blocking UI
            self.fetchChipDataFromDB_async(
                self.serialNumber,
                on_success=self._on_chip_data_loaded,
                on_error=self._on_chip_data_error
            )
        else:
            self._populate_chip_boxes()

    def initList(self):
        self.module = ModuleBox(self.master.firmware)

    # Makes a list of chips for a given module
    def createList(self):
        for lane in ModuleLaneMap[self.chipType]:
            self.ChipList.append(ModuleLaneMap[self.chipType][lane])

    def _add_loading_indicator(self):
        """Show a loading message while fetching chip data"""
        loading_label = QLabel("Loading chip data...")
        loading_label.setAlignment(Qt.AlignCenter)
        self.mainLayout.addWidget(loading_label)

    def _populate_chip_boxes(self, modulechipdata=None):
        """Populate the chip boxes in the main layout"""
        # Clear existing layout
        while self.mainLayout.count():
            item = self.mainLayout.takeAt(0)
            if item is not None:
                widget = item.widget()
                if widget is not None:
                    widget.deleteLater()
        
        if modulechipdata and set(self.ChipList) == set(modulechipdata.keys()):
            # Data matches expected chip layout
            for chipid in self.ChipList:
                self.ChipGroupBoxDict[chipid] = self.makeChipBoxWithDB(
                    chipid,
                    modulechipdata[chipid]["VDDA"],
                    modulechipdata[chipid]["VDDD"],
                    modulechipdata[chipid]["EFUSE"],
                    modulechipdata[chipid]["IREF"],
                )
        else:
            # Use default chip boxes (no database values)
            if modulechipdata:
                print(
                    f"Module {self.serialNumber} chip layout does not correspond to typical {self.chipType} chip layouts. Please modify the trim values manually."
                )
            self.ChipGroupBoxDict.clear()
            for chipid in self.ChipList:
                self.ChipGroupBoxDict[chipid] = self.makeChipBox(chipid)

        self.makeChipGroupBox(self.ChipGroupBoxDict)
        self.mainLayout.addStretch()

    def _on_chip_data_loaded(self, modulechipdata):
        """Callback when chip data is successfully loaded from database"""
        logger.debug(f"Chip data loaded for {self.serialNumber}")
        self._data_loading = False
        self.chipData = modulechipdata
        self._populate_chip_boxes(modulechipdata)
        self.chipDataLoaded.emit(modulechipdata)

    def _on_chip_data_error(self, error_msg):
        """Callback when chip data loading fails"""
        logger.warning(f"Failed to load chip data for {self.serialNumber}: {error_msg}")
        self._data_loading = False
        # Populate with default boxes without database values
        self._populate_chip_boxes(None)

    def fetchChipDataFromDB_async(self, moduleName, on_success=None, on_error=None):
        """Asynchronously fetch chip data from database"""
        def _fetch():
            return self.fetchChipDataFromDB(moduleName)

        module_registry_client.run_async(
            _fetch,
            on_success=on_success,
            on_error=on_error
        )

    def fetchTrimFromDB_async(self, moduleName, on_success=None, on_error=None):
        """Asynchronously fetch trim data from database"""
        def _fetch():
            return self.fetchTrimFromDB(moduleName)

        module_registry_client.run_async(
            _fetch,
            on_success=on_success,
            on_error=on_error
        )

    def fetchHDIVersionFromDB_async(self, moduleName, on_success=None, on_error=None):
        """Asynchronously fetch HDI version from database"""
        def _fetch():
            return self.fetchHDIVersionFromDB(moduleName)

        module_registry_client.run_async(
            _fetch,
            on_success=on_success,
            on_error=on_error
        )



    # get trim values from DB
    def makeChipBoxWithDB(self, pChipID, VDDA, VDDD, EfuseID="0", IREF="0"):
        self.ChipID = pChipID
        self.ChipLabel = QCheckBox("Chip ID: {0}".format(self.ChipID))
        self.ChipLabel.setChecked(True)
        self.ChipLabel.setObjectName("ChipStatus_{0}".format(pChipID))
        self.ChipVDDDLabel = QLabel("VDDD:")
        self.ChipVDDDEdit = QLineEdit()
        self.ChipVDDDEdit.setObjectName("VDDDEdit_{0}".format(pChipID))

        self.IREF = IREF
        chip_iref_db[str(pChipID)] = str(IREF)  # Store as string for easy comparison

        if not self.ChipVDDDEdit.text():
            logger.debug("no VDDD text")
        self.ChipVDDALabel = QLabel("VDDA:")
        self.ChipVDDAEdit = QLineEdit()
        self.ChipVDDDEdit.setText(VDDD)
        self.ChipVDDAEdit.setText(VDDA)
        self.ChipVDDAEdit.setObjectName("VDDAEdit_{0}".format(pChipID))

        self.ChipEfuseIDLabel = QLabel("Efuse ID:")
        self.ChipEfuseIDEdit = QLineEdit()
        self.ChipEfuseIDEdit.setText(EfuseID)
        self.ChipEfuseIDEdit.setObjectName("EfuseIDEdit_{0}".format(pChipID))

        self.HChipLayout = QGridLayout()
        self.HChipLayout.addWidget(self.ChipLabel, 0, 0, 1, 1)
        self.HChipLayout.addWidget(self.ChipVDDDLabel, 0, 1, 1, 1)
        self.HChipLayout.addWidget(self.ChipVDDDEdit, 0, 2, 1, 1)
        self.HChipLayout.addWidget(self.ChipVDDALabel, 0, 3, 1, 1)
        self.HChipLayout.addWidget(self.ChipVDDAEdit, 0, 4, 1, 1)
        self.HChipLayout.addWidget(self.ChipEfuseIDLabel, 0, 5, 1, 1)
        self.HChipLayout.addWidget(self.ChipEfuseIDEdit, 0, 6, 1, 1)

        return self.HChipLayout

    def makeChipBox(self, pChipID):
        self.ChipID = pChipID
        self.ChipLabel = QCheckBox("Chip ID: {0}".format(self.ChipID))
        self.ChipLabel.setChecked(True)
        self.ChipLabel.setObjectName("ChipStatus_{0}".format(pChipID))
        self.ChipVDDDLabel = QLabel("VDDD:")
        self.ChipVDDDEdit = QLineEdit()
        self.ChipVDDDEdit.setObjectName("VDDDEdit_{0}".format(pChipID))

        self.ChipVDDALabel = QLabel("VDDA:")
        self.ChipVDDAEdit = QLineEdit()
        self.ChipVDDAEdit.setObjectName("VDDAEdit_{0}".format(pChipID))
        self.ChipEfuseIDLabel = QLabel("Efuse ID:")
        self.ChipEfuseIDEdit = QLineEdit()
        self.ChipEfuseIDEdit.setObjectName("EfuseIDEdit_{0}".format(pChipID))

        if "CROC" in self.chipType:
            self.ChipVDDDEdit.setText("8")
            self.ChipVDDAEdit.setText("8")
        else:
            self.ChipVDDDEdit.setText("16")
            self.ChipVDDAEdit.setText("16")

        self.HChipLayout = QGridLayout()
        self.HChipLayout.addWidget(self.ChipLabel, 0, 0, 1, 1)
        self.HChipLayout.addWidget(self.ChipVDDDLabel, 0, 1, 1, 1)
        self.HChipLayout.addWidget(self.ChipVDDDEdit, 0, 2, 1, 1)
        self.HChipLayout.addWidget(self.ChipVDDALabel, 0, 3, 1, 1)
        self.HChipLayout.addWidget(self.ChipVDDAEdit, 0, 4, 1, 1)
        self.HChipLayout.addWidget(self.ChipEfuseIDLabel, 0, 5, 1, 1)
        self.HChipLayout.addWidget(self.ChipEfuseIDEdit, 0, 6, 1, 1)

        return self.HChipLayout

    def makeChipGroupBox(self, pChipGroupBoxDict):
        for key in pChipGroupBoxDict.keys():
            self.mainLayout.addLayout(pChipGroupBoxDict[key])
            self.mainLayout.addStretch(1)

    def getVDDA(self, pChipID):
        VDDAthing = self.findChild(QLineEdit, "VDDAEdit_{0}".format(pChipID))
        return VDDAthing.text()

    def getVDDD(self, pChipID):
        VDDDthing = self.findChild(QLineEdit, "VDDDEdit_{0}".format(pChipID))
        return VDDDthing.text()

    def getEfuseID(self, pChipID):
        efuseID = self.findChild(QLineEdit, "EfuseIDEdit_{0}".format(pChipID))
        return efuseID.text()

    def _get_fe_setting_default(self, key, fallback="0"):
        key_map = {
            "VREF": "VREF_ADC",
            "CINJ": "INJ_CAP",
        }
        source_key = key_map.get(key, key)

        value = FESettingsB.get(source_key)
        if value not in (None, ""):
            # Internal VREF convention here is volts; FESettings stores mV.
            if key == "VREF":
                try:
                    logger.debug(f"Using FE settings default for {key}: {value} mV (converted from FESettingsB)")
                    return str(float(value) / 1000.0)
                except Exception:
                    logger.debug(f"Failed to convert VREF value {value}, using fallback {fallback}")
                    return fallback
            logger.debug(f"Using FE settings default for {key}: {value} (from FESettingsB)")
            return str(value)

        logger.debug(f"No FE settings default found for {key}, using hardcoded fallback: {fallback}")
        return fallback

    def _get_chip_data_value(self, pChipID, key, default="0"):
        def resolve_default():
            return default() if callable(default) else default

        if not isinstance(self.chipData, dict):
            resolved_default = resolve_default()
            logger.debug(f"ChipID {pChipID}, {key}: No chip data available, using default: {resolved_default}")
            return resolved_default

        chip_entry = self.chipData.get(pChipID)
        if chip_entry is None:
            chip_entry = self.chipData.get(str(pChipID))
        if not isinstance(chip_entry, dict):
            resolved_default = resolve_default()
            logger.debug(f"ChipID {pChipID}, {key}: Chip not found in database, using default: {resolved_default}")
            return resolved_default

        if key not in chip_entry or chip_entry.get(key) is None:
            resolved_default = resolve_default()
            logger.debug(f"ChipID {pChipID}, {key}: Key not found in database, using default: {resolved_default}")
            return resolved_default

        value = chip_entry.get(key)
        
        logger.debug(f"ChipID {pChipID}, {key}: Retrieved value from database: {value}")
        return str(value)

    def getIREF(self, pChipID):
        return self._get_chip_data_value(
            pChipID, "IREF", lambda: self._get_fe_setting_default("IREF", "0")
        )
    
    def getVREF(self, pChipID):
        return self._get_chip_data_value(
            pChipID, "VREF", lambda: self._get_fe_setting_default("VREF", "0.8")
        )
    
    def getCINJ(self, pChipID):
        return self._get_chip_data_value(
            pChipID, "CINJ", lambda: self._get_fe_setting_default("CINJ", "8e-12")
        )

    def getDAC_PREAMP_L_LIN(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "DAC_PREAMP_L_LIN",
            lambda: self._get_fe_setting_default("DAC_PREAMP_L_LIN", "0"),
        )

    def getDAC_PREAMP_R_LIN(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "DAC_PREAMP_R_LIN",
            lambda: self._get_fe_setting_default("DAC_PREAMP_R_LIN", "0"),
        )

    def getDAC_PREAMP_TL_LIN(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "DAC_PREAMP_TL_LIN",
            lambda: self._get_fe_setting_default("DAC_PREAMP_TL_LIN", "0"),
        )

    def getDAC_PREAMP_TR_LIN(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "DAC_PREAMP_TR_LIN",
            lambda: self._get_fe_setting_default("DAC_PREAMP_TR_LIN", "0"),
        )

    def getDAC_PREAMP_T_LIN(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "DAC_PREAMP_T_LIN",
            lambda: self._get_fe_setting_default("DAC_PREAMP_T_LIN", "0"),
        )

    def getDAC_PREAMP_M_LIN(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "DAC_PREAMP_M_LIN",
            lambda: self._get_fe_setting_default("DAC_PREAMP_M_LIN", "0"),
        )

    def getDAC_REF_KRUM_LIN(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "DAC_REF_KRUM_LIN",
            lambda: self._get_fe_setting_default("DAC_REF_KRUM_LIN", "0"),
        )

    def getDAC_COMP_LIN(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "DAC_COMP_LIN",
            lambda: self._get_fe_setting_default("DAC_COMP_LIN", "0"),
        )
    
    def getDAC_COMP_TA_LIN(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "DAC_COMP_TA_LIN",
            lambda: self._get_fe_setting_default("DAC_COMP_TA_LIN", "0"),
        )

    def getDAC_LDAC_LIN(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "DAC_LDAC_LIN",
            lambda: self._get_fe_setting_default("DAC_LDAC_LIN", "0"),
        )
    
    def getADC_OFFSET_VOLT(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "ADC_OFFSET_VOLT",
            lambda: self._get_fe_setting_default("ADC_OFFSET_VOLT", "0"),
        )

    def getADC_MAXIMUM_VOLT(self, pChipID):
        return self._get_chip_data_value(
            pChipID,
            "ADC_MAXIMUM_VOLT",
            lambda: self._get_fe_setting_default("ADC_MAXIMUM_VOLT", "0"),
        )

    def getChipData(self):
        return self.chipData

    def getTrimValues(self):
        return self.trimValues

    def getChipStatus(self, pChipID):
        ChipCheckBox = self.findChild(QCheckBox, "ChipStatus_{0}".format(pChipID))
        ChipStatus = ChipCheckBox.isChecked()
        return ChipStatus

    def fetchNameLabel(self, moduleName): # Step 1: get the name label and module type (dual or quad)
        name_label, moduleType = module_registry_client.find_module_identity(moduleName)
        if name_label and moduleType:
            logger.debug(
                f"found module {moduleName} in {moduleType} registry with name label {name_label}"
            )
            print(f"found module {moduleName} in {moduleType} registry with name label {name_label}")
            return name_label, moduleType
        
        # If not found in CMS registry, return None to trigger fallback
        return None, None
    
    def _tryPurdueDatabase(self, moduleName):
        """Helper method: Try to fetch from Purdue database (backup)."""
        try:
            response = module_registry_client.get_purdue_response(moduleName)
            logger.debug(f"Purdue database response available for {moduleName}")
            return response
        except Exception as e:
            logger.warning(f"Failed to access Purdue database: {e}")
            return None


    def fetchHDIVersionFromDB(self, moduleName):
        name_label, moduleType = self.fetchNameLabel(moduleName)
        
        if name_label:
            # Try CMS database first
            try:
                data = module_registry_client.get_module_json(name_label)

                for entry in data.get("bare_module_data", []):
                    version = entry.get("VERSION")
                    if version is not None:
                        try:
                            vstr = _normalize_hdi_version(version, default=str(version).strip())
                            logger.info(f"HDI version from CMS database: {vstr}")
                            return vstr
                        except Exception:
                            return str(version).strip()

                logger.warning(f"HDI version not found for {name_label} in CMS database. Trying Purdue...")
            except Exception as e:
                logger.warning(f"Failed to fetch HDI version from CMS database: {e}. Trying Purdue...")
        
        # Fallback to Purdue database
        try:
            response = self._tryPurdueDatabase(moduleName)
            if response:
                html_content = response.text
                match = re.search(r"Version\s*=\s*(.+)", html_content)
                if match:
                    hdiversion = match.group(1).strip()
                    logger.info(f"HDI version from Purdue database: {hdiversion}")
                    return hdiversion
                else:
                    logger.warning("HDI version not found in Purdue database. Using default value of 1.")
                    return "1"
        except Exception as e:
            logger.warning(f"Failed to fetch from Purdue database: {e}")
        
        print("Warning: HDI version not found for module. Using default value of 1.")
        return "1"
        

    ## This function returns a list of dictionaries.  Each element of the list is a chip dictinary.
    def fetchChipDataFromDB(self, moduleName):
        module_name_key = moduleName.strip().upper() if moduleName else ""
        if module_name_key in chip_data_cache:
            logger.debug(f"Using cached chip data for {moduleName}")
            return chip_data_cache[module_name_key]

        name_label, moduleType = self.fetchNameLabel(moduleName)
        
        if name_label and moduleType:
            # Try CMS database first
            try:
                data = module_registry_client.get_module_json(name_label)

                if moduleType == "quad":
                    chipidmap = {"0": "12", "1": "13", "2": "14", "3": "15"}
                else:
                    chipidmap = {"1": "12", "0": "13"}

                chipdatadicts = [entry for entry in data["bare_module_data"] if entry["KIND_OF_PART"] == "CROC Chip"]

                chipdata = {}
                logger.info("Converting the units of chip data")
                for i, chip in enumerate(chipdatadicts):
                    chipdata[chipidmap[str(i)]] = {
                    "VDDA": str(chip.get("VDDA_TRIM_CODE", "0")),
                    "VDDD": str(chip.get("VDDD_TRIM_CODE", "0")),
                    "IREF": str(chip.get("IREF_TRIM_CODE", "0")),
                    "EFUSE": str(chip.get("EFUSE_CODE", "0")),
                    "VREF": str(chip.get("VREF_ADC_V", "0")),
                    "CINJ": str(chip.get("INJ_CAPACIT_F", "8e-12")),
                    "ADC_OFFSET_VOLT": str(1e4*float(chip.get("ADC_OFF_V", "0"))),
                    "ADC_MAXIMUM_VOLT": str(1e3*(4096*float(chip.get("ADC_SLO", "0"))+float(chip.get("ADC_OFF_V", "0")))),
                    "DAC_PREAMP_L_LIN": str(chip.get("probe_data", {}).get("DAC_PREAMP_L_LIN", "0")),
                    "DAC_PREAMP_R_LIN": str(chip.get("probe_data", {}).get("DAC_PREAMP_R_LIN", "0")),
                    "DAC_PREAMP_TL_LIN": str(chip.get("probe_data", {}).get("DAC_PREAMP_TL_LIN", "0")),
                    "DAC_PREAMP_TR_LIN": str(chip.get("probe_data", {}).get("DAC_PREAMP_TR_LIN", "0")),
                    "DAC_PREAMP_T_LIN": str(chip.get("probe_data", {}).get("DAC_PREAMP_T_LIN", "0")),
                    "DAC_PREAMP_M_LIN": str(chip.get("probe_data", {}).get("DAC_PREAMP_M_LIN", "0")),
                    "DAC_REF_KRUM_LIN": str(chip.get("probe_data", {}).get("DAC_REF_KRUM_LIN", "0")),
                    "DAC_COMP_LIN": str(chip.get("probe_data", {}).get("DAC_COMP_LIN", "0")),
                    "DAC_COMP_TA_LIN": str(chip.get("probe_data", {}).get("DAC_COMP_TA_LIN", "0")),
                    "DAC_LDAC_LIN": str(chip.get("probe_data", {}).get("DAC_LDAC_LIN", "0")),
                    }
                logger.debug(f"Fetched chip data for {name_label} from CMS database: {chipdata}")
                if module_name_key:
                    chip_data_cache[module_name_key] = chipdata
                return chipdata
            except Exception as e:
                logger.warning(f"Failed to fetch chip data from CMS database: {e}. Trying Purdue...")
        
        # Fallback to Purdue database
        try:
            response = self._tryPurdueDatabase(moduleName)
            if response:
                parser = etree.HTMLParser()
                tree = etree.fromstring(response.content, parser)
                chip_table = tree.xpath("//body/table")[0]
                
                chipidmap = {}
                chipidmap["0"] = "12"
                chipidmap["1"] = "13"
                chipidmap["2"] = "14"
                chipidmap["3"] = "15"

                chipdatalist = []
                for row in chip_table:
                    elementdata = []
                    for element in row:
                        elementdata.append(element.text)
                    chipdatalist.append(elementdata)
                chipdatadicts = [
                    dict(zip(chipdatalist[0], values)) for values in chipdatalist[1:]
                ]
                chipdata = {}
                for i, chip in enumerate(chipdatadicts):
                    chipdata[chipidmap[str(i)]] = ExtractChipData(chip["S/N"])
                    logger.debug(f"Extracted chip data for {chip['S/N']} from Purdue database: {chipdata[chipidmap[str(i)]]}")
                    
                logger.debug(f"Fetched chip data for {moduleName} from Purdue database: {chipdata}")
                if module_name_key:
                    chip_data_cache[module_name_key] = chipdata
                return chipdata
        except Exception as e:
            logger.warning(f"Failed to fetch from Purdue database: {e}")
        
        logger.error(f"Could not fetch chip data from either database for {moduleName}")
        return None

    def fetchTrimFromDB(self, moduleName):
        name_label, moduleType = self.fetchNameLabel(moduleName)
        
        if name_label and moduleType:
            # Try CMS database first
            try:
                data_json = module_registry_client.get_module_json(name_label)

                if moduleType == "quad":
                    chipidmap = {"0": "12", "1": "13", "2": "14", "3": "15"}
                else:
                    chipidmap = {"1": "12", "0": "13"}

                data = {}

                for entry in data_json["bare_module_data"]:
                    if entry["KIND_OF_PART"] == "CROC Chip":
                        slot = str(entry["ACROC_SLOT"])

                        data[chipidmap[slot]] = {
                            "VDDD": str(entry["VDDD_TRIM_CODE"]),
                            "VDDA": str(entry["VDDA_TRIM_CODE"]),
                            "IREF": str(entry["IREF_TRIM_CODE"]),
                            "EFUSE": str(entry["EFUSE_CODE"]),
                            "VREF": str(entry["VREF_ADC_V"]),
                            "CINJ": str(entry["INJ_CAPACIT_F"]),
                            "ADC_OFFSET_VOLT": str(entry["ADC_OFF_V"]),
                            "ADC_MAXIMUM_VOLT": str(entry["ADC_SLO"]),
                            "DAC_PREAMP_L_LIN": str(entry["DAC_PREAMP_L_LIN"]),
                            "DAC_PREAMP_R_LIN": str(entry["DAC_PREAMP_R_LIN"]),
                            "DAC_PREAMP_TL_LIN": str(entry["DAC_PREAMP_TL_LIN"]),
                            "DAC_PREAMP_TR_LIN": str(entry["DAC_PREAMP_TR_LIN"]),
                            "DAC_PREAMP_T_LIN": str(entry["DAC_PREAMP_T_LIN"]),
                            "DAC_PREAMP_M_LIN": str(entry["DAC_PREAMP_M_LIN"]),
                            "DAC_REF_KRUM_LIN": str(entry["DAC_REF_KRUM_LIN"]),
                            "DAC_COMP_LIN": str(entry["DAC_COMP_LIN"]),
                            "DAC_COMP_TA_LIN": str(entry["DAC_COMP_TA_LIN"]),
                            "DAC_LDAC_LIN": str(entry["DAC_LDAC_LIN"]),
                        }

                for chipID in ["12", "13", "14", "15"]:
                    if chipID not in data:
                        data[chipID] = {
                            "VDDD": "0",
                            "VDDA": "0",
                            "IREF": "0",
                            "EFUSE": "0",
                            "VREF": "0",
                            "CINJ": "0",
                            "DAC_PREAMP_L_LIN": "0",
                            "DAC_PREAMP_R_LIN": "0",
                            "DAC_PREAMP_TL_LIN": "0",
                            "DAC_PREAMP_TR_LIN": "0",
                            "DAC_PREAMP_T_LIN": "0",
                            "DAC_PREAMP_M_LIN": "0",
                            "DAC_REF_KRUM_LIN": "0",
                            "DAC_COMP_LIN": "0",
                            "DAC_COMP_TA_LIN": "0",
                            "DAC_LDAC_LIN": "0",
                            "ADC_OFFSET_VOLT": "0",
                            "ADC_MAXIMUM_VOLT": "0",
                        }

                logger.info(f"Fetched trim data for {name_label} from CMS database")
                return data
            except Exception as e:
                logger.warning(f"Failed to fetch trim data from CMS database: {e}. Trying Purdue...")
        
        # Fallback to Purdue database
        try:
            response = self._tryPurdueDatabase(moduleName)
            if response:
                parser = etree.HTMLParser()
                tree = etree.fromstring(response.content, parser)
                chip_table = tree.xpath("//body/table")[0]

                values = []
                for row in chip_table[1:]:
                    for element in row:
                        if element.text and element.text.startswith("U1"):
                            values.append([])

                        elif element.text and element.text.isdigit():
                            values[-1].append(element.text)

                data = {}
                for i in range(len(values)):
                    data[str(i + 12)] = {
                        "VDDD": values[i][1],
                        "VDDA": values[i][2],
                    }
                
                logger.info(f"Fetched trim data for {moduleName} from Purdue database")
                return data
        except Exception as e:
            logger.warning(f"Failed to fetch from Purdue database: {e}")
        
        logger.error(f"Could not fetch trim data from either database for {moduleName}")
        return None


class BeBoardBox(QWidget):
    changed = pyqtSignal()

    def __init__(self, master, firmware):
        super(BeBoardBox, self).__init__()
        self.master = master
        self.firmware = firmware
        self.ModuleList = []
        self.ChipWidgetDict = {}
        self.ChipWidgetKeyDict = {}
        self._serial_lookup_generation = 0
        self.mainLayout = QVBoxLayout()  # Use QVBoxLayout for vertical layout
        self._focus_after_update = None  # Track which module should receive focus after updateList

        self.initList()
        self.createList()

        scrollArea = QScrollArea()  # Create a scroll area
        scrollContent = QWidget()
        scrollContent.setLayout(self.mainLayout)  # Set mainLayout to scrollable content
        scrollArea.setWidget(scrollContent)
        scrollArea.setWidgetResizable(True)
        scrollArea.setVerticalScrollBarPolicy(
            Qt.ScrollBarAlwaysOn
        )  # Ensure the scrollbar is always visible
        scrollArea.setHorizontalScrollBarPolicy(
            Qt.ScrollBarAlwaysOff
        )  # Hide the horizontal scrollbar

        mainLayout = QVBoxLayout()
        mainLayout.addWidget(scrollArea)

        self.setLayout(mainLayout)  # Set mainLayout as the layout for BeBoardBox

        self.setGeometry(
            100, 100, 800, 600
        )  # Set initial geometry (x, y, width, height)
        self.setMinimumSize(900, 300)  # Set minimum size (width, height)

    def initList(self):
        ModuleRow = ModuleBox(self.firmware)
        self.ModuleList.append(ModuleRow)
        ModuleRow.TypeCombo.currentTextChanged.connect(self.updateList)
        ModuleRow.VersionCombo.currentTextChanged.connect(self.updateList)
        ModuleRow.HDIVersionCombo.currentTextChanged.connect(self.updateList)
        ModuleRow.SerialEdit.editingFinished.connect(
            self.createSerialUpdateCallback(ModuleRow)
        )

    def createList(self):
        self.ListLayout = QGridLayout()
        self.ListLayout.setVerticalSpacing(0)
        self.updateList()

        self.ListBox = QGroupBox()
        self.ListBox.setLayout(self.ListLayout)
        self.mainLayout.addWidget(self.ListBox)

    def deleteList(self):
        self.ListBox.deleteLater()
        self.mainLayout.removeWidget(self.ListBox)

    @debounce(100)
    def updateList(self, *args):
        focus_ctx = self._capture_focus_context()
        serialNumberWidgets = []
        chipIDWidgets = []

        active_modules = set(self.ModuleList)
        for cached_module in list(self.ChipWidgetDict.keys()):
            if cached_module not in active_modules:
                self.ChipWidgetDict.pop(cached_module, None)
                self.ChipWidgetKeyDict.pop(cached_module, None)

        # Clear existing layout
        for i in reversed(range(self.ListLayout.count())):
            widget = self.ListLayout.itemAt(i).widget()
            if widget:
                self.ListLayout.removeWidget(widget)
                widget.setParent(None)

        for index, module in enumerate(self.ModuleList):
            if index == 0 and "CROC" not in module.TypeCombo.currentText():
                module.VersionCombo.setCurrentText("v1")
                module.VersionCombo.setDisabled(True)
                module.TypeCombo.currentTextChanged.connect(self.updateList)
            elif index == 0:
                module.TypeCombo.currentTextChanged.connect(self.updateList)
                if "sh" in module.getSerialNumber().lower() or "rh" in module.getSerialNumber().lower():
                    numpart = "".join(filter(str.isdigit, module.getSerialNumber()))
                    if numpart.isdigit() and int(numpart) > 49:
                        module.VersionCombo.setCurrentText("v2")
                    else:
                        module.VersionCombo.setCurrentText("v1")

                module.VersionCombo.currentTextChanged.connect(self.updateList)
                module.VersionCombo.setDisabled(False)
            if index != 0:
                module.TypeCombo.setCurrentText(
                    self.ModuleList[0].TypeCombo.currentText()
                )
                module.TypeCombo.setDisabled(True)
                module.VersionCombo.setCurrentText(
                    self.ModuleList[0].VersionCombo.currentText()
                )
                module.VersionCombo.setDisabled(True)
                module.SerialEdit.editingFinished.connect(
                    self.createSerialUpdateCallback(module)
                )

            chip_key = (module.getSerialNumber(), module.getType())
            cached_key = self.ChipWidgetKeyDict.get(module)
            if module in self.ChipWidgetDict and cached_key == chip_key:
                chipBox = self.ChipWidgetDict[module]
            else:
                chipBox = ChipBox(self.master, module.getType(), module.getSerialNumber())
                self.ChipWidgetDict[module] = chipBox
                self.ChipWidgetKeyDict[module] = chip_key
            module.setMaximumHeight(50)

            serialNumberWidgets.append(module)
            chipIDWidgets.append(chipBox)

        # Add serial number widgets
        for index, widget in enumerate(serialNumberWidgets):
            self.ListLayout.addWidget(widget, index, 0, 1, 1)

        # Add chip ID widgets
        for index, widget in enumerate(chipIDWidgets):
            self.ListLayout.addWidget(widget, index + len(serialNumberWidgets), 0, 1, 1)

        # Add remove and add buttons
        for index, module in enumerate(self.ModuleList):
            if index == 0:
                continue  # no remove button for the first module
            removeButton = QPushButton("Remove")
            removeButton.setMaximumWidth(150)
            removeButton.clicked.connect(lambda checked, m=module: self.removeModule(m))
            self.ListLayout.addWidget(removeButton, index, 1, 1, 1)

        newButton = QPushButton("Add")
        newButton.setMaximumWidth(150)
        newButton.clicked.connect(self.addModule)
        self.ListLayout.addWidget(newButton, len(self.ModuleList), 1, 1, 1)
        self.update()
        self._restore_focus_context(focus_ctx)
        
        #set module focus
        if self._focus_after_update is not None:
            target_module = self._focus_after_update
            self._focus_after_update = None
            QTimer.singleShot(0, target_module.SerialEdit.setFocus)

        


    def createSerialUpdateCallback(self, module):
        return lambda: self.onSerialNumberUpdate(module)

    # Mapping of field names to widget attribute names
    _FOCUS_FIELD_MAP = {
        "serial": "SerialEdit",
        "port": "PortEdit",
        "type": "TypeCombo",
        "fc7": "FC7Combo",
        "version": "VersionCombo",
        "hdi": "HDIVersionCombo",
    }

    def _capture_focus_context(self):
        focus = QApplication.focusWidget()
        if focus is None:
            return None
        
        for index, module in enumerate(self.ModuleList):
            for field_name, attr_name in self._FOCUS_FIELD_MAP.items():
                if focus is getattr(module, attr_name):
                    return (field_name, index)
        return None

    def _restore_focus_context(self, focus_ctx):
        if not focus_ctx:
            return

        field, index = focus_ctx
        if index < 0 or index >= len(self.ModuleList):
            return

        module = self.ModuleList[index]
        attr_name = self._FOCUS_FIELD_MAP.get(field)
        if attr_name:
            target = getattr(module, attr_name, None)
            if target is not None:
                QTimer.singleShot(0, target.setFocus)

    def _tryPurdueDatabase(self, moduleName):
        """Helper method: Try to fetch from Purdue database (backup)."""
        try:
            response = module_registry_client.get_purdue_response(moduleName)
            logger.debug(f"Purdue database response available for {moduleName}")
            return response
        except Exception as e:
            logger.warning(f"Failed to access Purdue database: {e}")
            return None

    @debounce(500)
    def onSerialNumberUpdate(self, module):
        serial_number = module.getSerialNumber()
        if not serial_number:
            return

        self._serial_lookup_generation += 1
        lookup_generation = self._serial_lookup_generation

        def _apply_module_type(data):
            if lookup_generation != self._serial_lookup_generation:
                return

            if data:
                if module.TypeCombo.isEnabled():
                    module.TypeCombo.setCurrentText(data["type"])
                if module.HDIVersionCombo.isEnabled():
                    module.HDIVersionCombo.setCurrentText(data["HDIversion"])
                    print('returning hdi version {0}'.format(data["HDIversion"]))

                self.updateList()

        def _handle_lookup_error(error_message):
            if lookup_generation != self._serial_lookup_generation:
                return
            logger.warning(
                f"Asynchronous module lookup failed for {serial_number}: {error_message}"
            )

        module_registry_client.fetch_module_type_async(
            serial_number,
            on_success=_apply_module_type,
            on_error=_handle_lookup_error,
        )

    def fetchModuleTypeDB(self, moduleName):
        try:
            data = _fetch_module_type_from_databases(moduleName)
            if data:
                return data

            # If we get here, neither database has the module
            logger.warning(f"Could not find {moduleName} in either CMS or Purdue database")
            msg = QMessageBox()
            msg.information(
                None,
                "Error",
                f"Could not find {moduleName} in the module registries.",
                QMessageBox.Ok,
            )
            return None

        except Exception:
            logger.error(traceback.format_exc())
            self.master.purdue_connected = False
            return None

    def removeModule(self, module):
        self.ModuleList.remove(module)
        self.ChipWidgetDict.pop(module, None)
        self.ChipWidgetKeyDict.pop(module, None)
        module.deleteLater()
        self.updateList()
        self.changed.emit()

    def addModule(self):
        module = ModuleBox(self.firmware)
        module.TypeCombo.currentTextChanged.connect(self.updateList)
        self.ModuleList.append(module)
        # Mark this module to receive focus after updateList completes
        self._focus_after_update = module
        self.updateList()
        self.changed.emit()

    def getModules(self):
        return self.ModuleList

    def getFirmwareDescription(self):
        module_types = []
        for module in self.ModuleList:
            # Access the currently selected QtBeBoard object
            BeBoard = None
            for board in self.firmware:
                if board.getBoardName() == module.getFC7():
                    BeBoard = board
            if BeBoard is None:
                raise Exception("There are no FC7s active.")

            # Access the currently selected QtOpticalGroup of the QtBeBoard
            OpticalGroup = None
            for og in BeBoard.getAllOpticalGroups().values():
                if og.getFMCID() == module.getFMCID():
                    OpticalGroup = og

            # Create it if it doesn't already exist
            if OpticalGroup is None:
                OpticalGroup = QtOpticalGroup(FMCID=module.getFMCID())
                OpticalGroup.setBeBoard(
                    BeBoard
                )  # Ignore this line, see explanation in Firmware.py
                try:
                    BeBoard.addOpticalGroup(
                        FMCID=module.getFMCID(), OpticalGroup=OpticalGroup
                    )
                except KeyError as e:
                    logger.error(traceback.format_exc())
                    return (
                        None,
                        f"Error while adding Optical Group to BeBoard: {repr(e)}",
                    )

            # Create a QtModule object based on the input data
            Module = QtModule(
                moduleName=module.getSerialNumber(),
                moduleType=module.getType(),
                moduleVersion=module.getVersion(),
                hdiVersion=module.getHDIVersion(),
                FMCPort=module.getFMCPort(),
            )
            Module.setOpticalGroup(
                OpticalGroup
            )  # Ignore this line, see explanation in Firmware.py

            # Pull VDDA/VDDD trim and chip status from the ChipBox on the StartWindow.
            for chipID in ModuleLaneMap_Dict["HDIv{0}".format(module.getHDIVersion())][module.getType()].values():
                Module.getChips()[chipID].setStatus(
                    self.ChipWidgetDict[module].getChipStatus(chipID)
                )
                Module.getChips()[chipID].setVDDA(
                    self.ChipWidgetDict[module].getVDDA(chipID)
                )
                Module.getChips()[chipID].setVDDD(
                    self.ChipWidgetDict[module].getVDDD(chipID)
                )
                Module.getChips()[chipID].setEfuseID(
                    self.ChipWidgetDict[module].getEfuseID(chipID)
                )
                Module.getChips()[chipID].setIREF(
                    self.ChipWidgetDict[module].getIREF(chipID)
                )
                Module.getChips()[chipID].setDAC_PREAMP_L_LIN(
                    self.ChipWidgetDict[module].getDAC_PREAMP_L_LIN(chipID)
                )
                Module.getChips()[chipID].setDAC_PREAMP_R_LIN(
                    self.ChipWidgetDict[module].getDAC_PREAMP_R_LIN(chipID)
                )
                Module.getChips()[chipID].setDAC_PREAMP_TL_LIN(
                    self.ChipWidgetDict[module].getDAC_PREAMP_TL_LIN(chipID)
                )
                Module.getChips()[chipID].setDAC_PREAMP_TR_LIN(
                    self.ChipWidgetDict[module].getDAC_PREAMP_TR_LIN(chipID)
                )
                Module.getChips()[chipID].setDAC_PREAMP_T_LIN(
                    self.ChipWidgetDict[module].getDAC_PREAMP_T_LIN(chipID)
                )
                Module.getChips()[chipID].setDAC_PREAMP_M_LIN(
                    self.ChipWidgetDict[module].getDAC_PREAMP_M_LIN(chipID)
                )
                Module.getChips()[chipID].setDAC_REF_KRUM_LIN(
                    self.ChipWidgetDict[module].getDAC_REF_KRUM_LIN(chipID)
                )
                Module.getChips()[chipID].setDAC_COMP_LIN(
                    self.ChipWidgetDict[module].getDAC_COMP_LIN(chipID)
                )
                Module.getChips()[chipID].setDAC_COMP_TA_LIN(
                    self.ChipWidgetDict[module].getDAC_COMP_TA_LIN(chipID)
                )
                Module.getChips()[chipID].setDAC_LDAC_LIN(
                    self.ChipWidgetDict[module].getDAC_LDAC_LIN(chipID)
                )
                Module.getChips()[chipID].setADC_MAXIMUM_VOLT(
        
                    self.ChipWidgetDict[module].getADC_MAXIMUM_VOLT(chipID)
                )
                Module.getChips()[chipID].setADC_OFFSET_VOLT(
                    self.ChipWidgetDict[module].getADC_OFFSET_VOLT(chipID)
                )
                
                #try:
                #    vref_value = float(self.ChipWidgetDict[module].getVREF(chipID))
                #except Exception:
                #    vref_value = 0.8
                    
                Module.getChips()[chipID].setVREF(
                    self.ChipWidgetDict[module].getVREF(chipID)
                )

                #try:
                #    cinj_value = float(self.ChipWidgetDict[module].getCINJ(chipID))
                #except Exception:
                #    cinj_value = 8
                Module.getChips()[chipID].setCINJ(
                    self.ChipWidgetDict[module].getCINJ(chipID)
                )

            # Add the QtModule object to the currently selected Optical Group
            try:
                OpticalGroup.addModule(FMCPort=module.getFMCPort(), module=Module)
            except KeyError as e:
                logger.error(traceback.format_exc())
                if module.getFMCPort() in OpticalGroup.getAllModules():
                    OpticalGroup.removeModuleByIndex(module.getFMCPort())

                return None, f"Error while adding Module to Optical Group: {repr(e)}"

            module_types.append(
                Module.getModuleType() + " " + Module.getModuleVersion()
            )

        if not all([i == module_types[0] for i in module_types]):
            # iterate over module_types, if they're not all identical, return None
            return (
                None,
                "All modules must be of the same type! Please ensure you have entered the module data correctly.",
            )

        # only include the board if there are connected modules, otherwise ignore it
        ret = []
        for board in self.firmware:
            if (
                len(board.getAllOpticalGroups()) != 0
            ):  # If modules are connected to the board
                board.setBoardID(len(ret))
                ret.append(board)

        if ret == list():  # nothing added to ret -> no connected modules
            return None, "No valid module found!"
        else:
            return ret, "Success"

    # def getVDDA(self, module):
    #   VDDAdict = {}
    #   for key in self.ChipWidgetDict.keys():
    #           VDDAdict[key] = self.ChipWidgetDict[key].getVDDA()
    #   return VDDAdict


class StatusBox(QWidget):
    def __init__(self, verbose, index=0):
        super(StatusBox, self).__init__()
        self.index = index
        self.verbose = verbose
        self.mainLayout = QGridLayout()
        self.createBody()
        self.checkFwPar()
        self.setLayout(self.mainLayout)

    def createBody(self):
        FEIDLabel = QLabel("ID: {}".format(self.index))
        FEIDLabel.setStyleSheet("font-weight:bold")

        self.LabelList = []
        self.EditList = []

        for i, (key, value) in enumerate(self.verbose.items()):
            Label = QLabel()
            Label.setText(str(key) + ":")
            Edit = QLineEdit()
            Edit.setText(str(value))
            Edit.setDisabled(True)
            self.LabelList.append(Label)
            self.EditList.append(Edit)

        self.CheckLabel = QLabel()

        self.mainLayout.addWidget(FEIDLabel, 0, 0, 1, 1)

        for index in range(len(self.LabelList)):
            self.mainLayout.addWidget(self.LabelList[index], index + 1, 0, 1, 1)
            self.mainLayout.addWidget(self.EditList[index], index + 1, 1, 1, 1)

    def checkFwPar(self):
        return True
        """
                self.CheckLabel.setStyleSheet("color:red")
                PowerMode = str(self.PowerModeCombo.currentText())
                if not str(self.ANLVoltEdit.text()) or not str(self.DIGVoltEdit.text()) or not str(self.ANLAmpEdit.text()) or not str(self.DIGAmpEdit.text()):
                        self.CheckLabel.setText("V/I measure is missing")
                        return False

                comment = ''
                try:
                        if float(str(self.ANLVoltEdit.text())) < FEPowerUpVA[PowerMode][0] or float(str(self.ANLVoltEdit.text())) > FEPowerUpVA[PowerMode][1]:
                                comment +=  "Analog Voltage range: {}".format(FEPowerUpVA[PowerMode])
                        if float(str(self.DIGVoltEdit.text())) < FEPowerUpVD[PowerMode][0] or float(str(self.DIGVoltEdit.text())) > FEPowerUpVD[PowerMode][1]:
                                comment +=  "Digital Voltage range: {}".format(FEPowerUpVA[PowerMode])
                        #if math.fabs( float(str(self.AmpEdit.text())) - float(str(self.ANLAmpEdit.text())) - float(str(self.DIGAmpEdit.text())) ) > 0.1:
                        #       comment += "Current from Source deviated from measured current in module"
                        ChipAmp = float(str(self.ANLAmpEdit.text())) + float(str(self.DIGAmpEdit.text()))
                        if ChipAmp  < FEPowerUpAmp[PowerMode][0] or ChipAmp > FEPowerUpAmp[PowerMode][1]:
                                comment +=  "Amp range: {}".format(FEPowerUpAmp[PowerMode])
                except ValueError:
                        comment = "Not valid input"

                if comment == '':
                        comment = "Ok"
                        self.CheckLabel.setText(comment)
                        self.CheckLabel.setStyleSheet("color:green")
                        return True
                else:
                        self.CheckLabel.setText(comment)
                        return False
                """


class SimpleModuleBox(QWidget):
    typechanged = pyqtSignal()
    textchanged = pyqtSignal()
    destroy = pyqtSignal()

    def __init__(self):
        super(SimpleModuleBox, self).__init__()
        self.SerialString = None
        self.mainLayout = QGridLayout()
        self.createRow()
        self.setLayout(self.mainLayout)

    def createRow(self):
        SerialLabel = QLabel("SerialNumber:")
        self.SerialEdit = QLineEdit()
        self.SerialEdit.setMinimumWidth(55)
        self.SerialEdit.returnPressed.connect(self.on_editing_finished)

        CableIDLabel = QLabel("Cable ID:")
        self.CableIDEdit = QLineEdit()
        self.CableIDEdit.textChanged.connect(self.on_TypeChanged)
        self.CableIDEdit.setReadOnly(True)

        self.mainLayout.addWidget(SerialLabel, 0, 0)
        self.mainLayout.addWidget(self.SerialEdit, 0, 1)
        self.mainLayout.addWidget(CableIDLabel, 1, 0)
        self.mainLayout.addWidget(self.CableIDEdit, 1, 1)

    def setSerialNumber(self, serial):
        self.SerialEdit.setText(serial)

    def getSerialNumber(self):
        if not self.SerialEdit.text():  # case for nothing is inside serial box
            return None
        else:
            return self.SerialEdit.text()

    def setID(self, laneId):
        self.CableIDEdit.setText(str(laneId))

    def setHDIversion(self, hdiVersion: str):
        self.HDIversion = hdiVersion.split(".")[0]

    def getID(self):
        return self.CableIDEdit.text()

    def setType(self, typeStr):
        self.Type = typeStr

    def getType(self, SerialNumber):
        if "zh" in SerialNumber.lower():
            self.Type = "TFPX RD53A Quad"
        elif "rh" in SerialNumber.lower():
            self.Type = "TFPX CROC 1x2"
        elif "sh" in SerialNumber.lower():
            self.Type = "TFPX CROC Quad"
        return self.Type


    def setVersion(self, versionStr):
        self.version = versionStr

    def getVersion(self, SerialNumber):
        numpart = "".join(filter(str.isdigit, SerialNumber))
        if "sh" in SerialNumber.lower() or "rh" in SerialNumber.lower():
            if numpart.isdigit() and int(numpart) > 49:
                self.version = "v2"
            else:
                self.version = "v1"

        else:
            self.version = "v1"
        return self.version


    @QtCore.pyqtSlot()
    def on_TypeChanged(self):
        self.typechanged.emit()

    @QtCore.pyqtSlot()
    def on_textChange(self):
        self.SerialString = self.SerialEdit.text()
        self.textchanged.emit()

    @QtCore.pyqtSlot()
    def on_editing_finished(self):
        self.SerialString = self.SerialEdit.text()
        self.textchanged.emit()


class SimpleBeBoardBox(QWidget):
    changed = pyqtSignal()

    def __init__(self, master, firmware):
        super(SimpleBeBoardBox, self).__init__()
        self.master = master
        self.firmware = firmware
        self.ModuleList = []
        self.FilledModuleList = []
        self.BufferBox = None
        self.mainLayout = QGridLayout()
        self.currentModule = -1

        self.initList()
        self.createList()

        self.setLayout(self.mainLayout)
        scrollArea = QScrollArea()  # Create a scroll area
        scrollContent = QWidget()
        scrollContent.setLayout(self.mainLayout)  # Set mainLayout to scrollable content
        scrollArea.setWidget(scrollContent)
        scrollArea.setWidgetResizable(True)
        scrollArea.setVerticalScrollBarPolicy(
            Qt.ScrollBarAlwaysOn
        )  # Ensure the scrollbar is always visible
        scrollArea.setHorizontalScrollBarPolicy(
            Qt.ScrollBarAlwaysOff
        )  # Hide the horizontal scrollbar

        mainLayout = QVBoxLayout()
        mainLayout.addWidget(scrollArea)

        self.setLayout(mainLayout)  # Set mainLayout as the layout for BeBoardBox

        self.setGeometry(
            100, 100, 800, 600
        )  # Set initial geometry (x, y, width, height)

    def initList(self):
        ModuleRow = SimpleModuleBox()
        self.ModuleList.append(ModuleRow)
        self.ModuleList[-1].SerialEdit.setFocus()

    def createList(self):
        logger.debug(f"{__name__} : Creating module list")
        self.ListBox = QGroupBox()

        self.ListLayout = QGridLayout()
        self.ListLayout.setVerticalSpacing(0)

        self.updateList()

        self.ListBox.setLayout(self.ListLayout)
        self.mainLayout.addWidget(self.ListBox, 0, 0)

    def deleteList(self):
        self.ListBox.deleteLater()
        self.mainLayout.removeWidget(self.ListBox)

    def updateList(self):
        logger.debug(f"{__name__} : Updating module list")
        [columns, rows] = [self.ListLayout.columnCount(), self.ListLayout.rowCount()]

        for i in range(columns):
            for j in range(rows):
                item = self.ListLayout.itemAtPosition(j, i)
                if item:
                    widget = item.widget()
                    self.ListLayout.removeWidget(widget)
        logger.debug(f"{__name__} : Before connecting module signals")
        for index, module in enumerate(self.ModuleList):
            # module.setMaximumWidth(500)
            module.setMaximumHeight(50)
            module.typechanged.connect(self.on_TypeChanged)
            module.textchanged.connect(self.on_ModuleFilled)
            module.setID(index)
            
            self.ListLayout.addWidget(module, index, 0, 1, 1)
        logger.debug(f"{__name__} : After connecting module signals")
        NewButton = QPushButton("add")
        NewButton.setMaximumWidth(150)
        NewButton.clicked.connect(self.addModule)

        # For QR code, remove all modules
        ClearButton = QPushButton("Clear")
        ClearButton.setMaximumWidth(150)
        ClearButton.clicked.connect(self.clearModule)

        # self.ListLayout.addWidget(NewButton,len(self.ModuleList),1,1,1)
        self.ListLayout.addWidget(ClearButton, len(self.ModuleList), 0, 1, 1)
        self.update()
        logger.debug(f"{__name__} : Finished setting up module list")




    def removeModule(self, index):
        # For Manual change
        self.ModuleList.pop(index)

        # For QR SCAN
        # self.ModuleList = []
        # self.initList()

        if str(sys.version).startswith("3.8"):
            self.deleteList()
            self.createList()
        elif str(sys.version).startswith(("3.7", "3.9")):
            self.updateList()
        else:
            self.updateList()
        self.changed.emit()

    def clearModule(self):
        self.ModuleList = [SimpleModuleBox()]
        self.FilledModuleList = []
        for beboard in self.firmware:
            beboard.removeModules()

        if str(sys.version).startswith("3.8"):
            self.deleteList()
            self.createList()
        elif str(sys.version).startswith(("3.7", "3.9")):
            self.updateList()
        else:
            self.updateList()

        self.ModuleList[-1].SerialEdit.setFocus()

    def addModule(self):
        self.ModuleList.append(SimpleModuleBox())
        # For QR Scan
        self.BufferBox = SimpleModuleBox()

        if str(sys.version).startswith("3.8"):
            self.deleteList()
            self.createList()
        elif str(sys.version).startswith(("3.7", "3.9")):
            self.updateList()
        else:
            self.updateList()
        self.changed.emit()

        self.ModuleList[-1].SerialEdit.setFocus()

    def getModules(self):
        return self.FilledModuleList

    def _tryPurdueDatabase(self, moduleName):
        """Helper method: Try to fetch from Purdue database (backup)."""
        try:
            response = module_registry_client.get_purdue_response(moduleName)
            logger.debug(f"Purdue database response available for {moduleName}")
            return response
        except Exception as e:
            logger.warning(f"Failed to access Purdue database: {e}")
            return None

    def fetchModuleTypeDB(self, moduleName):

        try:
            data = _fetch_module_type_from_databases(moduleName)
            if data:
                return data

            # If we get here, neither database has the module
            logger.warning(f"Could not find {moduleName} in either CMS or Purdue database")
            msg = QMessageBox()
            msg.information(
                None,
                "Error",
                f"Could not find {moduleName} in the module registries.",
                QMessageBox.Ok,
            )
            return None

        except Exception:
            logger.error(traceback.format_exc())
            self.master.purdue_connected = False
            return None

    def getFirmwareDescription(self):
        module_types = []
        for module in self.ModuleList:
            if module.getSerialNumber() is None:
                continue  # ignore blank entries
            cable_properties = site_settings.CableMapping[module.getID()]
            if module.getID() not in site_settings.CableMapping.keys():
                raise Exception(
                    f"Encountered cable ID {module.getID()} not present in siteConfig."
                )

            # Access the currently selected QtBeBoard object
            BeBoard = None
            for beboard in self.firmware:
                if beboard.getBoardName() == cable_properties["FC7"]:
                    BeBoard = beboard

            if BeBoard is None:
                raise Exception(
                    f"Could not find {cable_properties['FC7']} in the firmware list. This may occur if the connection to the FC7 is broken."
                )

            # Access the currently selected QtOpticalGroup of the QtBeBoard
            OpticalGroup = None
            for og in BeBoard.getAllOpticalGroups().values():
                if og.getFMCID() == cable_properties["FMCID"]:
                    OpticalGroup = og

            # Create it if it doesn't already exist
            if OpticalGroup is None:
                OpticalGroup = QtOpticalGroup(FMCID=cable_properties["FMCID"])
                OpticalGroup.setBeBoard(
                    BeBoard
                )  # Ignore this line, see explanation in Firmware.py
                try:
                    BeBoard.addOpticalGroup(
                        FMCID=cable_properties["FMCID"], OpticalGroup=OpticalGroup
                    )
                except KeyError as e:
                    logger.error(traceback.format_exc())   
                    if module.getFMCID() in BeBoard.getAllOpticalGroups():
                        BeBoard.removeOpticalGroup(module.getFMCID())
                    return (
                        None,
                        f"Error while adding Optical Group to BeBoard: {repr(e)}",
                    )

            module_db_data = self.fetchModuleTypeDB(module.getSerialNumber())
            if module_db_data is None:
                return (None, f"Could not fetch module metadata for {module.getSerialNumber()}.")

            # Create a QtModule object based on the input data
            Module = QtModule(
                moduleName=module.getSerialNumber(),
                moduleType=module_db_data["type"],
                moduleVersion=module.getVersion(module.getSerialNumber()),
                hdiVersion=module_db_data["HDIversion"],
                FMCPort=cable_properties["FMCPort"],
            )
            Module.setOpticalGroup(
                OpticalGroup
            )  # Ignore this line, see explanation in Firmware.py

            # Fetch the VDDD/VDDA trim values from the DB, make a ChipBox due to built in error handling
            chipBox = ChipBox(
                self.master,
                module.getType(module.getSerialNumber()),
                module.getSerialNumber(),
            )
            chipData = chipBox.getChipData()
            if chipData:
                for chipID in ModuleLaneMap[
                    module.getType(module.getSerialNumber())
                ].values():
                    Module.getChips()[chipID].setVDDA(chipData[chipID]["VDDA"])
                    Module.getChips()[chipID].setVDDD(chipData[chipID]["VDDD"])
                    Module.getChips()[chipID].setEfuseID(chipData[chipID]["EFUSE"])
                    Module.getChips()[chipID].setIREF(chipData[chipID]["IREF"])
                    Module.getChips()[chipID].setVREF(str(1000 * float(chipData[chipID]["VREF"])))
                    Module.getChips()[chipID].setCINJ(str(1e13 * float(chipData[chipID]["CINJ"])))
                    Module.getChips()[chipID].setADC_OFFSET_VOLT(str(chipData[chipID]["ADC_OFFSET_VOLT"]))
                    Module.getChips()[chipID].setADC_MAXIMUM_VOLT(str(chipData[chipID]["ADC_MAXIMUM_VOLT"]))
                    Module.getChips()[chipID].setDAC_PREAMP_L_LIN(chipData[chipID]["DAC_PREAMP_L_LIN"])
                    Module.getChips()[chipID].setDAC_PREAMP_R_LIN(chipData[chipID]["DAC_PREAMP_R_LIN"])
                    Module.getChips()[chipID].setDAC_PREAMP_TL_LIN(chipData[chipID]["DAC_PREAMP_TL_LIN"])
                    Module.getChips()[chipID].setDAC_PREAMP_TR_LIN(chipData[chipID]["DAC_PREAMP_TR_LIN"])
                    Module.getChips()[chipID].setDAC_PREAMP_T_LIN(chipData[chipID]["DAC_PREAMP_T_LIN"])
                    Module.getChips()[chipID].setDAC_PREAMP_M_LIN(chipData[chipID]["DAC_PREAMP_M_LIN"])
                    Module.getChips()[chipID].setDAC_REF_KRUM_LIN(chipData[chipID]["DAC_REF_KRUM_LIN"])
                    Module.getChips()[chipID].setDAC_COMP_LIN(chipData[chipID]["DAC_COMP_LIN"])
                    Module.getChips()[chipID].setDAC_COMP_TA_LIN(chipData[chipID]["DAC_COMP_TA_LIN"])
                    Module.getChips()[chipID].setDAC_LDAC_LIN(chipData[chipID]["DAC_LDAC_LIN"])
                    
            else:
                print(
                    "Something went wrong while fetching VDDD/VDDA from the database. Proceeding with default values."
                )

            # trims = chipBox.getTrimValues()
            # if trims:
            #    for chipID in ModuleLaneMap[module.getType(module.getSerialNumber())].values():
            #        Module.getChips()[chipID].setVDDA(trims[chipID]['VDDA'])
            #        Module.getChips()[chipID].setVDDD(trims[chipID]['VDDD'])
            # else:
            #    print("Something went wrong while fetching VDDD/VDDA from the database. Proceeding with default values.")

            # Add the QtModule object to the currently selected Optical Group
            try:
                OpticalGroup.addModule(
                    FMCPort=cable_properties["FMCPort"], module=Module
                )
            except KeyError as e:
                logger.error(traceback.format_exc())
                return None, f"Error while adding Module to Optical Group: {repr(e)}"

            module_types.append(
                Module.getModuleType() + " " + Module.getModuleVersion()
            )

        if not all([i == module_types[0] for i in module_types]):
            # iterate over module_types, if they're not all identical, return None
            return (
                None,
                "All modules must be of the same type! Please ensure the serial numbers are correct.",
            )

        # only include the board if there are connected modules, otherwise ignore it
        ret = []
        for board in self.firmware:
            if (
                len(board.getAllOpticalGroups()) != 0
            ):  # If modules are connected to the board
                board.setBoardID(len(ret))
                ret.append(board)

        if ret == list():  # nothing added to ret -> no connected modules
            return (
                None,
                "No valid module found! If manually entering module number be sure to press 'Enter' on keyboard.",
            )
        else:
            return ret, "Success"

    @QtCore.pyqtSlot()
    def on_TypeChanged(self):
        self.changed.emit()

    @QtCore.pyqtSlot()
    def on_ModuleFilled(self):
        self.FilledModuleList.append(self.ModuleList[-1])
        self.addModule()

