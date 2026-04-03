import pyvisa
import time
import sys
import json
import os
import threading

from Gui.python.logging_config import get_logger

# Standalone execution block (comment out logger import above for standalone debugging)
class DummyLogger:
    """Dummy logger for standalone execution"""
    def debug(self, *a, **k): print(*a)
    def info(self, *a, **k): print(*a)
    def warning(self, *a, **k): print(*a)
    def error(self, *a, **k): print(*a)

#logger = DummyLogger() # use this one for standalone execution
logger = get_logger(__name__) # use this one for normal execution

class F4TTemperatureChamber:
    """
    Ship of Theseus temperature chamber control class
    """
    def __init__(self, resource):
        self._instrument = None
        self.rm = pyvisa.ResourceManager()
        self.profile_cache_file = os.path.join(
            os.path.dirname(os.path.dirname(__file__)),
            "jsonFiles",
            "profiles.json"
        )
        self.profiles = {}  # Dictionary to store profile information
        self.ambient_temperature = 24
        self.identity = None
        self._lock = threading.Lock()
        self.resource = resource
        self.load_profiles()
        self.connect(self.resource)
        
    def connect(self, resource):

        # --- HARD RESET SESSION ---
        try:
            if self._instrument:
                self._instrument.close()
        except:
            pass

        time.sleep(0.3)

        self._instrument = self.rm.open_resource(resource, access_mode='no_lock')

        self._instrument.read_termination = "\n"
        self._instrument.write_termination = "\n"
        self._instrument.timeout = 15000

        # --- CLEAR DEVICE BUFFER ---
        self._instrument.clear()
        time.sleep(0.2)

        # --- SINGLE SYNC QUERY ---
        idn = self._instrument.query("*IDN?")
        time.sleep(0.1)

        # --- REAL QUERY ---
        idn = self._instrument.query("*IDN?")
        self.identity = idn

        logger.info("Identity: %s", idn)

    def query(self, cmd):
        with self._lock:
            try:
                time.sleep(0.1)
                resp = self._instrument.query(cmd)
                time.sleep(0.05)
                return resp.strip()
            except Exception as e:
                logger.error("Query failed: %s | cmd = %s", e, cmd)
                return ""

    def query_retry(self, cmd, retries=5, delay=0.2):
        for _ in range(retries):
            resp = self.query(cmd)
            if resp:
                return resp
            time.sleep(delay)
        return ""

    def query_after_write(self, write_cmd, query_cmd, verify_cmd=None, verify_val=None):
        self.write(write_cmd)

        if verify_cmd:
            for _ in range(10):
                v = self.query_retry(verify_cmd)
                if v and verify_val(v):
                    break
                time.sleep(0.2)

        return self.query_retry(query_cmd)

    def write(self, cmd):
        with self._lock:
            try:
                self._instrument.write(cmd)
                time.sleep(0.1)
            except Exception as e:
                logger.error("Write failed: %s", e)

    def save_profiles(self):
        try:
            with open(self.profile_cache_file, "w") as f:
                json.dump(self.profiles, f, indent=4)
            logger.debug("Saved %d profiles" % len(self.profiles))
        except Exception as e:
            logger.error("Save failed: %s" % e)

    def load_profiles(self):
        if not os.path.exists(self.profile_cache_file):
            logger.debug("No cached profiles found")
            return

        try:
            with open(self.profile_cache_file, "r") as f:
                self.profiles = json.load(f)
            logger.debug("Loaded %d cached profiles" % len(self.profiles))
        except Exception as e:
            logger.error("Load failed: %s" % e)

    def queryInfo(self):
        # Query instrument for list of info
        #print("Identity: {}".format(self.identity))
        #print("Identity: {}".format(self._instrument.query("*IDN?")))
        logger.debug("Querying instrument...")
        logger.debug("Identity: {}".format(self.query("*IDN?")))
        #print("Ethernet temperature units: {}".format(self._instrument.query(":UNIT:TEMPERATURE?")))
        logger.debug("Display temperature units: {}".format(self.query(":UNIT:TEMPERATURE:DISPLAY?")))
        logger.debug("Current temperature: {}".format(self.query(":SOURce:CLOop1:PVALue?")))
        logger.debug("Current test profile {}".format(self.query("PROGRAM:SELECTED:NAME?")))
        #return self._instrument.query("*IDN?") # for initial communication with f4t
        return self.identity

    def set_temperature_unit(self, temperature_unit):
        """logger.debug("Setting temperature unit to {}".format(temperature_unit))"""
        self.write(":UNIT:TEMPERATURE {}".format(temperature_unit))

    def set_display_temperature_unit(self, temperature_unit):
        #logger.debug("Setting display temperature unit to {}".format(temperature_unit))
        self.write(":UNIT:TEMPERATURE:DISPLAY {}".format(temperature_unit))

    def set_point_temperature(self, temperature):
        #logger.debug("Temperature set point to {}".format(temperature))
        self.write(":SOURCE:CLOOP1:SPOINT {}".format(temperature))

    def get_temperature(self):
        resp = self.query_retry(":SOURCE:CLOOP1:PVAL?")
        return float(resp) if resp else None

    def get_setpoint(self):
        resp = self.query_retry(":SOURCE:CLOOP1:SPOINT?")
        return float(resp) if resp else None

    def get_profile_state(self):
        resp = self.query(":PROGRAM:SELECTED:STATE?")
        return resp.strip() if resp else None

    def get_profile_step(self):
        resp = self.query(":PROGRAM:SELECTED:STEP?")
        return int(resp) if resp else None

    def current_profile(self):
        #Get current profile/test name
        return self.query(":PROGRAM:SELECTED:NAME?")

    def set_profile(self, profile_number):
        logger.debug("Stopping old profile...")
        self.stop_profile()

        logger.debug("Profile set to %s" % profile_number)
        self.write(":PROGRAM:SELECTED:NUMBER {}".format(profile_number))

        # Wait for profile to be selected
        time.sleep(1.5)

        # Verify profile selection
        for _ in range(5):
            selected = self.query(":PROGRAM:SELECTED:NUMBER?")
            if selected and int(selected.strip()) == profile_number:
                logger.debug("Confirmed profile selection: %s", selected)
                return
            time.sleep(0.3)
        
        logger.error("Failed to confirm profile selection")

    def control_profile(self, state):
        logger.debug("%s profile" % state)
        self.write(":PROGRAM:SELECTED:STATE {}".format(state))

    def start_profile(self):
        logger.debug("=== START PROFILE SEQUENCE ===")

        # --- 1. Force STOP ---
        self.write(":PROGRAM:SELECTED:STATE STOP")
        time.sleep(1)

        # --- 2. Wait for IDLE ---
        for _ in range(10):
            state = self.get_profile_state()
            if state and state.strip().upper() in ["IDLE", "STOP", "END"]:
                break
            time.sleep(0.7)

        logger.debug(f"State after stop: {state}")

        # --- 4. Enable output ---
        self.write(":OUTPUT:STATE ON")
        time.sleep(0.5)

        # --- 5. START profile ---
        self.write(":PROGRAM:SELECTED:STATE START")
        time.sleep(1)

        # --- 6. Check state ---
        state = self.get_profile_state()
        logger.debug(f"State after START: {state}")

        # --- 7. If HOLD → RESUME ---
        if state and "HOLD" in state.upper():
            logger.debug("Detected HOLD → sending RESUME")
            self.write(":PROGRAM:SELECTED:STATE RESUME")
            time.sleep(1)

        # --- 8. Final verification loop ---
        running = False
        for _ in range(10):
            state = self.get_profile_state()
            if state and "RUN" in state.upper():
                running = True
                break
            time.sleep(0.7)

        if running:
            logger.info("Profile successfully running")
        else:
            logger.error("Profile failed to enter RUNNING state")

        # --- 9. Debug info ---
        err = self.query("SYST:ERR?")
        logger.debug(f"SCPI ERROR after start: {err}")

        step = self.query(":PROGRAM:SELECTED:STEP?")
        logger.debug(f"Program step: {step}")

        temp = self.get_temperature()
        sp = self.get_setpoint()
        logger.debug(f"Temp: {temp}, Setpoint: {sp}")

    def stop_profile(self):
        state = self.get_profile_state()
        logger.debug(f"Program state before stopping: {state}")

        # Always attempt stop
        self.write(":PROGRAM:SELECTED:STATE STOP")
        time.sleep(1)

        state = self.get_profile_state()
        logger.debug("Program state after stop: %s", state)

        # Force deselection
        try:
            self.write(":PROGRAM:SELECTED:NUMBER 1")
            time.sleep(0.5)
            self.write(":PROGRAM:SELECTED:NUMBER 0")
            time.sleep(0.5)
        except:
            pass

        logger.debug("Setting temperature to ambient...")
        self.write(f":SOURCE:CLOOP1:SPOINT {self.ambient_temperature}")
        time.sleep(0.5)

    def control_output(self, state):
        logger.debug("Setting output to %s", state)
        self.write(f":OUTPUT:STATE {state}")

    def turn_off(self):
        logger.debug("Turning off chamber...")

        self.write(":PROGRAM:SELECTED:STATE STOP")
        time.sleep(1)

        #turn off output
        logger.debug("Turning off output...")
        self.write(":OUTPUT:STATE OFF")
        time.sleep(1)

    def wait_until_idle(self, timeout=10):
        """Wait until the chamber reports IDLE or STOP, up to timeout seconds."""
        start = time.time()
        while time.time() - start < timeout:
            state = self.get_profile_state()
            if state:
                s = state.strip().upper()
                if s in ["IDLE", "STOP", "HOLD", "COMPLETE", "TERMINATED"]:
                    return True
            time.sleep(0.5)
        return False

    def get_stable_name(self, retries=5):
        """Return a stable profile name from the chamber, skipping invalid ones."""
        last = None
        for _ in range(retries):
            time.sleep(0.3)
            name = self.current_profile()
            if not name:
                continue

            name = name.strip().strip('"').strip()
            upper = name.upper()

            if (
                not name
                or any(bad in upper for bad in [
                    "ERROR", "SCPI", "COMPLETED",
                    "TERMINATED", "RUNNING", "IDLE"
                ])
                or name == "0"
            ):
                continue

            if name == last:
                return name

            last = name

        return None

    def query_profiles(self, force_refresh=False):
        """
        Robust profile query for Watlow F4T
        Returns: (profiles_dict, source)
        profiles_dict format: {name: number}
        """

        logger.debug("=== Enter query_profiles(force_refresh=%s) ===", force_refresh)

        logger.debug("Am I connected? Identity: %s", self.identity)

        # --- 1. Use cache unless forced ---
        if self.profiles and not force_refresh:
            logger.debug("Using cached profiles: %d", len(self.profiles))
            return self.profiles, "cache"

        profiles_dict = {}
        source = "instrument"

        try:
            # --- 2. Check chamber state (non-blocking) ---
            state = self.get_profile_state()
            logger.debug("Initial chamber state: %r", state)

            # --- 3. Loop through profile slots ---
            for profile_number in range(1, 41):

                # Step 1: Select profile
                self.write(f":PROGRAM:SELECTED:NUMBER {profile_number}")
                time.sleep(0.5)

                # Step 2: Wait for NAME to stabilize
                name = self.get_stable_name()

                if not name:
                    logger.debug("No valid name for slot %d", profile_number)
                    continue

                # Step 3: Clean
                name = name.strip().strip('"').strip()

                if not name or name == "0":
                    continue

                if name.upper() in ["ERROR", "SCPI", "RUNNING", "IDLE", "HOLD"]:
                    continue

                if name in profiles_dict:
                    continue

                profiles_dict[name] = profile_number
                logger.debug("VALID: %s -> %d", name, profile_number)

            # --- 4. Final sanity check ---
            if not profiles_dict:
                logger.error("No profiles found from instrument")
                if self.profiles:
                    return self.profiles, "cache"
                return {}, "error"

            # --- 5. Cache results ---
            self.profiles = profiles_dict
            self.save_profiles()

            logger.info("Profile scan complete: %d profiles", len(profiles_dict))
            return profiles_dict, source

        except Exception as e:
            logger.error("Profile query failed: %s", e, exc_info=True)
            if self.profiles:
                return self.profiles, "cache"
            return {}, "error"

#debugging stuff
if __name__ == "__main__":
    chamber = F4TTemperatureChamber("TCPIP::128.146.32.200::5025::SOCKET")

    print("\n--- SANITY TEST START ---")

    print("IDN:", chamber.query("*IDN?"))
    print("STATE:", chamber.query(":PROGRAM:SELECTED:STATE?"))
    print("TEMP:", chamber.query(":SOURCE:CLOOP1:PVAL?"))
    print("SETPOINT:", chamber.query(":SOURCE:CLOOP1:SPOINT?"))

    print("--- SANITY TEST END ---\n")
