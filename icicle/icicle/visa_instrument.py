"""PyVISA Instrument interface and associated decorator tools.

This module contains the `VISAInstrument` interface and the `@retry_on_fail_visa`
decorator.
"""

import pyvisa
from pyvisa import constants  # noqa: F401
import serial.serialutil
import logging
import pathlib
import inspect

from .instrument import Instrument, Multiton, acquire_lock, retry_on_fail

import icicle.sim as sim

SIM_PATH = pathlib.Path(inspect.getfile(sim)).parent.resolve()

logger = logging.getLogger(__name__)


def retry_on_fail_visa(
    attempts=3, exceptions=(pyvisa.VisaIOError, serial.serialutil.SerialException)
):
    """Specific implementation of `retry_on_fail` for visa instruments, which retries on
    `VisaIOErrors` by default.

    :returns: Decorated function.
    """
    return retry_on_fail(attempts, exceptions)


class VisaInstrument(Instrument, metaclass=Multiton, key="resource"):
    """Implementation of the Instrument Class with instantiation calls to the `pyvisa`
    package to instantiate a VISA-compatible GPIB, Serial, Ethernet, ... instrument.

    .. Warning: All subclasses of VISAInstrument must have the 'key' keyword class
        argument defined to specify which '__init__' keyword argument should be used as
        the Multiton key.
    """

    def __init__(self, resource="", sim=False):
        """.. Note: All arguments must be provided as explicit keyword argument.

        :param resource: VISA resource string. See pyvisa docs for examples.
        """
        super().__init__(resource)
        self._sim = sim

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Connects to instrument using pyvisa, saves raw VISA handle as _instrument.
        Attempts to find parameters BAUD_RATE and TIMEOUT on the current class.

        :param recovery_attempt: Whether this is a recovery attempt by `retry_on_fail`.
            Should be propagated.

        :returns: Instrument object with activated context.
        """
        if not self.connected():
            kwargs = {}
            # All instrument types
            for attr in (
                "TIMEOUT",
                "READ_TERMINATION",
                "WRITE_TERMINATION",
            ):
                if hasattr(type(self), attr):
                    logger.debug(
                        f"Setting {attr.lower()} to {repr(getattr(type(self), attr))}"
                    )
                    kwargs[attr.lower()] = getattr(type(self), attr)

            # Serial only
            if self._resource.startswith("ASRL"):
                for attr in (
                    "FLOW_CONTROL",
                    "BAUD_RATE",
                    "DATA_BITS",
                    "STOP_BITS",
                    "PARITY",
                ):
                    if hasattr(type(self), attr):
                        logger.debug(
                            f"Setting {attr.lower()} to "
                            f"{repr(getattr(type(self), attr))}"
                        )
                        kwargs[attr.lower()] = getattr(type(self), attr)

            visa_backend = "@py"
            if self._sim:
                visa_backend = f"{SIM_PATH}/visa_instrument.yaml@sim"
            self._instrument = pyvisa.ResourceManager(visa_backend).open_resource(
                self._resource, **kwargs
            )
        return super().__enter__(no_lock=True, recover_attempt=recover_attempt)

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Closes connection handle to instrument.

        :param recovery_attempt: Whether this is a recovery attempt by `retry_on_fail`.
            Should be propagated.
        """
        super().__exit__(
            exception_type,
            exception_value,
            traceback,
            no_lock=True,
            recover_attempt=recover_attempt,
        )
        if not self.connected():
            self._instrument.close()

    @property
    def sim(self):
        return self._sim
