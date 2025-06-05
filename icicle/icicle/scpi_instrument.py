"""SCPI Instrument interface and associated decorator tools.

This module contains the `SCPIInstrument` interface, and several function factories for
creating input verifiers for SCPI set commands.

Verifiers include: `verifier_or`, `truthy`, `is_numeric`, `is_integer`.
"""

import logging
import time

from .instrument import acquire_lock
from .visa_instrument import VisaInstrument, retry_on_fail_visa

logger = logging.getLogger(__name__)


class ValidationError(RuntimeError):
    pass


def verifier_or(*verifiers):
    """Verifier factory to combine multiple verifiers in OR-like behaviour.

    :param verifiers...: multiple verifiers (functions) to combined.

    :return: Verifier function.

    .. note:: FUNCTION FACTORY
    """

    def _verifier_or(value):
        for verifier in verifiers:
            try:
                return verifier(value)
            except ValidationError:
                pass
        raise ValidationError("Did not pass any verifier in verifier_or")

    _verifier_or.allowable = '[{", OR \n".join(verifiers.allowable)}]'
    return _verifier_or


def truthy(
    true_output=1,
    false_output=0,
    true_like=(True, 1, "1", "ON", "Y", "YES", "TRUE"),
    false_like=(False, 0, "0", "OFF", "N", "NO", "FALSE"),
):
    """Verifier factory for truthy verifiers. What is truthy is controlled by
    `true_like`, `false_like` lists.

    :param true_output: What to return if input is "truthy".
    :param false_output: What to return if input is NOT "truthy".
    :param true_like: List of things that should resolve to true_output.
    :param false_like: List of things that should resolve to false_output.

    :return: Verifier function.

    .. note:: FUNCTION FACTORY
    """

    def _truthy(value):
        if hasattr(value, "upper") and callable(value.upper):
            value = value.upper()
        if value in true_like:
            return true_output
        elif value in false_like:
            return false_output
        else:
            raise ValidationError(f"Cannot convert {value} to boolean")

    _truthy.allowable = f'[{",".join(str(a) for a in (true_like + false_like))}]'
    return _truthy


def is_numeric(min=None, max=None, scale=1, to_int=False):
    """Verifier factory for numeric verifiers. Resulting verifiers always return
    original type, or float if converted from string.

    :param min: Minimum input number to accept.
    :param false_output: Maximal input number to accept.

    :return: Verifier function.

    .. note:: FUNCTION FACTORY
    """

    def _is_numeric(value):
        if isinstance(value, int) or isinstance(value, float):
            rval = value
        try:
            rval = float(value)
        except (TypeError, ValueError):
            raise ValidationError(f"{value} is not numeric")
        if min is not None and rval < min:
            raise ValidationError(f"{value} is not in range (exceeds lower bound)")
        if max is not None and rval > max:
            raise ValidationError(f"{value} is not in range (exceeds upper bound)")
        rval *= scale
        return int(rval) if to_int else rval

    _is_numeric.allowable = (
        f'Numbers in range [{min if min is not None else ""}:'
        f'{max if max is not None else ""}]'
    )
    return _is_numeric


def is_integer(min=None, max=None, scale=1):
    """Verifier factory for integer verifiers.

    :param min: Minimum input number to accept.
    :param false_output: Maximal input number to accept.

    :return: Verifier function.

    .. note:: FUNCTION FACTORY
    """

    def _is_integer(value):
        if isinstance(value, int):
            rval = value
        try:
            rval = int(value)
        except (TypeError, ValueError):
            raise ValidationError(f"{value} is non-integer")
        if min is not None and rval < min:
            raise ValidationError(f"{value} is not in range (exceeds lower bound)")
        if max is not None and rval > max:
            raise ValidationError(f"{value} is not in range (exceeds upper bound)")
        return rval * scale

    _is_integer.allowable = (
        f'Numbers in range [{min if min is not None else ""}:'
        '{max if max is not None else ""}]'
    )
    return _is_integer


def is_in(*array, to_upper=False):
    """Verifier factory for "in"-type verifiers.

    :param array...: items in list of acceptable input values.

    :return: Verifier function.

    .. note:: FUNCTION FACTORY
    """

    def _is_in(value):
        if to_upper and hasattr(value, "upper"):
            value = value.upper()
        if value in array:
            return value
        else:
            raise ValidationError(f"{value} not in {array}")

    _is_in.allowable = '[{",".join(array)}]'
    _is_in.valid = array
    return _is_in


def map_to(mapping, to_upper=False):
    """Verifier factory for "map"-type verifiers.

    :param mapping: dictionary of acceptable input values and mapped output values.

    :return: Verifier function.

    .. note:: FUNCTION FACTORY
    """

    def _is_in(value):
        if to_upper and hasattr(value, "upper"):
            value = value.upper()
        if value in mapping:
            return mapping[value]
        else:
            raise ValidationError(f"{value} not in {mapping}")

    _is_in.allowable = '[{",".join(mapping.keys())}]'
    return _is_in


class SCPIInstrument(VisaInstrument, key="resource"):
    """Implementation of a VisaInstrument to communicate with devices accepting SCPI-
    like (query-set) command structures."""

    TIMEOUT = 10000  # 10 seconds
    """Link timeout (ms)."""
    SMALL_DELAY = 0.05  # 1/20 of second
    """Small delay between subsequent commands (s)."""

    READ_TERMINATION = "\n"
    """Read termination characters."""
    WRITE_TERMINATION = "\n"
    """Write termination characters."""

    # SCPIInstrument-level configuration for specific SCPI-like but non-SCPI device
    # classes
    SET_REQUIRES_READBACK = False
    """Either False, or True, or a callable to be executed after a readback with
    signature set_requires_feedback(instrument, command, value, readback_value).

    If True or callable, a readback is performed directly after a call to set(), before
    any query(). If False, it is assumed the device does not respond to set().
    """

    # Override reset command if required
    COM_RESET = "*RST; STATUS:PRESET; *CLS"

    # Override to provide commands (format:
    #       'COMMAND': {
    #           'SET': ':COM',
    #           'QUERY': ':COM?',
    #           'verifier': is_numeric(min=0, max=100)
    #       }
    # )
    SETTINGS = {
        "IDENTIFIER": {"QUERY": "*IDN?"},
    }

    def __init__(self, resource="ASRL/dev/ttyUSB1::INSTR", sim=False):
        """
        :param resource: VISA Resource address. See VISA docs for more info.
        """
        super().__init__(resource, sim)
        if sim:
            self.SMALL_DELAY = 0

    @acquire_lock()
    def __enter__(self, recover_attempt=False):
        """Establishes some important pyvisa parameters for SCPI-type instruments.
        Currently, these should always be set this way on the device before use!!!

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.

        :return: `SCPIInstrument` object in activated state.
        """
        super().__enter__(no_lock=True, recover_attempt=recover_attempt)
        return self

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Closes context on SCPIInstrument object. Defers to `VISAInstrument` class.

        :param recovery_attempt: Whether this is a recovery attempt by retry_on_fail.
        """
        super().__exit__(exception_type, exception_value, traceback, no_lock=True)

    @acquire_lock()
    @retry_on_fail_visa()
    def set(self, setting, *value, check_readback_only=False):
        """Set `setting` on instrument to `value`, and read-back using equivalent
        `query()`.

        :param setting: key in class dictionary SETTINGS.
        :param value: target value for `setting`.
        :param check_readback_only: how to only check readback value if
            SET_REQUIRES_READBACK is set, and not perform back-query. If 'same', checks
            that readback gives same command as was sent.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `set`-call is nested within).
        :param attempts: how many retries to give `set` command.

        :return: Read-back value if `query()` available, else whether number of bytes
                written during `set()` meets expectation
        """
        setting = setting.upper()
        if setting not in type(self).SETTINGS:
            raise ValidationError(
                f"Setting {setting} not found for device {type(self).__name__}. "
                "Cannot call set()."
            )
        setting_ = type(self).SETTINGS[setting]
        if "SET" not in (setting_):
            raise RuntimeError(
                f"SCPI command for SET {setting} not found for device "
                f"{type(self).__name__} - is query-only? Cannot call set()."
            )
        if "verifier" in setting_:
            verifiers = setting_["verifier"]
            if callable(verifiers):
                verifiers = (verifiers,)
            if len(verifiers) != len(value):
                raise ValidationError(
                    "Wrong number of arguments to set() call. "
                    f"Expected {len(verifiers)}, got {len(value)}"
                )
            value_ = list()
            for verifier, v in zip(verifiers, value):
                try:
                    value_.append(verifier(v))
                except ValidationError as ve:
                    logger.debug(f"ValidationError caught with message: {ve}")
                    raise ValidationError(
                        f"Value {v} does not pass verifier "
                        f"{verifier.__name__} for set() on setting {setting} "
                        f"for device {type(self).__name__}.\n"
                        f"Allowable values: {verifier.allowable}"
                    )
        else:
            value_ = value

        command = setting_["SET"].format(*value_)
        logger.debug(f'Setting "{command}"')
        time.sleep(self.SMALL_DELAY)

        if type(self).SET_REQUIRES_READBACK:
            logger.debug(
                "set() requires readback to empty buffer for this instrument class..."
            )
            rval = self._instrument.query(command)
            logger.debug(f"Write/read cycle returned: {rval}")
            if callable(type(self).SET_REQUIRES_READBACK):
                type(self).SET_REQUIRES_READBACK(self, command, value, rval)
            logger.debug(f"Check readback only value: {check_readback_only}")
            if check_readback_only == "same":
                return rval == command
            elif check_readback_only == "parse":
                if "parser" in setting_:
                    rval = setting_["parser"](rval, setting_)

        else:
            rval = self._instrument.write(command)
            logger.debug(f"Write returned: {rval}")
        time.sleep(self.SMALL_DELAY)

        if "QUERY" in setting_:
            # If possible, read back
            ret = self.query(setting, *value, no_lock=True, attempts=1)
            logger.debug(f'Read-back of "{command}" returned: {ret}')
            return ret
        else:
            if (
                isinstance(rval, str)
                or isinstance(rval, bool)
                or isinstance(rval, tuple)
            ):
                return rval
            else:
                return rval == len(command) + len(self._instrument.write_termination)

    @acquire_lock()
    @retry_on_fail_visa()
    def query(self, setting, *params):
        """Query `setting` on instrument.

        :param setting: key in class dictionary SETTINGS.
        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `query`-call is nested within).
        :param attempts: how many retries to give `query` command.

        :return: Data returned by device for given query.
        """
        setting = setting.upper()
        if setting not in type(self).SETTINGS:
            raise RuntimeError(
                f"Setting {setting} not found for device {type(self).__name__}. "
                "Cannot call query()."
            )
        setting_ = type(self).SETTINGS[setting]
        if "QUERY" not in setting_:
            raise RuntimeError(
                f"SCPI command for QUERY {setting} not found for device "
                f"{type(self).__name__} - is SET-only? Cannot call query()."
            )

        if "verifier" in setting_ and len(params):
            verifiers = setting_["verifier"]
            if callable(verifiers):
                verifiers = (verifiers,)
            params_ = list()
            for verifier, p in zip(verifiers, params):
                if verifier is None:
                    params_.append(p)
                    continue
                try:
                    params_.append(verifier(p))
                except ValidationError as ve:
                    logger.debug(f"ValidationError caught with message: {ve}")
                    raise ValidationError(
                        f"Value {p} does not pass verifier "
                        f"{verifier.__name__} for query() on setting {setting} "
                        f"for device {type(self).__name__}.\n"
                        f"Allowable values: {verifier.allowable}"
                    )
        else:
            params_ = params

        command = setting_["QUERY"].format(*params_)
        time.sleep(self.SMALL_DELAY)
        logger.debug(f'Querying "{command}"')
        ret = self._instrument.query(command).strip()
        logger.debug(f"Query returned: {ret}")
        if "parser" in setting_:
            ret = setting_["parser"](ret, setting_)
        return ret

    @acquire_lock()
    def reset(self):
        """Reset device using the provided COM_RESET command within the class
        implementation."""
        time.sleep(self.SMALL_DELAY)
        return self._instrument.write(type(self).COM_RESET)

    def identify(self, **kwargs):
        """Query \\*IDN for device identifier.

        :param no_lock: override `acquire_lock` (e.g. if lock already taken by function
            that `reset`-call is nested within).
        :param attempts: how many retries to give `query` command.
        """
        # uses lock from sub-call
        return self.query("IDENTIFIER", **kwargs)
