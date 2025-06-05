#! /usr/bin/env python
#
# Copyright Solarflare Communications Inc., 2012-13
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Solarflare Communications Inc. nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL SOLARFLARE COMMUNICATIONS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""Control a BINDER oven (MK 53) remotely over MODBus through TCP/IP.

This is not standard MODBus-TCP, but rather BINDER's variant of MODBus
tunnelled using a Lantronix XPort-03 adaptor.  It is described in the
document "BINDER Interface Technical Specifications" (Art. No. 7001-0242),
referred to as the 'techspec'

References to 'man. secn' are to sections of the BINDER MK Operating Manual
(Art. Nr. 7001-0028)

class BinderCtl is the main class
  Key methods:
    __init__: the hostname is the only required argument
    get_mode: Get the current operating mode of the oven
    set_mode_idle: Set the oven to Idle mode
    set_mode_active: Activate the oven
    get_temp: Get the current oven temperature
    get_setpoint: Get the current temp. setpoint
    set_setpoint: Set the temperature setpoint
    wait_for_temp: Block until the oven reaches its setpoint
    temp_ready_tester: Create a closure to test whether the oven has
     reached its setpoint (complicated; see its docstring)
  Pitfalls:
    Don't call set_mode_active before set_setpoint, or the oven might start
     up the wrong thing (eg. the fridge when you wanted the heater).  The
     lag-time can be several minutes

class SafetyException (derived from Exception) is the base class for various
 exceptions which are raised to indicate that oven operation may be unsafe
  Derived classes:
    SafetyAlarmException: Alarm on oven
    SafetyNoteException: Note (minor alarm) on oven
    SafetyDoorException: Oven door is open
    SafetyTempException: Requested setpoint outside rated range

class ModbusException (derived from Exception) is the base class for various
 exceptions which are raised to indicate a problem communicating over MODBus
  Derived classes:
    ModbusShortMessageException: Response too short, get more bytes
    ModbusFunctionException: Function code mismatch
    ModbusCrcException: CRC16 checksum mismatch
    ModbusErrorException: Remote sent an error response
    ModbusBadResponseException: Response couldn't be parsed but dunno why

class OvenStatusException (derived from Exception) is the base class for
 various exceptions which are raised by OvenCtl.wait_for_temp to indicate a
 change in the oven's status that makes waiting inappropriate
  Derived classes:
    OvenIdleException: Oven is in Idle mode
    OvenSetChangedException: Temperature setpoint was changed
"""
import struct
import time
import socket

BINDER_PORT = 10001

MB_SLAVEADDR = 1

# MODBus function codes
MB_FN_READN = 0x03
MB_FN_READN_ALT = 0x04
MB_FN_WRITE = 0x06
MB_FN_WRITEN = 0x10


def mb_fn_is_readn(mbfn):
    """Determine if a MODBus function code is "Read n words".

    There are two such function codes, 0x03 and 0x04.  I've only ever seen 0x03 from our
    oven, but the techspec says 0x04 can happen too
    """
    return mbfn in [MB_FN_READN, MB_FN_READN_ALT]


# MODBus error codes
MB_EE_FN = 1
MB_EE_ADDR = 2
MB_EE_RANGE = 3
MB_EE_BUSY = 4  # (should never happen: "Error code 4 (slave not ready) is
# not implemented in the controller since the controller always responds
# within 250 ms to a valid data request." (secn 2.7))
MB_EE_ACCESS = 5
#  and their descriptions
MB_ENAMES = {
    MB_EE_FN: "invalid function",
    MB_EE_ADDR: "invalid parameter address",
    MB_EE_RANGE: "parameter value outside range of values",
    MB_EE_BUSY: "slave not ready",
    MB_EE_ACCESS: "write access to parameter denied",
}


class ModbusException(Exception):
    """Indicate that a MODBus message was in some way invalid or unexpected."""

    def __init__(self, args, msgbytes):
        """Construct a ModbusException.

        Parameters:
            args: further information about the exception (eg. a text string)
            msgbytes: the full text of the message
        """
        self.msgbytes = msgbytes
        self.args = args

    def __str__(self):
        return "Invalid MODBus message (%s).  Bytes: %s" % (self.args, self.msgbytes)


class ModbusShortMessageException(ModbusException):
    """Indicate that a MODBus message was too short.

    This is used internally by the parse_*_response functions and caught by the
    OvenCtl.do_* methods.  User code should never see it.
    """

    def __init__(self, length, wanted, msgbytes):
        """Construct a ModbusShortMessageException.

        Parameters:
            length: the length of the message we got
            wanted: the length we expected the message to have
            msgbytes: the full text of the message
        """
        self.length = length
        self.wanted = wanted
        self.msgbytes = msgbytes

    def __str__(self):
        return "Wanted %s bytes, got %s" % (self.wanted, self.length)


class ModbusFunctionException(ModbusException):
    """Indicate that a MODBus message had an unexpected Function code."""

    def __init__(self, fn, expected, msgbytes):
        """Construct a ModbusFunctionException.

        Parameters:
            fn: the Function code of the message
            expected: the Function code we were expecting
            msgbytes: the full text of the message
        """
        self.fn = fn
        self.expected = expected
        self.msgbytes = msgbytes

    def __str__(self):
        return "Expected fn %02x, got %02x" % (self.expected, self.fn)


class ModbusCrcException(ModbusException):
    """Indicate that a MODBus message had an invalid CRC16."""

    def __init__(self, crc, checkcrc, msgbytes):
        """Construct a ModbusCrcException.

        Parameters:
            crc: the CRC16 enclosed in the message
            checkcrc: the CRC16 we computed for the message
            msgbytes: the full text of the message
        """
        self.crc = crc
        self.checkcrc = checkcrc
        self.msgbytes = msgbytes

    def __str__(self):
        return "Expected crc %04x, got %04x" % (self.checkcrc, self.crc)


class ModbusErrorException(ModbusException):
    """Indicate that the remote sent an error response."""

    def __init__(self, ecode, msgbytes):
        """Construct a ModbusErrorException.

        Parameters:
            ecode: the error code enclosed in the message
            msgbytes: the full text of the message
        """
        self.ecode = ecode
        self.ename = MB_ENAMES.get(self.ecode, "Unknown error")
        self.msgbytes = msgbytes

    def __str__(self):
        return "MODBus error code %d (%s)" % (self.ecode, self.ename)


class ModbusBadResponseException(ModbusException):
    """Indicate that response parsing failed for unknown reasons.

    This is used in cases where it should be impossible, as the errors that could occur
    should already have been caught and a different ModbusException raised.  If you get
    one of these, something is very wrong!
    """

    def __init__(self, msgbytes):
        """Construct a ModbusBadResponseException.

        Parameter: msgbytes: the full text of the message
        """
        self.msgbytes = msgbytes

    def __str__(self):
        return "'Impossible' MODBus error.  Bytes: %s" % self.msgbytes


class OvenStatusException(Exception):
    """Indicate that the oven status is inappropriate for waiting.

    This is used by OvenCtl.wait_for_temp to signal that the original
    desired/expected result won't ever be reached

    For statuses which might have safety implications, use
    SafetyException instead

    Derived classes:
        OvenIdleException: Oven is in Idle mode
        OvenSetChangedException: Oven setpoint was changed
    """


class OvenIdleException(OvenStatusException):
    """Indicate that the oven is idle and thus won't ever reach the setpoint."""


class OvenSetChangedException(OvenStatusException):
    """Indicate that the oven setpoint was changed from the one we were waiting for."""

    def __init__(self, new, old):
        """Construct an OvenSetChangedException.

        Parameters:
            new: the new temperature setpoint
            old: the temperature we were waiting for
        """
        self.new = new
        self.old = old

    def __str__(self):
        return "The setpoint was changed from %.2f to %.2f" % (self.old, self.new)


class SafetyException(Exception):
    """Indicate that oven operation may be unsafe in current state.

    Derived classes:
        SafetyAlarmException: Alarm on oven
        SafetyNoteException: Note (minor alarm) on oven
        SafetyDoorException: Oven door is open
        SafetyTempException: Requested setpoint outside rated range
    """


class SafetyNoteException(SafetyException):  # A note is like a minor alarm
    """Indicate a Note (a kind of minor alarm) on the oven."""

    def __init__(self, text):
        """Construct a SafetyNoteException.

        Parameter: text: the text of the Note
        """
        self.text = text

    def __str__(self):
        return "Note: %s" % self.text


class SafetyAlarmException(SafetyException):
    """Indicate an alarm state on the oven.

    This indicates the possibility of severe damage or danger if operation continues. In
    this case you should set the oven to Idle mode IMMEDIATELY. To clear the alarm it is
    necessary to use the RESET feature on the oven front control panel; ovenctl cannot
    remotely reset alarms (and arguably it shouldn't do so)
    """

    def __init__(self, text):
        """Construct a SafetyAlarmException.

        Parameter: text: the text of the Alarm
        """
        self.text = text

    def __str__(self):
        return "ALARM: %s" % self.text


class SafetyTempException(SafetyException):
    """Indicate that a requested temperature was outside rated range."""

    def __init__(self, over, temp, limit):
        """Construct a SafetyTempException.

        Parameters:
            over: set to True if temp. over max (else temp. under min)
            temp: the requested temperature setpoint
            limit: the limit which was exceeded (in whichever direction)
        """
        self.over = over
        self.temp = temp
        self.limit = limit

    def __str__(self):
        return "Temperature %.2f %s limit of %.2f" % (
            self.temp,
            "over" if self.over else "under",
            self.limit,
        )


class SafetyDoorException(SafetyException):
    """Indicate that the door of the oven is open."""


def calc_crc16(msg):  # string -> int
    """Calculate the CRC16 checksum according to secn 2.8 of the techspec."""
    crc = 0xFFFF
    for byte in msg:
        crc ^= byte
        # crc ^= ord(byte)
        for _ in range(8):
            sbit = crc & 1
            crc >>= 1
            crc ^= sbit * 0xA001
    return crc


def encode_float(value):  # float -> [int, int]
    """Encode a float into MODBus format as in secn 2.11.1 of the techspec."""
    words = struct.unpack(">HH", struct.pack(">f", value))
    return words[1], words[0]


def decode_float(value):  # [int, int] -> float
    """Decode a float from MODBus format as in secn 2.11.1 of the techspec."""
    # Yes, the words _are_ supposed to be swapped over
    return struct.unpack(">f", struct.pack(">HH", value[1], value[0]))[0]


def make_readn_request(addr, n_words):  # (int, int) -> string
    """Build a "Read more than one word" MODBus request string.

    The request is for n_words words starting from address addr

    Techspec: 2.9.1
    """
    msg = struct.pack(">BBHH", MB_SLAVEADDR, MB_FN_READN, addr, n_words)
    return msg + struct.pack("<H", calc_crc16(msg))


def parse_readn_response(msgbytes):  # string -> [int...]
    """Parse a "Read more than one word" MODBus response string.

    Returns a list of words read

    Can raise:
        ModbusException
        ModbusShortMessageException
        ModbusFunctionException
        ModbusCrcException

    Techspec: 2.9.1
    """
    if len(msgbytes) < 3:
        raise ModbusShortMessageException(len(msgbytes), None, msgbytes)
    ignore, func, n_bytes = struct.unpack(">BBB", msgbytes[:3])
    if not mb_fn_is_readn(func):
        raise ModbusFunctionException(func, MB_FN_READN, msgbytes)
    if n_bytes & 1:
        raise ModbusException("Odd number of bytes read", msgbytes)
    if len(msgbytes) < 5 + n_bytes:
        raise ModbusShortMessageException(len(msgbytes), 5 + n_bytes, msgbytes)
    (crc,) = struct.unpack("<H", msgbytes[3 + n_bytes : 5 + n_bytes])
    checkcrc = calc_crc16(msgbytes[: 3 + n_bytes])
    if crc != checkcrc:
        raise ModbusCrcException(crc, checkcrc, msgbytes)
    n_words = n_bytes >> 1
    words = []
    for word in range(n_words):
        words.extend(struct.unpack(">H", msgbytes[3 + word * 2 : 5 + word * 2]))
    return words


def make_write_request(addr, value):  # (int, int) -> string
    """Build a "Write one word" MODBus request string.

    The request is to write value to address addr

    Techspec: 2.9.2
    """
    msg = struct.pack(">BBHH", MB_SLAVEADDR, MB_FN_WRITE, addr, value)
    return msg + struct.pack("<H", calc_crc16(msg))


def parse_write_response(msgbytes):  # string -> (int, int)
    """Parse a "Write one word" MODBus response string.

    Returns (address written to, value written)

    Can raise:
        ModbusShortMessageException
        ModbusFunctionException
        ModbusCrcException

    Techspec: 2.9.2
    """
    if len(msgbytes) < 8:
        raise ModbusShortMessageException(len(msgbytes), 8, msgbytes)
    (crc,) = struct.unpack("<H", msgbytes[6:8])
    ignore, func, addr, value = struct.unpack(">BBHH", msgbytes[:6])
    if func != MB_FN_WRITE:
        raise ModbusFunctionException(func, MB_FN_WRITE, msgbytes)
    checkcrc = calc_crc16(msgbytes[:6])
    if crc != checkcrc:
        raise ModbusCrcException(crc, checkcrc, msgbytes)
    return addr, value


def make_writen_request(addr, words):  # (int, [int...]) -> string
    """Build a "Write more than one word" MODBus request string.

    The request is to write words, the list of words, to address addr

    Techspec: 2.9.3
    """
    n_words = len(words)
    msg = struct.pack(">BBHHB", MB_SLAVEADDR, MB_FN_WRITEN, addr, n_words, n_words * 2)
    for word in words:
        msg += struct.pack(">H", word)
    return msg + struct.pack("<H", calc_crc16(msg))


def parse_writen_response(msgbytes):  # string -> (int, int)
    """Parse a "Write more than one word" MODBus response string.

    Returns (address written to, number of words written)

    Can raise:
        ModbusShortMessageException
        ModbusFunctionException
        ModbusCrcException

    Techspec: 2.9.3
    """
    if len(msgbytes) < 8:
        raise ModbusShortMessageException(len(msgbytes), 8, msgbytes)
    (crc,) = struct.unpack("<H", msgbytes[6:8])
    ignore, func, addr, n_words = struct.unpack(">BBHH", msgbytes[:6])
    if func != MB_FN_WRITEN:
        raise ModbusFunctionException(func, MB_FN_WRITEN, msgbytes)
    checkcrc = calc_crc16(msgbytes[:6])
    if crc != checkcrc:
        raise ModbusCrcException(crc, checkcrc, msgbytes)
    return addr, n_words


def parse_err_response(msgbytes):  # string -> (bool, int)
    """Test a response string to see if it's a MODBus error response.

    Returns (response is an error, error code)
    If response is not an error, error code returned is None

    Can raise: ModbusCrcException

    Techspec: 2.7
    """
    if len(msgbytes) < 5:
        return False, None
    (crc,) = struct.unpack("<H", msgbytes[3:5])
    ignore, func, ecode = struct.unpack(">BBB", msgbytes[:3])
    if not func & 0x80:
        return False, None
    checkcrc = calc_crc16(msgbytes[:3])
    if crc != checkcrc:
        # we've already established that it _is_ an err_response
        raise ModbusCrcException(crc, checkcrc, msgbytes)
    return True, ecode


class BinderCtl(object):
    """Control a climate chamber or oven."""

    def __init__(self, hostname, port=BINDER_PORT, timeout=10, retries=10):
        """Construct an OvenCtl instance to control an oven.

        Parameters:
            hostname: the hostname or IP address of the oven
            port: the port to connect on (default 10001)
            timeout: the connect timeout in seconds (default 2.5)
            retries: the number of times to retry connection
        """
        self.hostname = hostname
        self.port = port
        self.timeout = timeout
        self.retries = retries

    def connect_with_retry(self):
        if not self.retries:
            return socket.create_connection((self.hostname, self.port), self.timeout)
        delay = 0.01
        for i in range(self.retries):
            try:
                sock = socket.create_connection(
                    (self.hostname, self.port), self.timeout
                )
                return sock
            except socket.error as err:
                left = self.retries - i - 1
                print("%s; %d tries left" % (err, left))
                if left == 0:
                    raise err
            time.sleep(delay)
            delay *= 2

    def do_readn(self, addr, n_words):
        """Read n_words words from the oven at address addr.

        Returns a list of words read

        Can raise: ModbusException: trouble at t' mill
        """
        read_req = make_readn_request(addr, n_words)
        sock = self.connect_with_retry()
        try:
            sock.send(read_req)
            # slave_addr, function, n_bytes, value(n_words)(2), crc(2)
            resp_len = 5 + (n_words * 2)
            good_resp = False
            resp = bytes()
            while not good_resp:
                if len(resp) >= resp_len:
                    raise ModbusBadResponseException(resp)
                resp += sock.recv(resp_len - len(resp))
                iserr, e = parse_err_response(resp)
                if iserr:
                    raise ModbusErrorException(e, resp)
                try:
                    data = parse_readn_response(resp)
                except ModbusShortMessageException:
                    continue
                good_resp = len(data) == n_words
            return data
        finally:
            sock.close()

    def do_write(self, addr, data):
        """Write data, a single word, to address addr on the oven.

        Can raise: ModbusException: trouble at t' mill
        """
        write_req = make_write_request(addr, data)
        sock = self.connect_with_retry()
        try:
            sock.send(write_req)
            resp_len = 8  # slave_addr, function, addr(2), data(2), crc(2)
            good_resp = False
            resp = bytes()
            while not good_resp:
                if len(resp) >= resp_len:
                    raise ModbusBadResponseException(resp)
                resp += sock.recv(resp_len - len(resp))
                iserr, e = parse_err_response(resp)
                if iserr:
                    raise ModbusErrorException(e, resp)
                try:
                    resp_addr, resp_data = parse_write_response(resp)
                except ModbusShortMessageException:
                    continue
                good_resp = (resp_addr == addr) and (resp_data == data)
            return
        finally:
            sock.close()

    def do_writen(self, addr, data):  # data is a list of WORDS
        """Write data, a list of words, to the oven, starting at address addr.

        Can raise: ModbusException: trouble at t' mill
        """
        write_req = make_writen_request(addr, data)
        sock = self.connect_with_retry()
        try:
            sock.send(write_req)
            resp_len = 8  # slave_addr, function, addr(2), length(2), crc(2)
            good_resp = False
            resp = bytes()
            while not good_resp:
                if len(resp) >= resp_len:
                    raise ModbusBadResponseException(resp)
                resp += sock.recv(resp_len - len(resp))
                iserr, e = parse_err_response(resp)
                if iserr:
                    raise ModbusErrorException(e, resp)
                try:
                    resp_addr, resp_words = parse_writen_response(resp)
                except ModbusShortMessageException:
                    continue
                good_resp = (resp_addr == addr) and (resp_words == len(data))
            return
        finally:
            sock.close()

    def read_float(self, addr):
        """Read a floating-point value from the oven at address addr.

        Can raise: ModbusException
        """
        data = self.do_readn(addr, 2)
        return decode_float(data)

    def write_float(self, addr, value):
        """Write a floating-point value to the oven at address addr.

        Can raise: ModbusException
        """
        self.do_writen(addr, encode_float(value))

    def read_int(self, addr):
        """Read an integer value from the oven at address addr.

        Can raise: ModbusException
        """
        data = self.do_readn(addr, 1)
        return data[0]

    def write_int(self, addr, value):
        """Write an integer value to the oven at address addr.

        Can raise: ModbusException
        """
        self.do_write(addr, value)
