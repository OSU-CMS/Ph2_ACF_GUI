"""TimePix4 socket control class for Spidr4 readout system."""

import logging

from pyvisa.constants import BufferOperation

from .instrument import Instrument, acquire_lock, ChannelError
from .scpi_instrument import (
    SCPIInstrument,
    is_integer,
    is_numeric,
)
from .utils.parser_utils import numeric_int, catch_failure, string_bool

logger = logging.getLogger(__name__)


# helper function for stop_run command
def tpx4_parse_stop_run(msg, _):
    msg = msg.strip()
    if "ok" in msg.lower():
        return True
    elif "triggers" in msg:
        return (int(msg.split()[0]),)
    elif "Bot bytecount" in msg and "Top bytecount" in msg and len(msg) >= 56:
        bot = int(msg[15:27].strip())
        top = int(msg[44:56].strip())
        return bot, top
    else:
        raise RuntimeError(f"Unknown response: {msg}")


@Instrument.register
class TPX4SC(SCPIInstrument, key="resource"):
    """SCPIInstrument implementation for Timepix4 readout via SPIDR4 system.

    Communicates with tpx4sc tool in Spidr4 tools package.
    """

    TIMEOUT = 3600000  # 1 hr
    """Ethernet link timeout."""

    COM_RESET = ""
    """Instrument Reset SCPI command."""

    SET_REQUIRES_READBACK = True
    """Every command must be sent as a query."""

    MEASURE_TYPES = {
        "BOT:BYTECOUNT": "Number of bytes read out from bottom chip half",
        "TOP:BYTECOUNT": "Number of bytes read out from top chip half",
    }

    SETTINGS = {
        "IDENTIFIER": {"QUERY": "get_devid"},
        "CONFIGURED": {
            "QUERY": "is_configured",
            "parser": string_bool("YES", "NO"),
        },
        "MONITORING": {
            "QUERY": "is_monitoring",
            "parser": string_bool("YES", "NO"),
        },
        "RUNNING": {
            "QUERY": "is_running",
            "parser": string_bool("YES", "NO"),
        },
        "CONFIGURE": {
            "SET": "configure",
            "parser": catch_failure(string_bool("OK", "xxxxx")),
        },
        "START_RUN": {
            "SET": "start_run {:d}",
            "verifier": is_integer(),
            "parser": lambda v, _: v.strip().split()[-1] if "OK" in v else False,
        },
        "STOP_RUN": {
            "SET": "stop_run",
            "parser": catch_failure(tpx4_parse_stop_run),
        },
        "START_MONITORING": {
            "SET": "start_mon",
            "parser": catch_failure(string_bool("OK", "xxxxx")),
        },
        "STOP_MONITORING": {
            "SET": "stop_mon",
            "parser": catch_failure(string_bool("OK", "xxxxx")),
        },
        "RESTART_READOUT": {
            "SET": "restart_readout",
            "parser": catch_failure(string_bool("OK", "xxxxx")),
        },
        "THRESHOLD": {
            "QUERY": "get_threshold",
            "SET": "set_threshold {:f}",
            "verifier": is_numeric(),
            "parser": catch_failure(
                lambda v, s: numeric_int(v.split()[-1].replace("e", ""), s)
            ),
            "units": "e",
        },
        "USER_DATA": {
            "SET": "user_data {:s}",
            "parser": catch_failure(string_bool("OK", "xxxxx")),
        },
        "QUIT": {
            "SET": "/q",
            "parser": catch_failure(string_bool("OK", "xxxxx")),
        },
    }
    """Settings dictionary with all Set/Query combinations."""

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for the tpx4sc."""

        @property
        def status(self):
            """
            :returns: Connection?
            :rtype: int
            """
            return int(bool(self._instrument.identify()))

        @property
        def value(self):
            m = self._instrument.measure()
            if self._measure_type == "BOT:BYTECOUNT":
                return m[0]
            if self._measure_type == "TOP:BYTECOUNT":
                return m[1]

    def __init__(self, resource="ASRL30::INSTR", sim=False):
        """
        .. Warning: the ``resource`` keyword argument is mandatory and must be
            explicitly specified - failing to do so will result in an error since the
            Multiton metaclass on VisaInstrument masks this default value for
            ``resource``.

        :param resource: VISA Resource address. See VISA docs for more info.
        """
        super().__init__(resource, sim=sim)

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Initialises connection to tpx4sc.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `TPX4SC` object in activated state.
        """
        super().__enter__(recover_attempt=recover_attempt, no_lock=True)
        return self

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Closes connection to tpx4sc.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.
        """
        super().__exit__(exception_type, exception_value, traceback, no_lock=True)

    def validate_channel(self, channel, raise_exception=True):
        """Check if an channel exists on this device. Only successful if `channel == 1`.

        :param channel: Channel number to validate as an input
        """
        if channel == 1:
            return True
        else:
            if raise_exception:
                raise ChannelError(
                    f"Channel {channel} does not exist or is not enabled on this "
                    "device."
                )
            else:
                return False

    @acquire_lock()
    def set(self, setting, *value, **kwargs):
        """Set `setting` on instrument to `value`, and read-back using equivalent
        `query()` if available.

        :param setting: key in class dictionary SETTINGS.
        :param value: target value for `setting`.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `set_channel`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: read-back value if `query()` available, else whether number of bytes
            written during `set()` meets expectation
        """
        if not self.sim:
            self._instrument.flush(
                BufferOperation.discard_read_buffer
                | BufferOperation.discard_receive_buffer
            )
        return super().set(
            f"{setting}", *value, no_lock=True, check_readback_only="parse", **kwargs
        )

    @acquire_lock()
    def query(self, setting, *params, **kwargs):
        """Query `setting` on instrument.

        :param setting: key in class dictionary SETTINGS.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `query`-call is nested within).
        :param attempts: how many retries to give `query` command.

        :return: data returned by device for given query.
        """
        if not self.sim:
            self._instrument.flush(
                BufferOperation.discard_read_buffer
                | BufferOperation.discard_receive_buffer
            )
        return super().query(f"{setting}", *params, no_lock=True, **kwargs)

    @acquire_lock()
    def status(self):
        """Check status of tpx4sc.

        :return: status string.
        """
        # uses lock from sub-call
        if self.query("RUNNING", no_lock=True) is True:
            return "RUNNING"
        elif self.query("MONITORING", no_lock=True) is True:
            return "MONITORING"
        elif self.query("CONFIGURED", no_lock=True) is True:
            return "CONFIGURED"
        else:
            return "UNCONFIGURED"

    @acquire_lock()
    def measure(self):
        """Measure data throughput in top and bottom chip halves.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `measure`-call is nested within).

        :return: tuple `(bottom_half_bytes_read, top_half_bytes_read)`.
        """
        if not self.query("RUNNING", no_lock=True):
            return (0, 0)
        self._instrument.flush(
            BufferOperation.discard_read_buffer | BufferOperation.discard_receive_buffer
        )
        value = self._instrument.read()
        # "Bot bytecount: ____________  Top bytecount: ____________"
        if "Bot bytecount" in value and "Top bytecount" in value and len(value) >= 56:
            bot = int(value[15:27].strip())
            top = int(value[44:56].strip())
            return (bot, top)

        if "L1A" in value:
            toks = value.split("|")
            vals = [int(tok.strip().split()[1]) for tok in toks]
            return tuple(vals)

        raise RuntimeError(f"Expected byte counts but got {value}")
