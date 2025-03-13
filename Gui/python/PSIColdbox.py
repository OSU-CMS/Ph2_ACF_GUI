from paho.mqtt import client as mqtt_client
from paho.mqtt.enums import CallbackAPIVersion
from time import sleep, time
import threading 

# Define monitoring payload types
payload_types = {
    "Env": {
        "temp_air": float,
        "temp_water": float,
        "rel_hum": float,
        "dp": float,
        "can_errors": int,
        "i2c_errors": int,
        "runtime": int,
        "valve0_status": int,
        "valve1_status": int,
    },
    "VAR": {
        "green": int,  # 1 means on
        "yellow": int,  # 1 means on
        "red": int,  # 1 means on
        "lid_status": int,  # 1 means locked, otherwise open
        "interlock_status": int,  # 1 means good, otherwise bad
        "free_disk_space": int,  # value in GB
        "flowswitch_status": int,  # < 0 means not implemented, 0 means bad, otherwise good
        "throttle": int,
    },
    'Supply_U': [float] * 8,
    'Supply_P': [float] * 8,
    'Supply_I': [float] * 8,
    'Peltier_U': [float] * 8,
    'Peltier_P': [float] * 8,
    'Peltier_I': [float] * 8,
    'Peltier_R': [float] * 8,
    'ControlVoltage_Set': [float] * 8,
    'Error': [int] * 8,  # TEC controller errors in hex format (leading '0x')
    'Mode': [int] * 8,
    'Temp_Diff': [float] * 8,
    'Temp_M': [float] * 8,
    'Temp_Set': [float] * 8,
    'Temp_W': [float] * 8,
    'PID_Max': [float] * 8,
    'PID_Min': [float] * 8,
    'PID_kd': [float] * 8,
    'PID_ki': [float] * 8,
    'PID_kp': [float] * 8,
    'PowerState': [int] * 8,
    'Ref_U': [float] * 8,
}

# General conversion function
def convert(payload):
    payload_type, payload_data = payload.split("=", 1)
    payload_type = payload_type.strip()

    if payload_type not in payload_types:
        raise ValueError(f"Unknown payload type: {payload_type}")

    # Split data by comma and strip whitespace
    payload_list = [x.strip() for x in payload_data.split(",")]

    if payload_type == "VAR":
        # Special parsing for 'VAR' payload
        payload_dict = {}
        for key, value in zip(payload_types["VAR"], payload_list):
            if value.startswith(('G', 'Y', 'R', 'L', 'I', 'D', 'F', 'T')):
                # Extract numeric part from strings like 'G0', 'Y1'
                # Handle traffic light
                if value.startswith(('G', 'Y', 'R')):
                    payload_dict[key] = "off"
                    if int(value[1:]) == 1:
                        payload_dict[key] = "on"
                # Handle lid sensor
                elif value.startswith('L'):
                    payload_dict[key] = "open"
                    if int(value[1:]) == 1:
                        payload_dict[key] = "locked"
                # Handle interlock status
                elif value.startswith('I'):
                    payload_dict[key] = "bad"
                    if int(value[1:]) == 1:
                        payload_dict[key] = "good"
                elif value.startswith('F'):
                    payload_dict[key] = "bad"
                    if int(value[1:]) < 0:
                        payload_dict[key] = "not implemented"
                    elif int(value[1:]) > 0:
                        payload_dict[key] = "good"
                else:
                    payload_dict[key] = int(value[1:])
            else:
                payload_dict[key] = int(value)
        return {"VAR": payload_dict}

    elif payload_type == "Env":
        # Convert to dictionary for 'Env' type
        payload_dict = dict(zip(payload_types["Env"], payload_list))
        for key, value in payload_dict.items():
            payload_dict[key] = payload_types["Env"][key](value) if value.lower() != "nan" else None
        return {"Env": payload_dict}

    elif payload_type == "Error":
        # Error returns hex values, convert to integers
        return {payload_type: [payload_types[payload_type][i](value, 16) if value.lower() != "nan" else None for i, value in enumerate(payload_list)]}
    
    elif isinstance(payload_types[payload_type], list):
        # General handling for list-based payloads (e.g., 'Peltier_U', 'Peltier_R', etc.)
        return {payload_type: [payload_types[payload_type][i](value) if value.lower() != "nan" else None for i, value in enumerate(payload_list)]}

    raise ValueError(f"Unhandled payload type: {payload_type}")

        
class ColdboxMonitor:
    """ essentially Clemens' MonTessieClient
    threaded, monitors tessies 'monTessie' topic

    provides
    * a callback option for alarms
    * read access to cached monitor data
    """


    def __init__(self, host, topic="monTessie", error_callback=None):
        """
        Initialize the MonTessie MQTT client.
        :param host: MQTT broker hostname
        :param topic: MQTT topic to subscribe to
        :param error_callback: Callback function for handling "Error" messages
        """
        self.host = host
        self.topic = topic
        self.error_callback = error_callback
        self.client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2)

        # Set up MQTT callbacks
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message

        # cache data to make it available without waiting for it to appear
        self.lock = threading.Lock()
        self.messages = {}

    def on_connect(self, client, userdata, flags, reason_code, properties=None):
        """Callback for successful connection."""
        if reason_code == 0:
            print(f"Connected to {self.host} successfully.")
            client.subscribe(self.topic)
        else:
            print(f"Failed to connect to {self.host}, reason code: {reason_code}. Retrying...")

    def on_disconnect(self, client, userdata, disconnect_flags, reason_code, properties=None):
        """Callback for disconnection."""
        print(f"Disconnected from {self.host}. Reason code: {reason_code}. Reconnecting...")
        threading.Thread(target=self.reconnect, daemon=True).start()

    def reconnect(self):
        """Reconnect to the MQTT broker."""
        while True:
            try:
                print(f"Attempting to reconnect to {self.host}...")
                self.client.reconnect()
                print(f"Successfully reconnected to {self.host}.")
                break
            except Exception as e:
                print(f"Reconnection failed: {e}. Retrying in 60 seconds...")
                time.sleep(60)

    def on_message(self, client, userdata, msg):
        """Callback for received messages."""
        payload = msg.payload.decode()
        try:
            converted_payload = convert(payload)
            payload_type = list(converted_payload.keys())[0]

            if payload_type == "Error":
                if 1 in converted_payload[payload_type] and self.error_callback:
                    self.error_callback(converted_payload)
            if payload_type == "VAR":
                if converted_payload[payload_type]["lid_status"] != "locked":
                    self.error_callback(converted_payload)
                if converted_payload[payload_type]["interlock_status"] != "good":
                    self.error_callback(converted_payload)
                if converted_payload[payload_type]["flowswitch_status"] == "bad":
                    self.error_callback(converted_payload)

            with self.lock:
                for key in converted_payload:
                    self.messages[key] = converted_payload[key]
                    
        except Exception as e:
            print(f"Failed to process message: {e}")
            print(f"payload = '{payload}'")

    def start(self):
        """Start the MQTT client."""
        try:
            self.client.connect(self.host, 1883, 60)
            self.client.loop_start()
        except Exception as e:
            print(f"Failed to connect to {self.host}: {e}. Retrying in the background...")
            threading.Thread(target=self.reconnect, daemon=True).start()

    def stop(self):
        """Stop the MQTT client."""
        self.client.loop_stop()
        self.client.on_disconnect = None
        self.client.disconnect()

    # access to cached messages, do I need to worry about thread safety here?
    def get(self, key):
        if key in self.messages:
            with self.lock:
                return self.messages[key]
            
        if key not in payload_types:
            print("unknown key ",key)
            return None
        return None

    
class Tessie:
    """
        This class represents a client for the Tessie MQTT communication system.

        Attributes:
        broker (str): The address of the MQTT broker.
        port (int): The port number of the MQTT broker.
        topic (str): The topic to subscribe to.
        _client (mqtt_client.Client): The MQTT client object.
        waiting (List[str]): A list of variables that are waiting to be received.
        found (List[str, str]): A list of received variables and their corresponding values.

        Methods:
        __init__(self):
            Initializes the Tessie client by setting the attributes, connecting to the MQTT broker, and subscribing to the topic.
        on_connect(client, userdata, flags, rc):
            A callback function that is called when the client successfully connects to the MQTT broker.
        _connect_mqtt(self):
            Connects to the MQTT broker.
        decode_msg(msg: str):
            Decodes a received message and stores it in the `found` list if it matches a waiting variable.
        on_message(client, userdata, msg_recv):
            A callback function that is called when a message is received on the subscribed topic.
        _subscribe(self):
            Subscribes to the specified topic.
        _wait_for_var(var):
            Waits for a specified variable to be received and returns its value.
        get(self, var, args=""):
            Sends a request to get the value of a specified variable. Returns the value of the variable.
        set(self, var, data, args=""):
            Sends a request to set the value of a specified variable to a specified value.
        cmd(self, cmd, args=""):
            Sends a command to the Tessie system.
        help(self):
            Sends a request for help on the Tessie system.
        """

    def __init__(self, broker, topic='ctrlTessie'):
        self.broker = broker
        self.port = 1883
        self.topic = topic
        client_id = 'Python Tessie Client'
        self._client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2) 
        Tessie.waiting = []
        Tessie.found = []
        self._connect_mqtt()
        while not self._client.is_connected():
            pass
        self._subscribe()

    @staticmethod
    def on_connect(client, userdata, flags, rc, properties=None):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    def _connect_mqtt(self):
        # Set Connecting Client ID
        self._client.on_connect = self.on_connect
        self._client.connect(self.broker, self.port)
        self._client.loop_start()

    def stop(self):
        self._client.loop_stop()
        self._client.disconnect()

    @staticmethod
    def decode_msg(msg: str):
        for var in Tessie.waiting:
            if msg.startswith(var):
                Tessie.waiting.remove(var)
                Tessie.found.append([var, msg])

    @staticmethod
    def on_message(client, userdata, msg_recv):
        if msg_recv.payload.decode().startswith('get'):
            return
        if msg_recv.payload.decode().startswith('set'):
            return
        if msg_recv.payload.decode().startswith('cmd'):
            return
        if msg_recv.payload.decode().startswith('help'):
            return
        if msg_recv.payload.decode().startswith('>'):
            print(msg_recv.payload.decode())
            return
        # print('recv: ' + msg_recv.payload.decode())
        Tessie.decode_msg(msg_recv.payload.decode())

    def _subscribe(self):
        self._client.subscribe(self.topic)
        self._client.on_message = self.on_message

    @staticmethod
    def _wait_for_var(var):
        timeout = 5
        start_time = time()
        while True:
            if time() - start_time > timeout:
                # timeout reached, exit the loop
                break
            for msg in Tessie.found:
                if msg[0] == var:
                    Tessie.found.remove(msg)
                    return msg[1]

    def get(self, var, args="") -> str:
        msg = 'get ' + var + args
        Tessie.waiting.append(var)
        # print('send ' + msg)
        if self._client.publish(self.topic, msg)[0] != 0:
            print(f'Failed to send message: {msg}')
        result = Tessie._wait_for_var(var)
        if not result:
            print('no result')
            return False
        if not result.startswith(var):
            print('wrong result')
            return False
        result = result[len(var) + 3:]
        # print(result)
        return result

    def set(self, var, data, args=""):
        msg = 'set ' + str(var) + ' ' + str(data) + args
        # print('send ' + msg)
        if self._client.publish(self.topic, msg)[0] != 0:
            print(f'Failed to send message: {msg}')

    def cmd(self, cmd, args="", answer=False):
        msg = 'cmd ' + cmd + args
        # print('send ' + msg)
        if answer:
            Tessie.waiting.append(cmd)
        if self._client.publish(self.topic, msg)[0] != 0:
            print(f'Failed to send message: {msg}')
        if answer:
            result = Tessie._wait_for_var(cmd)
            if not result:
                print('no result')
                return False
            if not result.startswith(cmd):
                print('wrong result')
                return False
            result = result[len(cmd) + 3:]
            # print(result)
            return result

    def help(self):
        msg = 'help'
        if self._client.publish(self.topic, msg)[0] != 0:
            print(f'Failed to send message: {msg}')


class Valve:
    def __init__(self, tessie: Tessie, i):
        self.name = "valve" + str(i)
        self._tessie = tessie

    def set(self, value):
        self._tessie.set(self.name, "on" if value == 1 else "off")

    def get(self):
        return 1 if self._tessie.get(self.name) == "on" else 0


class Env:
    def __init__(self, tessie: Tessie):
        self._tessie = tessie

    def getRH(self):
        return float(self._tessie.get("RH"))

    def getDP(self):
        return float(self._tessie.get("DP"))

    def getTempAir(self):
        return float(self._tessie.get("Temp"))

    def getTempWater(self):
        return float(self._tessie.get("Temp_W", " tec 8"))

    def getVprobe(self, number):
        data = self._tessie.get(f"vprobe{number}")
        return [float(x) for x in data.split(",")] # returns [-999.0] if not available

    

class ConfTEC:
    def __init__(self, tessie: Tessie, i):
        self._tessie = tessie
        self.name = " tec " + str(i)

    def _single(self, data):
        if self.single:
            return data
        else:
            return [float(x) for x in data.split(",")]

    def saveToFlash(self):
        return self._tessie.cmd("SaveVariables", self.name, True)

    def getPID(self):
        return [self._single(self._tessie.get("PID_kp", self.name)),
                self._single(self._tessie.get("PID_ki", self.name)),
                self._single(self._tessie.get("PID_kd", self.name))]

    def setPID(self, kp: float, ki: float, kd: float):
        self._tessie.set("PID_kp", str(kp), self.name)
        self._tessie.set("PID_ki", str(ki), self.name)
        self._tessie.set("PID_kd", str(kd), self.name)

    def getPIDMinMax(self):
        return [self._single(self._tessie.get("PID_Min", self.name)),
                self._single(self._tessie.get("PID_Max", self.name))]

    def setPIDMinMax(self, pidmin, pidmax):
        self._tessie.set("PID_Min", str(round(pidmin, 1)), self.name)
        self._tessie.set("PID_Max", str(round(pidmax, 1)), self.name)

    def setRef(self, ref: float):
        self._tessie.set("Ref_U", str(round(ref, 3)), self.name)

    def getRef(self):
        return self._single(self._tessie.get("Ref_U", self.name))

    def setMode(self, mode: int):
        self._tessie.set("Mode", str(mode), self.name)

    def getMode(self):
        return self._single(self._tessie.get("Mode", self.name))

    def clearError(self):
        self._tessie.cmd("clearError", self.name)

    def getError(self):
        return self._single(self._tessie.get("Error", self.name))


class TEC:
    def __init__(self, tessie: Tessie, i):
        self._tessie = tessie
        self.conf = ConfTEC(tessie, i)
        self.name = " tec " + str(i)
        self.single = True if i != 0 else False

    def _single(self, data):
        if self.single:
            return data
        else:
            return [float(x) for x in data.split(",")]

    def pon(self):
        self._tessie.cmd("Power_On", self.name)

    def poff(self):
        self._tessie.cmd("Power_Off", self.name)

    def getState(self):
        return self._single(self._tessie.get("PowerState", self.name))

    def getTemp(self):
        return self._single(self._tessie.get("Temp_M", self.name))

    def getUI(self):
        return [self._single(self._tessie.get("Supply_U", self.name)),
                self._single(self._tessie.get("Supply_I", self.name)),
                self._single(self._tessie.get("Peltier_U", self.name)),
                self._single(self._tessie.get("Peltier_I", self.name))]

    def setTemp(self, temp: float):
        self._tessie.set("Temp_Set", str(round(temp)), self.name)

    def setVoltage(self, u: float):
        self._tessie.set("ControlVoltage_Set", str(round(u, 2)), self.name)

    def getVoltage(self):
        return self._single(self._tessie.get("ControlVoltage_Set", self.name))

    def reset(self):
        return self._tessie.cmd("Reboot", self.name, True)

    def loadFromFlash(self):
        return self._tessie.cmd("LoadVariables", self.name, True)

    def getSWVersion(self):
        return self._tessie.cmd("GetSWVersion", self.name, True)

                
                

class Coldbox:
    """ 
    colbox controller based on Noah code
    allows two styles of access:
    a) through component objects,   e.g. coldbox.valve0.set(1)
    b) through getters/setters      e.g. coldbox.flush("on")
    an optional callback function is called in case of alarms
    """
    def __init__(self,host='coldbox02', error_callback=None):
        self._tessie = Tessie(host, topic="ctrlTessie")
        self.valve0 = Valve(self._tessie, 0)
        self.valve1 = Valve(self._tessie, 1)
        self.tecall = TEC(self._tessie, 0)
        self.tec1 = TEC(self._tessie, 1)
        self.tec2 = TEC(self._tessie, 2)
        self.tec3 = TEC(self._tessie, 3)
        self.tec4 = TEC(self._tessie, 4)
        self.tec5 = TEC(self._tessie, 5)
        self.tec6 = TEC(self._tessie, 6)
        self.tec7 = TEC(self._tessie, 7)
        self.tec8 = TEC(self._tessie, 8)
        self.env = Env(self._tessie)

        self.valid_channels = [1,2,3,4,5,6,7,8]
        self._tecs = [self.tecall, self.tec1, self.tec2, self.tec3, self.tec4, self.tec5, self.tec6, self.tec7, self.tec8]

        self.monitor = ColdboxMonitor(
            host=host,
            topic="monTessie",
            error_callback=error_callback,
        )

    def __enter__(self):
        self.monitor.start()
        
    def __exit__(self, *args):
        self.monitor.stop()
        self._tessie.stop()
        #print("exit", args)


    def channel_arg(self, arg, caller=""):
        """ helper for handling channel arguments """
        if arg in (0, "all"):
            return [0]
        elif arg in self.valid_channels:
            return [arg]
        elif isinstance(arg, list) and all(channel in self.valid_channels for channel in arg):
            return arg
        elif isinstance(arg, tuple) and all(channel in self.valid_channels for channel in arg):
            return arg
        else:
            print("Coldbox.{caller} : invalid channel argument ",arg)
            return []

     # pass-through functions for data available in the control topic
    def get_air_temperature(self)->float:
        """ query the the air temperature sensor"""
        return self.env.getTempAir()

    def get_water_temperature(self)->float:
        """ query the the air temperature sensor"""
        return self.env.getTempWater()

    def get_relative_humidity(self)->float:
        """ query the the relative humidity sensor"""
        return self.env.getRH()
    
    def get_dewpoint(self)->float:
        """ query the dewpoint"""
        return self.env.getDP()

    def get_tec_temperature(self, channel = 0):
        """ tec temperatures of either of a single channel or all channels (channel=0)"""
        return self._tec_get_property(channel, "Temp_M")
        if channel in (0, "all"):
            return self.tecall.getTemp()
        elif channel in self.valid_channels:
            return self._tecs[channel].getTemp()
    
    def rinse(self):
        """ switch N2 to 'rinse'"""
        self.flush("rinse")
    
    def flush(self, cmd="flush"):
        """ control the N2 valves. Possible  options are 
        "flush", "on"  : start flushing
        "rinse"        : start rinsing
        "both"         : open both valves
        "off"          : stop N2 flow
        """
        if cmd in ("flush", "on", 1, "1"):
            self.valve0.set(1)
            self.valve1.set(0)
        elif cmd in ("rinse"):
            self.valve0.set(0)
            self.valve1.set(1)
        elif cmd in ("both"):
            self.valve0.set(1)
            self.valve1.set(1)
        elif cmd in ("off", 0, "0"):
            self.valve0.set(0)
            self.valve1.set(0)

            
    def get_monitor_data(self, key, timeout = 0):
        """ get data from the monitor topic"""
        if not key in payload_types:
            print("unknown key ", key)
            return None

        for ntry in range(timeout + 1):
            result = self.monitor.get(key)
            if result is not None:
                return result
            sleep(1)
        return None
            
        
    def get_interlock_status(self, timeout=0):
        """ get the interlock status,  "good" | "bad" """
        var = self.get_monitor_data("VAR", timeout)
        if var is None:
            return "unknown"
        return var["interlock_status"]


    def get_flow_switch(self, timeout=0):
        """ get the flow switch status:  "good" | "bad" | "not available" """
        var = self.get_monitor_data("VAR", timeout)
        if var is None:
            return "unknown"
        return var["flowswitch_status"]

    def get_lid_status(self, timeout=0):
        """ get the lid status :  "open" | "locked" """
        var = self.get_monitor_data("VAR", timeout)
        if var is None:
            return "unknown"
        return var["lid_status"]

    def get_traffic_light(self, timeout=0):
        """ get the status of the 'traffic light' as a dictionary 
        {"green" : "on"|"off", "yellow" : "on"|"off", "red" : "on" | "off"}
        """
        var = self.get_monitor_data("VAR", timeout)
        if var is None:
            return {"unknown" for color in ("green","yellow","red")}
        return {color:var[color] for color in ("green","yellow","red")}
        
       
    def tec(self, channels=0, cmd=None, voltage = None, temperature = None):
        """ control tec settings, cmd = "on" | "off" 
        voltage or temperature can be set, the mode is chosen to which value is present
        """
        
        channels = self.channel_arg(channels, "tec")
            
        if cmd in ("off", "poff"):
            for c in channels:
                self._tecs[c].poff()

        if isinstance(temperature,float) and voltage is None:
            for c in channels:
                self._tecs[c].setTemp(temperature)
        elif isinstance(voltage, float) and temperature is None:
            for c in channels:
                self._tecs[c].setVoltage(voltage)
            
        if cmd in ("on", "pon"):
            for c in channels:
                self._tecs[c].pon()

    def _tec_get_property(self, channel, key):
        """ query tec values in the control topic,  for either a single channel or all (channel=0)"""
        if key in ("Temp_M", "PowerState", "Supply_U", "Supply_I", "Peltier_U", "Peltier_I"):
            if channel in (0, "all"):
                tec = self.tecall
                return tec._single(tec._tessie.get(key, tec.name))
            elif channel in self.valid_channels:
                tec = self._tecs[channel]
                return tec._single(tec._tessie.get(key, tec.name))
            else:
                print("unknown channel ", channel)
                return None
        else:
            print("unknown tec property", key)
            return None
        
    def get_tec_state(self, channel=0):
        """ query the on/off state of a tec channel or all channels """
        return self._tec_get_property(channel, "PowerState")


    def on(self):
        """ turn on all tecs """
        self.tecall.pon()

    def off(self):
        """ turn off all tecs """
        self.tecall.poff()

        
    def get_voltage_probe(self, channel):
        """ return a dictionary with voltages or None if no probecard connected
        for single channels only, channel must be an integer from [1..8]
        """
        if not channel in self.valid_channels:
            print("Coldbox.get_voltage_probe : invalid channel ",channel)
            return None
        
        vprobe_names = [
            "VIN", "Voff1/2", "vdda0", "vddd0",
            "vdda1", "vddd1", "vdda2", "vddd2",
            "vdda3", "vddd3"
        ]
        v = self.env.getVprobe(channel)
        try:
            return {vprobe_names[n]:v[n] for n in range(10)}

        except TypeError:
            return None


        
def handle_error_message(error_payload):
    """
    Custom callback to handle "Error" messages.
    :param error_payload: The parsed "Error" payload
    """
    print("WARNING: Error detected!")
    print(error_payload)


    
if __name__ == "__main__":

    # initialize the Coldbox controller and provide a callback for alarms
    coldbox = Coldbox(host='coldbox02.psi.ch', error_callback=handle_error_message)

    with coldbox:
        coldbox.flush()
        print("air temperature    ", coldbox.get_air_temperature())
        print("water temperature  ", coldbox.get_water_temperature())
        print("interlock status   ", coldbox.get_interlock_status(timeout=10))
        print("traffic light      ", coldbox.get_traffic_light())
        print("flow switch        ", coldbox.get_flow_switch())
        print("lid                ", coldbox.get_lid_status())
        channel = 8
        print(f"voltage probes for channel {channel} = ", coldbox.get_voltage_probe(channel)) 

        try:
            while True:
                print("relative humidity ", coldbox.get_relative_humidity())
                sleep(10)
        except KeyboardInterrupt:
            print('interrupted!')
  
    print("shutting down")
