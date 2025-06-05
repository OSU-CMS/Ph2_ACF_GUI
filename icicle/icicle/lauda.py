"""Lauda class for Lauda chillers."""

from .instrument import acquire_lock, Instrument, ChannelError
from .scpi_instrument import SCPIInstrument, is_numeric, map_to, is_integer
from .utils.parser_utils import numeric_float, numeric_int, numeric_bool, int_map


@Instrument.register
class Lauda(SCPIInstrument, key="resource"):
    """SCPIInstrument implementation for Lauda chillers.

    This class has been developed using the documentation for Lauda Eco Gold thermostats
    and tested using a Lauda Proline Kryomat RP 3090 CW chiller. This should work for
    other Lauda thermostats and chillers too.
    """

    BAUD_RATE = 9600
    """Serial link Baud rate."""

    TIMEOUT = 10000  # 10 seconds
    """Serial link timeout."""

    COM_RESET = ""
    """Instrument Reset SCPI command."""

    READ_TERMINATION = "\r\n"
    """Read termination characters."""

    WRITE_TERMINATION = "\r\n"
    """Write termination characters."""

    MEASURE_TYPES = {
        "TEMP:BATH": "Bath Temperature (°C)",
        "TEMP:EXT": "Ext Temperature (°C)",
    }
    """Measurement types for the Lauda chiller."""

    REGISTERS = {
        "DIAGNOSTICS": "Diagnostic register",
        # [ Error | Alarm | Warning | Overtemperature | Low Level | High Level |
        #   External control value missing ] ; XXXXXXX ; X=0 good ; X=1 error
    }

    SETTINGS = {
        "IDENTIFIER": {"QUERY": "TYPE"},
        "SYSTEM_MODE": {
            "SET": "OUT_MODE_00_{}",
            "QUERY": "IN_MODE_00",
            "verifier": map_to({"LOC": "0", "REM": "0", "RWL": "1"}),
            "parser": int_map(
                {
                    0: "REM",
                    1: "RWL",
                }
            ),
        },
        "START": {"SET": "START"},
        "STOP": {"SET": "STOP"},
        "OPERATION": {"QUERY": "IN_MODE_02", "parser": numeric_bool},
        "STATUS": {"QUERY": "STATUS", "parser": numeric_int},
        "DIAGNOSTICS": {"QUERY": "STAT", "parser": numeric_int},
        "TEMPERATURE_TARGET": {
            "SET": "OUT_SP_00_{:.2f}",
            "QUERY": "IN_SP_00",
            "verifier": is_numeric(min=-90.0, max=200.0),
            "parser": numeric_float,
        },
        "TEMPERATURE_BATH": {"QUERY": "IN_PV_00", "parser": numeric_float},
        "TEMPERATURE_EXTERNAL": {"QUERY": "IN_PV_01", "parser": numeric_float},
        "TEMPERATURE_REFERENCE": {
            "SET": "OUT_MODE_01_{}",
            "QUERY": "IN_MODE_01",
            "verifier": is_integer(min=0, max=3),
            "parser": numeric_int,
        },
        "TYPE": {"QUERY": "TYPE"},
        # This goes against the idea of having a well-defined SCPIInstrument object
        # ... TODO: please replace by actual command definitions
        "COMMAND": {"SET": "{}"},
    }
    """Settings dictionary with all Set/Query SCPI combinations."""

    MEASURE_COMMAND_MAP = {
        "TEMP:BATH": "TEMPERATURE_BATH",
        "TEMP:EXT": "TEMPERATURE_EXTERNAL",
    }

    class MeasureChannel(Instrument.MeasureChannel):
        """MeasureChannel implementation for Lauda chillers."""

        @property
        def status(self):
            """
            :returns: Diagnostics returned by ``STAT`` query.
            """
            return self._instrument.query("DIAGNOSTICS")

        @property
        def value(self):
            return self._instrument.query(
                type(self._instrument).MEASURE_COMMAND_MAP.get(self._measure_type)
            )

    def __init__(self, resource="ASRL11::INSTR", sim=False):
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
        """Initialises connection to Lauda.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `Lauda` object in activated state.
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
        """Closes connection to Lauda.

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
