"""Read-only access to a module's test history on Panthera.

Panthera has no JSON API for this; we use the same request handlers as the
https://panthera.fit.edu/review_modules.php page and parse their HTML:

  1. POST index.php (username, userpass)               -> PHP session cookie
  2. POST review_modules_handler.php?handle=get_review_table
        names[]=<serial>, other filters '%'            -> id_module
  3. GET  ...?handle=get_sequences&id_module=<id>      -> sequence ids
  4. GET  ...?handle=pane_single&id_sequence=<id>      -> sequence details

Nothing is written to Panthera.
"""
import re
import threading

import requests
from bs4 import BeautifulSoup
from PyQt5 import QtCore

from Gui.python.logging_config import get_logger

logger = get_logger(__name__)

BASE_URL = "https://panthera.fit.edu"
HANDLER = BASE_URL + "/request_handlers/review_modules_handler.php"
REVIEW_PAGE = BASE_URL + "/review_modules.php"
TIMEOUT = 30
MAX_SEQUENCES = 40

SEARCH_FILTERS = ("subdetector", "type_module", "croc_version", "grade", "name_rubric", "location")


class PantheraError(Exception):
    pass


def _text(node):
    return " ".join(node.get_text(" ", strip=True).split()) if node is not None else ""


def _style_color(node, prop="color"):
    style = (node.get("style") or "") if node is not None else ""
    m = re.search(r"(?:^|;)\s*%s\s*:\s*([^;]+)" % re.escape(prop), style)
    return m.group(1).strip() if m else ""


def _after_label(text, label):
    """'Upload Time: 2025-07-03 (UTC)' -> '2025-07-03 (UTC)'."""
    return text.split(":", 1)[1].strip() if text.lower().startswith(label.lower()) and ":" in text else text


# ------------------------------------------------------------------ parsing
def parse_module_id(html, serial):
    soup = BeautifulSoup(html or "", "html.parser")
    fallback = None
    for tr in soup.find_all("tr"):
        m = re.search(r"get_sequences\('?(\d+)'?\)", tr.get("onclick", ""))
        if not m:
            continue
        first = tr.find("td")
        if first is not None and _text(first).upper() == serial.upper():
            return m.group(1)
        fallback = fallback or m.group(1)
    return fallback


def parse_sequence_ids(html):
    soup = BeautifulSoup(html or "", "html.parser")
    ids = []
    for node in soup.find_all(attrs={"data-sequence_name": True}):
        sid = (node.get("id") or "").replace("ID", "").strip()
        if sid.isdigit():
            ids.append(sid)
    if not ids:  # layout fallback: any view_sequence('N') / SequenceID N reference
        ids = re.findall(r"view_sequence\('?(\d+)'?\)", html or "") or \
            re.findall(r"SequenceID:?\s*(\d+)", html or "")
    seen, out = set(), []
    for sid in ids:
        if sid not in seen:
            seen.add(sid)
            out.append(sid)
    return out


def parse_pane_single(html):
    """Parse one sequence pane into {'module': {...}, 'sequence': {...}}."""
    soup = BeautifulSoup(html or "", "html.parser")

    module = {}
    crit = soup.find(class_="search-criteria")
    if crit is not None:
        for b in crit.find_all("b"):
            label = _text(b).rstrip(":")
            if not _text(b).endswith(":") or b.find("a") is not None:
                continue
            parent_text = _text(b.parent)
            module[label] = parent_text[len(_text(b)):].strip()

    seq = {"id": "", "name": "", "location": "", "location_full": "", "upload_time": "",
           "reviewed": "", "reviewed_color": "", "is_bad": "", "is_bad_color": "",
           "versions": [], "review_info": "", "grades": "", "results": []}
    top = soup.find("details")
    if top is None:
        return {"module": module, "sequence": seq}
    summary = top.find("summary")
    title = summary.find(class_="sequence-title") if summary else None
    if title is not None:
        seq["name"] = _after_label(_text(title), "Sequence")
        m = re.search(r"(\d+)", title.get("title", ""))
        seq["id"] = m.group(1) if m else ""
    loc = summary.find(class_="sequence-location") if summary else None
    if loc is not None:
        seq["location"] = _after_label(_text(loc), "Location")
        seq["location_full"] = loc.get("title", "")
    up = summary.find(class_="sequence-upload-time") if summary else None
    seq["upload_time"] = _after_label(_text(up), "Upload Time") if up is not None else ""
    rev = summary.find(class_="sequence-reviewed") if summary else None
    if rev is not None:
        seq["reviewed"] = _text(rev)
        seq["reviewed_color"] = _style_color(rev.find("b") or rev)
    bad = summary.find(id=re.compile(r"^isBadButton")) if summary else None
    if bad is not None:
        a = bad.find("a") or bad
        seq["is_bad"] = _text(a)
        seq["is_bad_color"] = _style_color(a, "background-color")

    content = top.find(class_="find_modules-result-content")
    if content is not None:
        # leaf <div>s that belong to the sequence itself, not to a nested result
        for div in content.find_all("div"):
            if div.find("div") is not None or div.find_parent("details") is not top:
                continue
            t = _text(div)
            if "Version:" in t:
                seq["versions"].append(t)
            elif t.startswith("Latest Review Time"):
                seq["review_info"] = t
        # "Grades" block: text between <b>Grades</b> and <b>Results</b>
        grades_b = content.find("b", string=re.compile(r"^\s*Grades\s*$"))
        if grades_b is not None:
            parts = []
            for sib in grades_b.next_siblings:
                if getattr(sib, "name", None) == "b" and _text(sib) == "Results":
                    break
                t = _text(sib) if hasattr(sib, "get_text") else str(sib).strip()
                if t:
                    parts.append(t)
            seq["grades"] = " ".join(parts)
        for det in content.find_all("details", class_="collapsible"):
            s = det.find("summary")
            r = {"name": _text(s), "test_type": "", "time": "", "by": "", "location": ""}
            box = det.find(class_="find_modules-result-content")
            if box is not None:
                for div in box.find_all("div"):
                    if div.find("div") is not None:
                        continue
                    t = _text(div)
                    if t.startswith("Test Type:"):
                        r["test_type"] = _after_label(t, "Test Type")
                    elif t.startswith("Time:"):
                        r["time"] = _after_label(t, "Time")
                    elif t.startswith("Result By:"):
                        a = div.find("a")
                        r["by"] = _text(a) if a is not None else _after_label(t, "Result By")
                    elif t.startswith("Location:"):
                        r["location"] = _after_label(t, "Location")
            seq["results"].append(r)
    return {"module": module, "sequence": seq}


# ------------------------------------------------------------------ client
class PantheraClient:
    def __init__(self, username, password):
        self.username, self._password = username, password
        self.session = None
        self._lock = threading.Lock()

    def _login(self):
        s = requests.Session()
        r = s.post(BASE_URL + "/index.php",
                   files={"username": (None, self.username), "userpass": (None, self._password)},
                   timeout=TIMEOUT)
        r.raise_for_status()
        self.session = s

    def _search(self, serial):
        files = [("names[]", (None, serial))] + [(f, (None, "%")) for f in SEARCH_FILTERS]
        r = self.session.post(HANDLER, params={"handle": "get_review_table"}, files=files, timeout=TIMEOUT)
        r.raise_for_status()
        try:
            return r.json()
        except ValueError:
            return None  # not logged in -> handler answers with a page, not JSON

    def _get(self, **params):
        r = self.session.get(HANDLER, params=params, timeout=TIMEOUT)
        r.raise_for_status()
        return r.text

    def fetch_history(self, serial):
        """Return {'module_id', 'module': {...}, 'sequences': [...], 'truncated': bool}."""
        with self._lock:
            if self.session is None:
                self._login()
            data = self._search(serial)
            if data is None:  # session expired or login failed: retry once
                self._login()
                data = self._search(serial)
            if data is None:
                raise PantheraError("Could not sign in to Panthera with the current GUI login.")
            module_id = parse_module_id(data.get("html", ""), serial)
            if module_id is None:
                return {"module_id": None, "module": {}, "sequences": [], "truncated": False}
            seq_html = self._get(handle="get_sequences", id_module=module_id)
            seq_ids = parse_sequence_ids(seq_html)
            if not seq_ids and seq_html.strip():
                logger.warning(
                    f"No sequence ids recognised in Panthera get_sequences for {serial} "
                    f"(module {module_id}); response starts: {seq_html[:500]!r}")
            truncated = len(seq_ids) > MAX_SEQUENCES
            module, sequences = {}, []
            for sid in seq_ids[:MAX_SEQUENCES]:
                parsed = parse_pane_single(self._get(handle="pane_single", id_sequence=sid, max_image_width=5000))
                module = module or parsed["module"]
                parsed["sequence"]["id"] = parsed["sequence"]["id"] or sid
                sequences.append(parsed["sequence"])
            sequences.sort(key=lambda q: q["upload_time"], reverse=True)
            return {"module_id": module_id, "module": module, "sequences": sequences,
                    "truncated": truncated}


_clients = {}


def client_for(username, password):
    key = (username, password)
    if key not in _clients:
        _clients.clear()  # only keep the current operator's session
        _clients[key] = PantheraClient(username, password)
    return _clients[key]


# ------------------------------------------------------------------ async
class _Signals(QtCore.QObject):
    result = QtCore.pyqtSignal(object)
    error = QtCore.pyqtSignal(str)


class _Runnable(QtCore.QRunnable):
    def __init__(self, fn):
        super().__init__()
        self.fn = fn
        self.signals = _Signals()

    def run(self):
        try:
            self.signals.result.emit(self.fn())
        except PantheraError as e:
            self.signals.error.emit(str(e))
        except requests.RequestException as e:
            logger.warning(f"Panthera request failed: {e}")
            self.signals.error.emit(f"Could not reach Panthera: {e}")
        except Exception as e:
            logger.exception("Unexpected error reading Panthera history")
            self.signals.error.emit(f"Unexpected error reading Panthera history: {e}")


def fetch_history_async(username, password, serial, on_success, on_error):
    """Run fetch_history in the global thread pool; callbacks run on the GUI thread."""
    worker = _Runnable(lambda: client_for(username, password).fetch_history(serial))
    worker.signals.result.connect(on_success)
    worker.signals.error.connect(on_error)
    QtCore.QThreadPool.globalInstance().start(worker)
    return worker
