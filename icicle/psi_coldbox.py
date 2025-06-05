from .instrument import Instrument, Multiton, retry_on_fail, acquire_lock, ChannelError
from .scpi_instrument import is_in, is_numeric, ValidationError
from .mqtt_client import MQTTClient
import time
import logging


class ConnectionError(RuntimeError):
    pass


class SettingError(Exception):
    pass


logger = logging.getLogger(__name__)


@Instrument.register
class PSIColdbox(Instrument, metaclass=Multiton, key="resource"):
    SMALL_DELAY = 1

    # Default settings as proposed by PSI
    PID_KD = 0.25
    PID_KI = 0.015
    PID_KP = 0.17
    PID_MIN = -4
    PID_MAX = 12
    CONTROL_VOLTAGE = 0.0
    TEMPERATURE = 15.0
    MODE = 1

    # Measurements are performed in *C
    MEASURE_TYPES = {
        "TEMPERATURE": "Temperature of the peltier (degC)",
        "SPEED": "Can not be measured, this is required by TemperatureChannel",
        "VOLTAGE": "Can not be measured, this is required by MeasureChannel",
    }

    DEFAULT_PIN_MAP = {
        0: "VIN",
        1: "VOFS",
        2: "VDDA_ROC0",
        3: "VDDD_ROC0",
        4: "VDDA_ROC1",
        5: "VDDD_ROC1",
        6: "VDDA_ROC2",
        7: "VDDD_ROC2",
        8: "VDDA_ROC3",
        9: "VDDD_ROC3",
    }

    # Settings that can be set or read
    # SET: Publish a set command to MQTT and await reply
    # QUERY: Publish a get command to MQTT and await reply
    # FIND: Statement by which the reply is identified
    # READ: Statement by which the reply is identified in the monitor channel
    # POS: Position where the reply is found in the read argument. (1, 2, 3, ...)
    # verifier: argument requirements for a set command
    # default: default value, used for testing purpose
    # channel: can this argument be run on a channel (0=ALL, 1-8)
    # type: object of returned argument

    SETTINGS = {
        "CONTROL_VOLTAGE": {
            "SET": "set ControlVoltage_Set {}",
            "QUERY": "get ControlVoltage_Set",
            "FIND": "ControlVoltage_Set",
            "verifier": is_numeric(min=-12, max=12),
            "channel": True,
            "default": CONTROL_VOLTAGE,
            "type": float,
        },
        "DEW_POINT": {
            "QUERY": "get DP",
            "FIND": "DP",
            "READ": "Env",
            "POS": 4,
            "type": float,
        },
        "DISKSPACE": {"READ": "VAR", "POS": 6, "type": int, "ASSERT": "D"},
        "DP": {"QUERY": "get DP", "FIND": "DP", "READ": "Env", "POS": 4, "type": float},
        "ERROR": {
            "QUERY": "get Error",
            "READ": "Error",
            "FIND": "Error",
            "channel": True,
            "type": int,
        },
        "FLOW": {"READ": "VAR", "POS": 7, "type": int, "ASSERT": "F"},
        "FLUSH_TOGGLE": {
            "SET": "cmd valve0",
            "QUERY": "get valve0",
            "FIND": "valve0",
            "READ": "Env",
            "POS": 8,
            "type": int,
        },
        "FLUSH": {
            "SET": "set valve0 {}",
            "QUERY": "get valve0",
            "FIND": "valve0",
            "READ": "Env",
            "POS": 8,
            "type": int,
            "verifier": is_in("on", "off"),
            "default": "off",
        },
        "INTERLOCK": {"READ": "VAR", "POS": 5, "type": int, "ASSERT": "I"},
        "LID": {"READ": "VAR", "POS": 4, "type": int, "ASSERT": "L"},
        "MODE": {
            "SET": "set Mode {}",
            "QUERY": "get Mode",
            "FIND": "Mode",
            "channel": True,  # mode can now be set for each channel individually
            "verifier": is_in(0, 1),
            "default": MODE,
            "type": int,
        },
        "N2_THROTTLE": {
            "SET": "cmd throttleN2{}",
            "READ": "VAR",
            "POS": 8,
            "type": int,
            "ASSERT": "T",
            "verifier": is_in("On", "Off"),
            "default": "Off",
        },
        "PELTIER_CURRENT": {
            "QUERY": "get Peltier_I",
            "FIND": "Peltier_I",
            "channel": True,
            "type": float,
        },
        "PELTIER_POWER": {
            "QUERY": "get Peltier_P",
            "FIND": "Peltier_P",
            "channel": True,
            "type": float,
        },
        "PELTIER_RESISTANCE": {
            "QUERY": "get Peltier_R",
            "FIND": "Peltier_R",
            "channel": True,
            "type": float,
        },
        "PELTIER_VOLTAGE": {
            "QUERY": "get Peltier_U",
            "FIND": "Peltier_U",
            "channel": True,
            "type": float,
        },
        "PID_KD": {
            "SET": "set PID_kd {}",
            "QUERY": "get PID_kd",
            "FIND": "PID_kd",
            "verifier": is_numeric(min=0, max=1),
            "channel": True,
            "default": PID_KD,
            "type": float,
        },
        "PID_KI": {
            "SET": "set PID_ki {}",
            "QUERY": "get PID_ki",
            "FIND": "PID_ki",
            "verifier": is_numeric(min=0, max=1),
            "channel": True,
            "default": PID_KI,
            "type": float,
        },
        "PID_KP": {
            "SET": "set PID_kp {}",
            "QUERY": "get PID_kp",
            "FIND": "PID_kp",
            "verifier": is_numeric(min=0, max=1),
            "channel": True,
            "default": PID_KP,
            "type": float,
        },
        "PID_MAX": {
            "SET": "set PID_Max {}",
            "QUERY": "get PID_Max",
            "FIND": "PID_Max",
            "verifier": is_numeric(min=0, max=12),
            "channel": True,
            "default": PID_MAX,
            "type": float,
        },
        "PID_MIN": {
            "SET": "set PID_Min {}",
            "QUERY": "get PID_Min",
            "FIND": "PID_Min",
            "verifier": is_numeric(min=-5, max=0),
            "channel": True,
            "default": PID_MIN,
            "type": float,
        },
        "POWER": {
            "SET": "cmd Power_{}",
            "QUERY": "get PowerState",
            "FIND": "PowerState",
            "verifier": is_in("On", "Off"),
            "channel": True,
            "type": int,
        },
        "REFERENCE_VOLTAGE": {
            "QUERY": "get Ref_U",
            "FIND": "Ref_U",
            "channel": True,
            "type": float,
        },
        "REL_HUMIDITY": {
            "QUERY": "get RH",
            "FIND": "RH",
            "READ": "Env",
            "POS": 3,
            "type": float,
        },
        "RELATIVE_HUMIDITY": {
            "QUERY": "get RH",
            "FIND": "RH",
            "READ": "Env",
            "POS": 3,
            "type": float,
        },
        "RH": {"QUERY": "get RH", "FIND": "RH", "READ": "Env", "POS": 3, "type": float},
        "RINSE_TOGGLE": {
            "SET": "cmd valve1",
            "QUERY": "get valve1",
            "FIND": "valve1",
            "READ": "Env",
            "POS": 9,
            "type": int,
        },
        "RINSE": {
            "SET": "set valve1 {}",
            "QUERY": "get valve1",
            "FIND": "valve1",
            "READ": "Env",
            "POS": 9,
            "type": int,
            "verifier": is_in("on", "off"),
            "default": "off",
        },
        "SOFTWARE_VERSION": {
            "QUERY": "cmd GetSWVersion",
            "FIND": "GetSWVersion",
            "channel": True,
            "type": str,
        },
        "SUPPLY_CURRENT": {
            "QUERY": "get Supply_I",
            "FIND": "Supply_I",
            "channel": True,
            "type": float,
        },
        "SUPPLY_POWER": {
            "QUERY": "get Supply_P",
            "FIND": "Supply_P",
            "channel": True,
            "type": float,
        },
        "SUPPLY_VOLTAGE": {
            "QUERY": "get Supply_U",
            "FIND": "Supply_U",
            "channel": True,
            "type": float,
        },
        "TEMPERATURE_AIR": {
            "QUERY": "get Temp",
            "FIND": "Temp",
            "READ": "Env",
            "POS": 1,
            "type": float,
        },
        "TEMPERATURE_BOX": {
            "QUERY": "get Temp",
            "FIND": "Temp",
            "READ": "Env",
            "POS": 1,
            "type": float,
        },
        "TEMPERATURE_MEASURED": {
            "QUERY": "get Temp_M",
            "FIND": "Temp_M",
            "channel": True,
            "type": float,
        },
        "TEMPERATURE_SET": {
            "SET": "set Temp_Set {}",
            "QUERY": "get Temp_Set",
            "FIND": "Temp_Set",
            "verifier": is_numeric(min=-40, max=40),
            "channel": True,
            "default": TEMPERATURE,
            "type": float,
        },
        "TEMPERATURE_WATER": {
            "QUERY": "get Temp_W",
            "FIND": "Temp_W",
            "channel": True,
            "type": float,
        },
        "THROTTLE_N2": {
            "SET": "cmd throttleN2{}",
            "READ": "VAR",
            "POS": 8,
            "type": int,
            "ASSERT": "T",
            "verifier": is_in("On", "Off"),
            "default": "Off",
        },
        "RUNTIME": {
            "READ": "Env",
            "POS": 7,
            "type": int,
        },
        "VOLTAGE": {
            "SET": "set ControlVoltage_Set {}",
            "QUERY": "get ControlVoltage_Set",
            "FIND": "ControlVoltage",
            "verifier": is_numeric(min=-12, max=12),
            "channel": True,
            "default": CONTROL_VOLTAGE,
            "type": float,
        },
        "VOLTAGE_PROBE": {
            "QUERY": "get vprobe",
            "FIND": "vprobe",
            "channel": True,
            "type": str,
        },
    }

    class MeasureChannel(Instrument.MeasureChannel):
        @property
        def status(self):
            return 0

        @property
        def value(self):
            if self._measure_type == "VOLTAGE":
                # this returns a comma separated value
                return self._instrument.query("VOLTAGE_PROBE", channel=self._channel)
            elif self._measure_type == "TEMPERATURE":
                return self._instrument.read("TEMPERATURE_MEASURED", self._channel)
            elif self._measure_type == "SPEED":
                return 0
            else:
                raise RuntimeError(
                    f"Unknown measurement type {self._type} "
                    f"for instrument class {type(self._instrument)}"
                )

        @property
        def pin_map(self):
            return self._instrument.get_pin_map()

        def query_all(self):
            ret = {}
            # dict if success or '-999' if fail
            voltages = self.value
            if isinstance(voltages, dict):
                for idx, pin in self.pin_map.items():
                    ret[idx] = voltages[pin]
            return ret

    class TemperatureChannel(Instrument.TemperatureChannel):
        """TemperatureChannel implementation for TECs."""

        @property
        def status(self):
            """
            :returns: Measurement Status Register value.
            """
            return 0

        @property
        def state(self):
            return self._instrument.query("POWER", self._channel)

        @state.setter
        def state(self, value):
            self._instrument.set("POWER", value, self._channel)

        @property
        def temperature(self):
            return self._instrument.query("TEMPERATURE_SET", self._channel)

        @temperature.setter
        def temperature(self, value):
            self._instrument.set("TEMPERATURE_SET", value, self._channel)

        @property
        def speed(self):
            return 0

        @speed.setter
        def speed(self, value):
            return

    def __init__(
        self,
        resource,
        off_on_disconnect=False,
        clientID="coldbox",
        username="",
        password="",
        init=False,
        sim=False,
    ):

        if sim:
            raise NotImplementedError(
                f"{type(self).__name__} has no simulation backend"
            )

        super().__init__(resource)

        resources = resource.split("::")
        if len(resources) != 4 or resources[0] != "TCPIP" or resources[3] != "SOCKET":
            raise ValueError(
                f"{resource}: invalid resource string "
                "(hint: TCPIP::<IP_OR_NAME>::<PORT>::SOCKET)"
            )

        self.hostname = resources[1]
        self.port = int(resources[2])

        self.off_on_disconnect = off_on_disconnect
        self.clientID = clientID
        self.mqtt_client = PSIColdboxClient(
            hostname=self.hostname,
            clientID=clientID,
            port=self.port,
            username=username,
            password=password,
            off_on_disconnect=off_on_disconnect,
        )

        if init:
            self.__enter__()
            self.load_variables_from_icicle()
            self.__exit__()

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        self.mqtt_client.connect()
        self._connected = True
        return self

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        if self.off_on_disconnect:
            self.off()
            print("turning off")
        self.mqtt_client.disconnect()
        self._connected = False

    # ToDo: remove once tessie float interpretation is fixed ...
    @staticmethod
    def _catch_wrong_type(expected_type, val):
        ret = 0
        try:
            ret = expected_type(val)
        except ValueError:
            logger.debug(f"Expected {expected_type}, got {val}. Trying to fix.")
            if (expected_type == int or expected_type == bool) and val == "1.4013e-45":
                ret = 1
            else:
                raise ValueError(
                    f"Conversion of returned value failed. "
                    f"Expected {expected_type}, got {val}. "
                    "Please report to ICICLE developer."
                )
        return ret

    def connected(self):
        return self._connected

    @acquire_lock()
    @retry_on_fail()
    def reset(self, *args, **kwargs):
        pass

    # Checks if a channel is valid or not
    def validate_channel(self, channel, raise_exception=True):
        try:
            if int(channel) in range(9):
                return True
            else:
                raise ChannelError()
        except Exception:
            if raise_exception:
                raise ChannelError(f"{channel} is not a valid channel")
            else:
                return False

    # Checks channel argument and turns it valid
    def _validate_channel(self, channel):
        if channel in ["ALL", "All", "0", 0]:
            return 0
        else:
            self.validate_channel(channel)
            return channel

    @acquire_lock()
    @retry_on_fail()
    def on(self, channel=0):
        return self.set("POWER", "on", channel=channel, no_lock=True)

    @acquire_lock()
    @retry_on_fail()
    def off(self, channel=0):
        return self.set("POWER", "off", channel=channel, no_lock=True)

    @acquire_lock()
    @retry_on_fail()
    def interlocked(self):
        return self.read("INTERLOCK")

    # Sets a value to setting and reads back the value
    @acquire_lock()
    @retry_on_fail()
    def set(self, setting, value, channel=0):
        setting = setting.upper()

        # Checks if setting is known
        if setting not in type(self).SETTINGS:
            raise ValidationError(
                f"Setting {setting} not found for device {type(self).__name__}. "
                "Cannot call set()."
            )

        # Checks if setting can be set
        setting_ = type(self).SETTINGS[setting]
        if "SET" not in (setting_):
            raise ValidationError(
                f"No command found for SET {setting} for device "
                f"{type(self).__name__} - is query-only? Cannot call set()."
            )

        # Syntax for Power is Power_On or Power_Off and therefore different.
        # Syntax for ThrottleN2 is ThrottleN2On or ThrottleN2Off
        if setting in ["POWER", "THROTTLE_N2"]:
            if value in ["on", "ON", 1, True, "1"]:
                value = "On"
            if value in ["off", "OFF", 0, False, "0"]:
                value = "Off"

        # Syntax for FLUSH and RINSE is FLUSH on/off
        elif setting in ["FLUSH", "RINSE"]:
            if value in ["on", "On", "ON", 1, True, "1"]:
                value = "on"
            if value in ["off", "Off", "OFF", 0, False, "0"]:
                value = "off"

        elif setting in ["FLUSH_TOGGLE", "RINSE_TOGGLE"]:
            value = ""

        # Checks if type is correct
        elif "type" in setting_ and callable(setting_["type"]):
            value = setting_["type"](value)
        else:
            raise ValidationError(f"{value} has the wrong type.")

        # Checks if there are limitations on the input
        if "verifier" in setting_:
            verifier = setting_["verifier"]
            try:
                value_ = verifier(value)
            except ValidationError as ve:
                logger.debug(f"ValidationError caught with message: {ve}")
                raise ValidationError(
                    f"Value {value} does not pass verifier "
                    f"{verifier.__name__} for set() on setting {setting} "
                    f"for device {type(self).__name__}.\n"
                    f"Allowable values: {verifier.allowable}"
                )
        else:
            value_ = value

        # Loads command and checks if channel is valid
        command = setting_["SET"].format(value_)
        channel = self._validate_channel(channel)

        if channel != 0 and "channel" not in setting_:
            raise ChannelError(
                f"Command for SET {setting} does not support channels."
                f"{type(self).__name__} - has no channels? Cannot call set()."
            )
        command = f"{command} tec {channel}"

        # Command is executed
        logger.debug(f'Setting "{command}"')
        self.execute(command, no_lock=True)
        time.sleep(self.SMALL_DELAY)

        # ToDo: remove once tessie has query for throttleN2
        if setting == "THROTTLE_N2":
            return self.read(setting, channel)

        # Query is executed
        return self.query(setting, channel, no_lock=True, attempts=1)

    # Requests a value and reads it back
    @acquire_lock()
    @retry_on_fail()
    def query(self, setting, channel=0):
        setting = setting.upper()

        # Checks if setting is known
        if setting not in type(self).SETTINGS:
            raise ValidationError(
                f"Setting {setting} not found for device {type(self).__name__}. "
                "Cannot call query()."
            )
        setting_ = type(self).SETTINGS[setting]

        # Checks if setting can be queried
        if "QUERY" not in setting_ or "FIND" not in setting_:
            raise ValidationError(
                f"Command for QUERY {setting} not found for device "
                f"{type(self).__name__} - is SET-only? Cannot call query()."
            )

        # Loads command and checks if channel is valid
        command = setting_["QUERY"]
        find = setting_["FIND"]
        channel = self._validate_channel(channel)

        if channel != 0:
            if setting == "VOLTAGE_PROBE":
                command = f"{command}{channel}"
            elif "channel" not in setting_:
                raise ChannelError(
                    f"Command for SET {setting} does not support channels."
                    f"{type(self).__name__} - has no channels? Cannot call set()."
                )
            else:
                command = f"{command} tec {channel}"
        elif setting == "VOLTAGE_PROBE":
            raise ChannelError(
                "Channel 0 or 'ALL' is not possible for VOLTAGE_PROBE setting."
            )

        logger.debug(f'Querying "{command}" and waiting for "{find}"')

        # Command is executed. Sessions waits for a reply starting with param:find
        ret = (
            self.mqtt_client.publish_and_receive(command, find)
            .replace(" ", "")
            .split("=")[1]
        )
        logger.debug(f"Query returned: {ret}")

        # If repl contains "," it is an array. Reply will be a list of arguments.
        # Arguments are converted to expected datatype.

        if "," in ret:
            ret = ret.split(",")
            for i, val in enumerate(ret):
                if "type" in setting_ and callable(setting_["type"]) and val != "":
                    ret[i] = self._catch_wrong_type(setting_["type"], val)
        else:
            if setting in ["FLUSH", "RINSE", "FLUSH_TOGGLE", "RINSE_TOGGLE"]:
                if ret == "on":
                    ret = 1
                if ret == "off":
                    ret = 0
            elif "type" in setting_ and (setting_["type"]):
                ret = self._catch_wrong_type(setting_["type"], ret)

        time.sleep(self.SMALL_DELAY)

        if setting == "VOLTAGE_PROBE":
            if type(ret) is list:
                dictionary = {type(self).DEFAULT_PIN_MAP[i]: ret[i] for i in range(10)}
                ret = dictionary

        return ret

    # Reads a value from the monitor stream if possible, else uses query.
    def read(self, setting, channel=0):
        setting = setting.upper()

        # Checks if setting is known and in monitor stream. Else uses query
        if setting not in type(self).SETTINGS:
            raise ValidationError(
                f"Setting {setting} not found for device {type(self).__name__}. "
                "Cannot call read()."
            )

        if setting in ["SOFTWARE_VERSION", "VOLTAGE_PROBE"]:
            raise ValidationError(
                f"Setting {setting} can not be read for device {type(self).__name__}. "
                "Cannot call read(). Try query() instead."
            )

        setting_ = type(self).SETTINGS[setting]

        # Checks, what will indicate the requested value
        if "READ" in setting_:
            find = setting_["READ"]
        elif "FIND" in setting_:
            find = setting_["FIND"]
        else:
            raise ValidationError(
                f"Command for READ / FIND {setting} not found for device "
                f"{type(self).__name__} - is SET-only? Cannot call read()."
            )

        # Checks if channel is valid. If POS is given, uses this as channel
        channel = self._validate_channel(channel)
        if channel != 0 and "channel" not in setting_:
            raise ChannelError(
                f"Command for read {setting} does not support channels."
                f"{type(self).__name__} - has no channels? Cannot call read()."
            )
        elif channel == 0 and "POS" in setting_:
            channel = setting_["POS"]

        # Reads the monitor and looks for command
        logger.debug(f'Waiting for "{find}"')
        ret = self.mqtt_client.receive(find).replace(" ", "").split("=")[1]
        logger.debug(f"Result found: {ret}")

        # Value is found, converts the output
        if "," in ret:
            ret = ret.split(",")

            # If channel or POS is given, returns the matching value.
            if channel != 0:
                ret = ret[channel - 1]
                if "type" in setting_ and callable(setting_["type"]) and ret != "":
                    if find == "VAR":
                        return self._catch_wrong_type(setting_["type"], ret[1:])
                    else:
                        return self._catch_wrong_type(setting_["type"], ret)
                return ret

            # If no channel is given, all of the array is transformed.
            else:
                for i, val in enumerate(ret):
                    if "type" in setting_ and callable(setting_["type"]) and val != "":
                        ret[i] = self._catch_wrong_type(setting_["type"], val)
        else:
            if "type" in setting_ and (setting_["type"]):
                ret = self._catch_wrong_type(setting_["type"], ret)

        return ret

    @acquire_lock()
    @retry_on_fail()
    def execute(self, command):
        self.mqtt_client.publish(command)

    @acquire_lock()
    @retry_on_fail()
    def reboot(self, channel):
        channel = self._validate_channel(channel)
        self.mqtt_client.publish(f"cmd Reboot tec {channel}")

    @acquire_lock()
    @retry_on_fail()
    def clear_error(self, channel):
        channel = self._validate_channel(channel)
        self.mqtt_client.publish(f"cmd ClearError tec {channel}")

    @acquire_lock()
    @retry_on_fail()
    def save_variables_on_tec(self, channel=0):
        channel = self._validate_channel(channel)
        self.mqtt_client.publish(f"cmd SaveVariables tec {channel}")

    @acquire_lock()
    @retry_on_fail()
    def load_variables_from_tec(self, channel=0):
        channel = self._validate_channel(channel)
        self.mqtt_client.publish(f"cmd LoadVariables tec {channel}")

    @acquire_lock()
    @retry_on_fail()
    def load_variables_from_icicle(self, channel=0):
        channel = self._validate_channel(channel)
        self.set("PID_KD", value=type(self).PID_KD, channel=channel, no_lock=True)
        self.set("PID_KI", value=type(self).PID_KI, channel=channel, no_lock=True)
        self.set("PID_KP", value=type(self).PID_KP, channel=channel, no_lock=True)
        self.set("PID_MIN", value=type(self).PID_MIN, channel=channel, no_lock=True)
        self.set("PID_MAX", value=type(self).PID_MAX, channel=channel, no_lock=True)

    def get_pin_map(self):
        return type(self).DEFAULT_PIN_MAP


class PSIColdboxClient(MQTTClient):
    TIMEOUT = 20000

    def __init__(
        self,
        hostname,
        clientID="coldbox",
        port=1883,
        username="",
        password="",
        ssl=False,
        off_on_disconnect=False,
    ):
        super().__init__(clientID, ssl=ssl)

        self.hostname = hostname
        self.topic_ctrl = "ctrlTessie"
        self.topic_mon = "monTessie"
        self.port = port
        self.username = username
        self.password = password
        self.off_on_disconnect = off_on_disconnect

    def connect(self):
        super().connect(self.hostname, self.port, self.username, self.password)
        self.loop_start()

    def disconnect(self):
        super().disconnect()
        self.loop_stop()

    def publish(self, msg):
        super().publish(self.topic_ctrl, msg)

    def subscribe(self, topic):
        self.client.subscribe(topic=topic)

    def unsubscribe(self, topic):
        self.client.unsubscribe(topic=topic)

    def publish_and_receive(self, msg, receive_msg) -> str:
        def on_message(client, userdata, msg):
            nonlocal receival

            input = msg.payload.decode()
            logger.debug(f"Received message: {input}")
            if receive_msg in input:
                if "get" not in input:
                    if "cmd" not in input:  # fix for "cmd GetSWVersion"
                        receival = input

        receival = None
        self.client.on_message = on_message
        self.client.subscribe(self.topic_ctrl)
        self.client.publish(self.topic_ctrl, msg)

        start = time.time()

        while receival is None:
            time.sleep(0.1)
            if time.time() - start > type(self).TIMEOUT / 1000:
                raise RuntimeError("Received no reply before timeout")

        assert receival is not None

        self.unsubscribe(self.topic_ctrl)
        return receival

    def receive(self, receive_msg) -> str:
        def on_message(client, userdata, msg):
            nonlocal receival

            input = msg.payload.decode()
            logger.debug(input)
            if receive_msg in input:
                receival = input

        receival = None
        self.client.on_message = on_message
        self.client.subscribe(self.topic_mon)

        start = time.time()

        while receival is None:
            time.sleep(0.1)
            if time.time() - start > type(self).TIMEOUT / 1000:
                raise RuntimeError("Received no reply before timeout")

        assert receival is not None

        self.unsubscribe(self.topic_mon)
        return receival
