from PyQt5.QtNetwork import QTcpSocket, QAbstractSocket
import time
import threading
import os
import json

import sys
from pathlib import Path
from Gui.siteSettings import chamber_ip, chamber_port

from python.logging_config import get_logger

BASE_DIR = Path(__file__).resolve().parent.parent 

cache_dir = BASE_DIR / "jsonfiles"
cache_dir.mkdir(exist_ok=True)

logger = get_logger(__name__)


class F4TTemperatureChamber:
    def __init__(self):
        self._instrument = None
        self.sock = QTcpSocket()
        logger.debug("Tcp Socket created")

        self.temp_chamber_cache_file = str(BASE_DIR / "jsonfiles" / "tempChamberProfiles_osu.json")
        logger.debug("Profile cache file: %s" % self.temp_chamber_cache_file)

        self.ip = chamber_ip
        self.port = chamber_port

        self.ambient_temperature = 24
        self._lock = threading.Lock()
        logger.debug("Lock created")
        logger.debug("Loading profiles...")

        self.profiles = {}  # Initialize profiles to an empty dict

        self.connect()

        self.query_profiles()  # This will load from the chamber and cache results

    def connect(self):

        if not self.ip or not self.port:
            logger.error("IP or port not found in cache")
            return
        
        logger.debug("Connecting to %s:%s" % (self.ip, self.port))
    
        # Ensure a clean slate
        try:
            if self.sock.state() != QAbstractSocket.UnconnectedState:
                logger.debug("Socket is not unconnected, aborting")
                self.sock.abort() # Immediate hard reset of the socket
            else:
                logger.debug("Socket is already unconnected")
        except Exception as e:
            logger.error("Error aborting socket: %s" % e)
    
        logger.debug("Connecting to %s:%s" % (self.ip, self.port))
        self.sock.connectToHost(self.ip, int(self.port))
    
        if not self.sock.waitForConnected(5000):
            logger.error("Could not connect to the Thermal Chamber: Connection failed: %s" % self.sock.errorString())
            return

        # --- BUFFER CLEAR / SYNC ---
        # Send a newline to clear any partial commands, then query IDN
        self.write("\n*IDN?")
    
        # Wait and discard whatever there is (clears the pipes)
        if self.sock.waitForReadyRead(2000):
            junk = self.sock.readAll()
            logger.debug("Cleared buffer: %s" % junk.data().decode().strip())

        # --- ACTUAL IDENTITY VERIFICATION ---
        self.write("*IDN?")
        if self.sock.waitForReadyRead(3000):
            idn = self.sock.readAll().data().decode().strip()
            logger.info("Verified Identity: %s" % idn)

        # Set temperature unit to Celsius
        self.set_temperature_unit("C")


    def write(self, cmd):
        cmd = cmd + "\n"
        logger.debug("Writing command: %s" % cmd)
        time.sleep(0.05)
        self.sock.write(cmd.encode())
        self.sock.waitForBytesWritten(1000)

    def query(self, cmd):
        self.write(cmd)
        if self.sock.waitForReadyRead(3000):
            response = self.sock.readAll().data().decode().strip()
            logger.debug("Query response: %s" % response)
            return response
        return None

    def save_profiles(self):
        try:
            data = {}

            if os.path.exists(self.temp_chamber_cache_file):
                with open(self.temp_chamber_cache_file, "r") as f:
                    data = json.load(f)

            data["profiles"] = self.profiles
            
            with open(self.temp_chamber_cache_file, "w") as f:
                logger.debug("File opened for writing")
                json.dump(data, f, indent=4)
                logger.debug("JSON data written to file")

            logger.debug("Saved %d profiles" % len(self.profiles))

        except Exception as e:
            logger.error("Save failed: %s" % e)
            logger.error("Exception type: %s" % type(e).__name__)
            import traceback
            logger.error("Full traceback: %s" % traceback.format_exc())

    def load_connection(self):
        logger.debug("Checking if profile cache file exists: %s" % self.temp_chamber_cache_file)

        ip = self.ip
        port = self.port

        return ip, port


    def load_profiles(self):
        logger.debug("Checking if profile cache file exists: %s" % self.temp_chamber_cache_file)
        if not os.path.exists(self.temp_chamber_cache_file):
            logger.debug("Profile cache file does not exist")
            logger.debug("No cached profiles found")
            self.profiles = {}
            return

        logger.debug("Loading profiles from file: %s" % self.temp_chamber_cache_file)
        try:
            with open(self.temp_chamber_cache_file, "r") as f:
                data = json.load(f)
                self.profiles = data.get("profiles", {})
                logger.debug("Profiles loaded from file")

            logger.debug("Loaded %d cached profiles" % len(self.profiles))
        except Exception as e:
            logger.error("Load failed: %s" % e)

    def set_temperature_unit(self, temperature_unit):
        logger.debug("Setting temperature unit to %s", temperature_unit)
        self.write(f":UNIT:TEMPERATURE {temperature_unit}")

    def set_display_temperature_unit(self, temperature_unit):
        logger.debug("Setting display temperature unit to %s", temperature_unit)
        self.write(f":UNIT:TEMPERATURE:DISPLAY {temperature_unit}")

    def set_setpoint_temperature(self, temperature):
        logger.debug("Setting setpoint temperature to %s", temperature)
        self.write(f":SOURCE:CLOOP1:SPOINT {temperature}")

    def get_temperature(self):
        logger.debug("Getting temperature")
        return self.query(":SOURCE:CLOOP1:PVAL?")

    def get_setpoint(self):
        logger.debug("Getting setpoint")
        result = self.query(":SOURCE:CLOOP1:SPOINT?")
        if result is None:
            logger.error("Failed to get setpoint")
        return result

    def get_profile_state(self):
        logger.debug("Getting profile state")
        result = self.query(":PROGRAM:SELECTED:STATE?")
        if result is None:
            logger.error("Failed to get profile state")
        return result

    def get_profile_step(self):
        logger.debug("Getting profile step")
        result = self.query(":PROGRAM:SELECTED:STEP?")
        if result is None:
            logger.error("Failed to get profile step")
        return result

    def get_profile_name(self):
        logger.debug("Getting profile name")
        result = self.query(":PROGRAM:SELECTED:NAME?")
        if result is None:
            logger.error("Failed to get profile name")
        return result
    
    def set_profile(self, profile_number):
        logger.debug("Setting profile to %i", profile_number)
        self.write(":PROGRAM:SELECTED:NUMBER {}".format(profile_number))
        time.sleep(0.05)

    def start_profile(self):
        logger.debug("=== START PROFILE SEQUENCE ===")

        # --- 1. FORCE STOP ---
        self.stop_profile()

        # --- 2. WAIT FOR IDLE ---
        for _ in range(10):
            state = self.get_profile_state()
            if state and state.strip().upper() in ["IDLE", "STOP", "END"]:
                break
            time.sleep(0.6)

        logger.debug("State after stop: %s" % state)

        # --- 3. Enable Output ---
        self.control_output("ON")
        time.sleep(0.5)

        # --- 4. Start Profile ---
        logger.debug("Starting profile")
        self.write(":PROGRAM:SELECTED:STATE START")
        time.sleep(0.3)

        # --- 5. Verify Running ---
        logger.debug("Verifying profile is running")
        state = self.get_profile_state()
        if state and state.strip().upper() == "RUN":
            logger.debug("Profile is running")
        elif state and state.strip().upper() == "HOLD":
            logger.debug("Profile is on hold, resuming...")
            self.resume_profile()
        else:
            logger.error("Profile failed to start")

        # --- 6. Final verification loop ---
        running = False
        for _ in range(10):
            state = self.get_profile_state()
            if state and state.strip().upper() == "RUN":
                running = True
                break
            time.sleep(0.3)

        if running:
            logger.info("Profile successfully running")
        else:
            logger.error("Profile failed to enter RUNNING state")

    def stop_profile(self):
        logger.debug("Stopping profile")
        self.write(":PROGRAM:SELECTED:STATE STOP")
        time.sleep(1)

        logger.debug("State after stop: %s" % self.get_profile_state())

        logger.debug("Setting temperature to ambient")
        self.set_setpoint_temperature(self.ambient_temperature)
        time.sleep(2)

    def pause_profile(self):
        logger.debug("Pausing profile")
        self.write(":PROGRAM:SELECTED:STATE PAUSE")
        time.sleep(0.3)

    def resume_profile(self):
        logger.debug("Resuming profile")
        self.write(":PROGRAM:SELECTED:STATE RESUME")
        time.sleep(0.3)

    def control_output(self, state):
        logger.debug("Setting output to %s", state)
        self.write(":OUTPUT:STATE %s" % state)
        time.sleep(0.3)

    def turn_off(self):
        logger.debug("Turning off chamber...")
        self.stop_profile()
        self.control_output("OFF")
        time.sleep(1)

    def query_profiles(self, force_refresh=False):
        """
        Robust profile query for Watlow F4T
        Returns: (profiles_dict, source)
        profiles_dict format: {name: number}

        # --- 1. Use cache unless forced ---
        # There is no cache, although if you wish to use cached profiles,
        you can uncomment the following block, and make the required changes in __init__ 
        if self.profiles and not force_refresh:
            logger.debug("Using cached profiles: %d", len(self.profiles))
            return self.profiles, "cache"
        """

        profiles_dict = {}
        source = "instrument"

        try:
            # --- 2. Loop through profile slots ---
            for profile_number in range(1, 41):
                # Step 1: Select profile
                self.set_profile(profile_number)

                # Step 2: Get profile name
                name = self.get_profile_name()

                if not name:
                    logger.debug("Profile %d has no valid name", profile_number)
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





