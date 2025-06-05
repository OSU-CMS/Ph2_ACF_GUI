"""AdcBoard class for probe cards to measure voltage on RD53B quad-chip modules.
Different versions of the probe card will be produced in future.

This code currently supports V2 of the AdcBoard.

Regarding the channel numbers of the AdcBoard, there are 18 channels.
In accordance with the icicle philosophy, the channel numbers start at 1,
even though, this might be different in the schematics.
Also, there are 16 voltage channels and 2 temperature channels:

.. code-block::

    Temperature: 9,18

In the MeasureChannel class it is however possible to assign the
measure type freely to any channel.
"""

import time

import struct
from .instrument import Instrument, acquire_lock, ChannelError
from .visa_instrument import VisaInstrument, retry_on_fail_visa
import pyvisa


@Instrument.register
class AdcBoard(VisaInstrument, key="resource"):
    """AdcBoard class.

    This board is designed to be used with a probe card to measure voltages on RD53B
    quad-chip modules. This is required for SLDO scans.
    """

    BAUD_RATE = 115200
    """Serial link Baud rate."""
    TIMEOUT = 100000  # 10 seconds
    """Serial link timeout (ms)."""

    WRITE_REMINATION = ""
    READ_TERMINATION = ""
    DATA_BITS = 8
    STOP_BITS = 10
    PARITY = 0

    N_CHANNELS = 18

    TEMP_CHANNELS = [9, 18]

    DEFAULT_PIN_MAP = {
        1: "VDDA_ROC2",
        2: "-",
        3: "VDDD_ROC2",
        4: "-",
        5: "VDDA_ROC3",
        6: "-",
        7: "VDDD_ROC3",
        8: "-",
        10: "VIN",
        11: "-",
        12: "VDDA_ROC0",
        13: "-",
        14: "VDDD_ROC0",
        15: "VOFS",
        16: "VDDA_ROC1",
        17: "VDDD_ROC1",
    }
    HEAD_LEN = 8
    MSG_LEN = 90  # Message without header
    DATA_LEN = 72  # Rest is status bytes
    STAT_LEN = 8  # Status message length
    REQUEST_STUB = "5A5A{req:02X}{req:02X}{cmd:02X}{cmd:02X}0000"
    APPLICATION_STUB = "5A5A{req:02X}{req:02X}{cmd:02X}{cmd:02X}5858"

    MEASURE_TYPES = {
        "VOLTAGE": "Differential voltage on pin",
        "TEMPERATURE": "Temperature on ADC",
    }

    CMD = {"read": 0x11, "application": 0x37, "status": 0x33}

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for TTI."""

        @property
        def status(self):
            """
            :return: status as received from query_status()
            :rtype: int
            """
            # Basically update for the status
            self._instrument.query_status()
            return 1 if self._instrument._status == 0x55 else 0

        @property
        def value(self):
            """Returns the pin value measured. This always remeasures the pin value at
            call.

            :return: pin value at measurement
            :rtype: int
            """
            values = self._instrument.query_adc(refresh=True)
            return values[self._channel - 1]

        @property
        def read(self):
            """Returns the pin value last measured. This will not remeasure the pin
            value. Mostly used, when an array of pins is read out.

            This function is not included in `measurechannel_cli`,
            so it is not accessible form the command line.

            :return: pin value from last measurement
            :rtype: int
            """
            values = self._instrument.query_adc(refresh=False)
            return values[self._channel - 1]

        def channel_name(self):
            if self._channel in self.TEMP_CHANNELS:
                return "Temperature"
            else:
                return self._instrument._PIN_MAP[self._channel]

    def __init__(self, resource="ASRL21::INSTR", pin_map=None, sim=False):
        """
        .. Warning: the ``resource`` keyword argument is mandatory and
        must be explicitly specified - failing to do so will result in
        an error since the Multiton metaclass on VisaInstrument masks
        this default value for ``resource``.

        :param resource: VISA Resource address. See VISA docs for more info.
        :param pin_map: Can be used to clean up the printout
                and remove disconnected pins. Depends on the used module.
                Currently only generic (all pins printed) and 'QUAD'
                implemented.
        :param disconnect_on_exit: Whether to set connected pin to
                `OFF` on device close.
        """
        super().__init__(resource, sim=sim)
        self._pin_map = self._build_pin_map(pin_map)
        self.nRequest = 0
        # timestamp from when the last measurement was performed
        self._age_data = 0
        self._values = None
        self._status = 0
        self._configured = False

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Initialises connection to relay board.

        :param recovery_attempt: Whether this is a recovery attempt by
                retry_on_fail.

        :return: `RelayBoard` object in activated state.
        """
        super().__enter__(self, no_lock=True)
        if self._sim:
            self._instrument.read_termination = ""
            self._instrument.write_termination = "\n"
        else:
            self._instrument.read_termination = ""
            self._instrument.write_termination = ""
        time.sleep(0.3)
        if not self._configured:
            if not self.sim:
                self._instrument.flush(pyvisa.constants.VI_READ_BUF)
            self._send_command(self._setApplication())
            self._instrument.read_bytes(self.HEAD_LEN)  # Just to empty buffer
            self._configured = True
        return self

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Closes connection to relay board.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.
        """
        super().__exit__(exception_type, exception_value, traceback, no_lock=True)

    @acquire_lock()
    @retry_on_fail_visa(exceptions=[pyvisa.VisaIOError, ValueError])
    def query_adc(self, refresh=True):
        """Measures once the full array of pins connected to the AdcBoard.

        :return: voltage and temperature values (converted) in separate lists.
        """
        if refresh or time.time() - self._age_data > 1:
            command = self._getNextRequest("read")
            self._send_command(command)
            head = self._instrument.read_bytes(self.HEAD_LEN)
            self._checkHead(head)
            rb = self._instrument.read_bytes(self.MSG_LEN)
            val = [
                struct.unpack("f", rb[i : i + 4])[0] for i in range(0, self.DATA_LEN, 4)
            ]
            self._checkData(val)
            self._age_data = time.time()
            self._values = val

        values = [self._values[i] for i in range(0, self.N_CHANNELS)]
        # temps = [self._values[i] for i in self.TEMP_CHANNELS]

        return values

    @acquire_lock()
    @retry_on_fail_visa(exceptions=[pyvisa.VisaIOError, ValueError])
    def query_status(self):
        """Query to check the status of the ADC_Board. Used currently mainly for
        identification of the device. If not an ADC_Board, either no answer will be
        given or `_checkHead()` will fail.

        :return: production id, serial id, hardware version, hardware assembly, software
            version,
        """
        self._send_command(self._getNextRequest("status"))
        head = self._instrument.read_bytes(self.HEAD_LEN)
        self._checkHead(list(head))
        rb = list(self._instrument.read_bytes(self.STAT_LEN))
        prod_id = hex(rb[0] << 8 | rb[1])
        ser_id = hex(rb[2] << 8 | rb[3])
        hw_vers = hex(rb[4])
        hw_assembly = hex(rb[5])
        sw_vers = hex(rb[6] << 8 | rb[7])

        return prod_id, ser_id, hw_vers, hw_assembly, sw_vers

    def identify(self):
        """Command to identify the module. calls `query_status()` and prints out
        production and serial number for identification of the board. Currently bad
        handler for case of different device. In this case, call most likely freezes due
        to no feedback.

        :print: Production number and serial number of adc_board
        """
        (
            prod_id,
            ser_id,
            _,
            _,
            _,
        ) = self.query_status()
        prod_chars = [chr(byte) for byte in bytes.fromhex(prod_id[2:])]
        print(
            f"ADC_Board, Production number {prod_chars[0]}{prod_chars[1]}, "
            + f"Serial number {ser_id}"
        )

    def set(self, setting, value, **kwargs):
        """ "Fake" entry point to extend common interface as much as possible.

        No values to actually set on this device - therefore empty function.
        uses lock from sub-command - pass kwargs
        """
        return True

    def query(self, setting=False, **kwargs):
        """ "Fake" entry point to extend common interface as much as possible.

        :param no_lock: override `acquire_lock`
                (e.g. if lock already taken by function that `set`-call is
                nested within).
        :param attempts: how many retries to give `set` command.
        """

        return self.query_adc(**kwargs)

    def monitor(self, repeats=0):
        """Monitoring the pin voltages and temperatures.

        :param repeats: how many measurements to do before stopping. If 0, indefinite
            repeats
        """
        itnum = 0
        while itnum < repeats or repeats == 0:
            now = time.asctime()
            values = self.query_adc()
            print(now)
            print(
                "Voltage\n"
                + "\n".join(
                    [
                        "{: >2s} {: .3f} V".format(self.get_pin_map()[i], values[i - 1])
                        for i in self.get_pin_map().keys()
                    ]
                )
            )
            print(
                "Temperature\n"
                + "\n".join(
                    [
                        "{: >2d} {: .2f} C".format(i, values[j - 1])
                        for i, j in enumerate(self.TEMP_CHANNELS)
                    ]
                )
            )
            # time.sleep(1)
            if repeats != 0:
                itnum += 1

    def get_pin_map(self):
        """Getter method for the pin_map for a specific moduel type.

        TODO: When we have different modules (e.g. double modules) -
                make the returned pinmap dependent on :param pin_map
        """
        return self._pin_map

    def validate_channel(self, channel, raise_exception=True):
        """Check if an output exists on this device.

        :param channel: Channel number to validate as an output.
        """
        if channel >= 1 and channel <= 18:
            return True
        else:
            if raise_exception:
                raise ChannelError(
                    f"Channel {channel} does not exist or is not enabled on this "
                    "device."
                )
            else:
                return False

    def _send_command(self, command):
        if self._sim:
            self._instrument.write(str(command))
        else:
            self._instrument.write_raw(command)

    def _build_pin_map(self, pin_map):
        """Function to build a pin map for different modules.

        To be expanded for different types.
        """
        # making sure to capitalize the input if there is any.
        pin_map = None if pin_map is None else pin_map.upper()
        if pin_map == "QUAD":
            newmap = {}
            for key in [12, 14, 16, 17, 1, 3, 5, 7, 10]:
                newmap[key] = self.DEFAULT_PIN_MAP[key]
            return newmap
        elif pin_map == "DOUBLE":
            newmap = {}
            for key in [12, 14, 16, 17, 10]:
                newmap[key] = self.DEFAULT_PIN_MAP[key]
            return newmap
        elif pin_map is None:
            return self.DEFAULT_PIN_MAP
        else:
            print("unknown pin_map type")

    def _getNextRequest(self, req_type):
        """Helper method to create a request word.

        :param req_type: bytes to indicate type of request (stored in CMD dict)
        :return: command to send a request to the adc_board
        """
        req = self.REQUEST_STUB.format(req=self.nRequest % 256, cmd=self.CMD[req_type])
        self.nRequest += 1
        return bytes.fromhex(req)

    def _setApplication(self):
        """Helper method to create the application command word.

        :return: command to set the application mode on adc_board
        """
        app = self.APPLICATION_STUB.format(
            req=self.nRequest, cmd=self.CMD["application"]
        )
        self.nRequest += 1
        return bytes.fromhex(app)

    def _checkHead(self, head):
        """Function to check the measurement quality. Raises errors, if something with
        the header is not okay.

        :param head: First 8 bytes of a message received from the AdcBoard
                     0,1: 0x5A0x5A (start sequence)
                     2  : board id   ---> Ignored
                     3  : packet number of request
                     4  : command status byte --> filled into self._status
                     5  : received packet command ---> Ignored
                     6,7: active channels on ADC 1,2
                     In case of an error, bytes 5-7 are set to 0.
                     And byte 4 returns error code

        :return: True, if message is fine, raise ValueError else.
        """

        def splitReg(reg):
            return reg & 0xFF00, reg & 0xFF

        h1, h2, bid, r_num, status, req_cmd, err_1, err_2 = list(head)
        self._status = status  # To be able to read the status of the adc_board
        if h1 != 0x5A or h1 != h2:
            raise ValueError("Header mismatch ", hex(h1), hex(h2))
        # elif r_num != self.nRequest - 1 % 255:
        #    raise ValueError(
        #        f"Wrong request number received.",
        #        f"Expected {self.nRequest-1} :",
        #        r_num,
        #    ) # The adc_board currently does not send the correct number
        elif status != 0x55:
            if status == 0x66:
                err_msg = "start"
            elif status == 0x77:
                err_msg = "number"
            elif status == 0x88:
                err_msg = "command"
            elif status == 0x99:
                err_msg = "length"
            elif status == 0xAA:
                err_msg = "application code"
            else:
                err_msg = "undefined error"
            raise ValueError(f"Request packed {err_msg} error", status)
        elif err_1 not in [1, 9] or err_2 not in [1, 9]:
            raise ValueError(
                "Issue with ADC cannel activation. Expects (9,9)", err_1, err_2
            )
        return True

    def _checkData(self, res):
        """Checks the integrity of the data. Will be extended. Currently only the length
        of the data is checked.

        :param res: Rest of the message, always interpreted in packages of 4 bytes
        :return: True, if data has correct lenght.
        """
        if len(res) != 18:
            raise ValueError("Invalid length: " + str(len(res)))
        return True

    @acquire_lock()
    def off(self):
        """ "Fake" entry point to keep common interface.

        This device does not have settings to turn on/off except for physically pulling
        the plug. Function not actively used.
        """
        return False

    @acquire_lock()
    def on(self):
        """ "Fake" entry point to keep common interface.

        This device does not have settings to turn on/off except for physically pulling
        the plug. Function not actively used.
        """
        return True


# end class AdcBoard
