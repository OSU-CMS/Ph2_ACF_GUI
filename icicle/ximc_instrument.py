"""Libximc/pyximc Instrument interface/implementation. May be subclassed, but can be
used directly for most libximc-compliant motor controllers or translation stages.

This module contains the `XimcInstrument` class, and associated Enums and Exceptions.

.. Warning: importing this module attempts to load both the local pyximc interface,
            and the system libximc dynamic linked library. If either does not exist or
            is not installed, the import will fail.
"""

from .instrument import Instrument, Multiton, acquire_lock, retry_on_fail

from sys import exit
from enum import IntEnum

# libximc
try:
    import libximc as ximc
    from deps.libximc_profiles import load_profile
except OSError as err:
    print(err)
    print(
        "Can't load libximc library. Please add all shared libraries to the "
        "appropriate places. It is decribed in detail in developers' documentation. "
        "On Linux make sure you installed libximc-dev package.\nMake sure that the "
        "architecture of the system and the interpreter is the same"
    )
    exit(1)


# ==== XIMC exception definitions ====


class XimcError(RuntimeError):
    """Generic libXIMC/XimcInstrument error class."""

    pass


class XimcResultError(XimcError):
    """Error returned from libXIMC as pyximc.Result."""

    def __init__(self, resultcode, msg=None):
        if msg:
            super().__init__(
                f"libXIMC command returned result code {resultcode} with message {msg}"
            )
        else:
            super().__init__(f"libXIMC command returned result code {resultcode}")


class XimcReadbackError(XimcError):
    """Readback inconsistency after sending XimcInstrument command."""

    pass


class XimcCalibrationError(XimcError):
    """Issue reading or processing calibration procedure for XimcInstrument."""

    pass


# ======== end XIMC exceptions ========


class XimcInstrument(Instrument, metaclass=Multiton, key="resource"):
    """Implementation of the Instrument Class that wraps libximc to control Standa and
    similar motor controllers.

    .. Warning: All subclasses of XimcInstrument must have the 'key' keyword class
                argument defined to specify which '__init__' keyword argument should
                be used as the Multiton key.
    """

    NO_SIMULATION = True

    # Currently unused - read back from stage/engine settings :)
    STAGE_CALIBRATIONS = {
        "STANDA": {
            "8MT175-150": {"um": 2.5, "mm": 0.0025, "cm": 0.00025},
        },
    }
    """ Currently Unused - initial implementation of stage calibrations. Now read from
    stage/engine setting stored on device on __enter__() """

    # Default is mm - conversion to others
    USER_UNITS = {
        "um": 1e-3,
        "mm": 1e0,
        "cm": 1e1,
        "m": 1e3,
        "inch": 0.0393701,
        "NM": 5.39957e-7,  # nautical miles
    }
    """Conversion factors for different choices of user units.

    Assume base unit in calibration is mm.
    """

    class MicrostepMode(IntEnum):
        """Libximc engine microstep modes."""

        MICROSTEP_MODE_FRAC_256 = 9
        MICROSTEP_MODE_FRAC_128 = 8
        MICROSTEP_MODE_FRAC_64 = 7
        MICROSTEP_MODE_FRAC_32 = 6
        MICROSTEP_MODE_FRAC_16 = 5
        MICROSTEP_MODE_FRAC_8 = 4
        MICROSTEP_MODE_FRAC_4 = 3
        MICROSTEP_MODE_FRAC_2 = 2
        MICROSTEP_MODE_FULL = 1

    @classmethod
    def library_version(cls):
        """Get libximc version.

        :returns: libximc version
        """
        sbuf = ximc.create_string_buffer(64)
        ximc.lib.ximc_version(sbuf)
        return str(sbuf.raw.decode().rstrip("\0"))

    @classmethod
    def devices(cls):
        """Enumerate ximc devices.

        :returns: list of ximc devices; each device is a dict with fields `idx`,
            `resource`, `name`.
        """
        probe_flags = ximc.EnumerateFlags.ENUMERATE_PROBE
        enum_hints = b""
        devenum = ximc.lib.enumerate_devices(probe_flags, enum_hints)
        dev_count = ximc.lib.get_device_count(devenum)
        controller_name = ximc.controller_name_t()
        devices = list()
        for idx in range(dev_count):
            enum_name = ximc.lib.get_device_name(devenum, idx)
            result = ximc.lib.get_enumerate_device_controller_name(
                devenum, idx, ximc.byref(controller_name)
            )
            if result == ximc.Result.Ok:
                devices.append(
                    {
                        "idx": int(idx),
                        "resource": enum_name.decode(),
                        "name": controller_name.ControllerName.decode(),
                    }
                )
        return devices

    def __init__(
        self, resource="", calibration=None, microstep_mode=None, user_units="mm"
    ):
        """.. Note: All arguments must be provided as explicit keyword argument.

        :param resource: libximc controller name. See libximc docs for examples.
        :param calibration: user unit multiplier value to overrride value read from
            controller. (Not recommended)
        :param microstep_mode: custom microstep mode to override value read from
            controller. (Not recommended)
        :param user_units: user_units to use for this device.
        """
        self._calibration = calibration
        self._microstep_mode = microstep_mode
        self._user_units = user_units
        super().__init__(resource)

    @acquire_lock()
    def __enter__(self, recover_attempt=False, skip_info_read=False):
        """Initialises connection to ximc device.

        Caution: this is a locking action, and allocates the libximc device handle.

        :param recovery_attempt: Whether this is a recovery attempt by `retry_on_fail`.
            Should be propagated.
        :param skip_info_read: If `True`, does not attempt to read calibration values
            from controller's internal memory.

        :returns: Instrument object with activated context.
        """
        if not self._connected:
            self._instrument = ximc.lib.open_device(
                self._resource.encode()
                if isinstance(self._resource, str)
                else self._resource
            )
        ret = super().__enter__(no_lock=True, recover_attempt=recover_attempt)
        if not skip_info_read:
            calibration, microstep_mode = self.read_calibration(no_lock=True).values()
            if self._calibration is None:
                self._calibration = calibration
            if self._microstep_mode is None:
                self._microstep_mode = microstep_mode
        return ret

    @acquire_lock()
    def __exit__(
        self,
        exception_type=None,
        exception_value=None,
        traceback=None,
        recover_attempt=False,
    ):
        """Closes connection handle to instrument and unlocks the device handle.

        :param recovery_attempt: Whether this is a recovery attempt by `retry_on_fail`.
            Should be propagated.
        """

        if self._connected:
            ximc.lib.close_device(
                ximc.byref(ximc.cast(self._instrument, ximc.POINTER(ximc.c_int)))
            )
        super().__exit__(
            exception_type,
            exception_value,
            traceback,
            no_lock=True,
            recover_attempt=recover_attempt,
        )

    @property
    def has_calibration(self):
        """
        :returns: Whether an active calibration has been loaded from the controller
            memory or set during instantiation.
        """
        return self._calibration is not None and self._microstep_mode is not None

    @property
    def ximc_calibration(self):
        """
        :returns: `pyximc.calibration_t` object corresponding to current calibration to
            be used in libximc `command_*_calb()` methods. Returns `None` if not
            `has_calibration`.
        """
        if self.has_calibration:
            ximc_calibration = ximc.calibration_t()
            ximc_calibration.A = ximc.c_double(self._calibration)
            ximc_calibration.MicrostepMode = int(self._microstep_mode)
            return ximc_calibration
        else:
            raise XimcCalibrationError(
                "No loaded calibration to generate ximc.calibration_t object from"
            )

    # ============== XIMC commands ===============

    @acquire_lock()
    @retry_on_fail()
    def identify(self):
        """Return identification information about controller that this device
        corresponds to.

        :returns: Information as dict with fields `manufacturer`, `manufacturer_id`,
            `description`, `major, `minor`, `release`.
        """
        devinfo = ximc.device_information_t()
        result = ximc.lib.get_device_information(self._instrument, ximc.byref(devinfo))
        if result != ximc.Result.Ok:
            raise XimcResultError(result)
        return {
            "manufacturer": repr(ximc.string_at(devinfo.Manufacturer).decode()),
            "manufacturer_id": repr(ximc.string_at(devinfo.ManufacturerId).decode()),
            "description": repr(ximc.string_at(devinfo.ProductDescription).decode()),
            "major": repr(devinfo.Major),
            "minor": repr(devinfo.Minor),
            "release": repr(devinfo.Release),
        }

    @acquire_lock()
    @retry_on_fail()
    def read_calibration(self):
        """Read screw pitch, engine steps per revolution, and microstep mode from
        connected controller, and generate calibration info.

        :returns: Calibration as dict with fields `multiplier`, `microstep_mode`.
        """
        stage_settings = ximc.stage_settings_t()
        result = ximc.lib.get_stage_settings(
            self._instrument, ximc.byref(stage_settings)
        )
        if result != ximc.Result.Ok:
            raise XimcResultError(
                result, "Failed to read stage settings - libXIMC returned error"
            )
        engine_settings = ximc.engine_settings_t()
        result = ximc.lib.get_engine_settings(
            self._instrument, ximc.byref(engine_settings)
        )
        if result != ximc.Result.Ok:
            raise XimcResultError(
                result, "Failed to read engine settings - libXIMC returned error"
            )
        return {
            "multiplier": (
                1
                / int(engine_settings.StepsPerRev)
                * float(stage_settings.LeadScrewPitch)
                * type(self).USER_UNITS[self._user_units]
            ),
            "microstep_mode": int(engine_settings.MicrostepMode),
        }

    @acquire_lock()
    @retry_on_fail()
    def set_microstep_mode(
        self, microstep_mode, skip_calibration_update=False, readback=True
    ):
        """Set engine microstep mode.

        :param microstep_mode: microstep mode to set.
        :param skip_calibration_update: if set to `True`, does not trigger an update of
            the user units calibration.
        :param readback: whether to readback/check new value.

        :returns: Readback microstep mode if `readback` is `True`, else None.
        """
        engine_settings = ximc.engine_settings_t()
        result = ximc.lib.get_engine_settings(
            self._instrument, ximc.byref(engine_settings)
        )
        if result != ximc.Result.Ok:
            raise XimcResultError(
                result, "Failed to read engine settings - libXIMC returned error"
            )
        engine_settings.MicrostepMode = int(microstep_mode)
        result = ximc.lib.set_engine_settings(self._instrument, engine_settings)
        if result != ximc.Result.Ok:
            raise XimcResultError(
                result,
                (
                    "Failed to set microstep mode in engine settings - "
                    "libXIMC returned error"
                ),
            )
        if not readback:
            if not skip_calibration_update:
                self._microstep_mode = microstep_mode
            return
        else:
            result = ximc.lib.get_engine_settings(
                self._instrument, ximc.byref(engine_settings)
            )
            microstep_mode_ = int(engine_settings.MicrostepMode)
            if not microstep_mode_ == microstep_mode:
                raise XimcReadbackError("microstep_mode did not read back as expected!")
            if not skip_calibration_update:
                self._microstep_mode = microstep_mode_
            return microstep_mode_

    @acquire_lock()
    def write_profile(
        self,
        profile=None,
        profile_vendor=None,
        profile_device=None,
        skip_calibration_update=False,
    ):
        """Load and/or write profile from `deps/custom_profiles/<vendor>/<device>` or
        `deps/pyximc_profiles/<vendor>/<device>` to connected controller.

        :param profile: pyximc profile function (callable) to execute.
        :param profile_vendor: vendor of device to fetch profile from
            `deps/(custom|pyximc)_profiles/<vendor>/<device>`
        :param profile_device: vendor of device to fetch profile from
            `deps/(custom|pyximc)_profiles/<vendor>/<device>`
        :param skip_calibration_update: if set to `True`, does not trigger an update of
            the user units calibration.
        """
        if profile:
            profile_ = profile
        elif profile_vendor and profile_device:
            profile_ = load_profile(profile_vendor, profile_device)
        else:
            raise XimcError("No profile supplied to write_profile")
        if not (profile_ and callable(profile_)):
            raise XimcError("Profile cannot be executed")

        result = profile_(ximc.lib, self._instrument)
        if result != ximc.Result.Ok:
            raise XimcResultError(
                result, "Failed to set requested profile - libXIMC returned error"
            )
        if not skip_calibration_update:
            self._calibration, self._microstep_mode = self.read_calibration(
                no_lock=True
            ).values()

    @acquire_lock()
    @retry_on_fail()
    def get_position(self):
        """Read current position of device in steps and microsteps.

        :returns: Position as dict with fields `steps`, `microsteps`.
        """
        pos = ximc.get_position_t()
        result = ximc.lib.get_position(self._instrument, ximc.byref(pos))
        if result != ximc.Result.Ok:
            raise XimcResultError(result)
        return {"steps": pos.Position, "microsteps": pos.uPosition}

    @acquire_lock()
    @retry_on_fail()
    def get_position_user(self):
        """Read current position of device and encoder (if applicable) in user units.

        :returns: Position as dict with fields `position`, `encoder_position`.
        """
        if not self.has_calibration:
            raise XimcCalibrationError("XimcInstrument has no calibration initialised")
        pos = ximc.get_position_calb_t()
        result = ximc.lib.get_position_calb(
            self._instrument, ximc.byref(pos), ximc.byref(self.ximc_calibration)
        )
        if result != ximc.Result.Ok:
            raise XimcResultError(result)
        return {
            "position": float(pos.Position),
            "encoder_position": float(pos.EncPosition),
        }

    @acquire_lock()
    @retry_on_fail()
    def move(self, steps, microsteps, wait_and_readback=True):
        """Move device to provided position in steps and microsteps (location = steps *
        calibration + microsteps * calibration / microsteps_per_step).

        :param steps: location to move to in steps.
        :param microsteps: sub-location to move to in microsteps.
        :param wait_and_readback: whether to wait for move to complete and
            readback/check new position.

        :returns: Position as dict with fields `steps`, `microsteps` if
            `wait_and_readback` is `True`, else None.
        """
        result = ximc.lib.command_move(self._instrument, steps, microsteps)
        if result != ximc.Result.Ok:
            raise XimcResultError(result)
        if wait_and_readback:
            # Check position and return
            self.wait(no_lock=True)
            position = self.get_position(no_lock=True)
            if position["steps"] != steps or position["microsteps"] != microsteps:
                raise XimcReadbackError(
                    f"move({steps}, {microsteps}) actually moved to "
                    f'({position["steps"]}, {position["microsteps"]})'
                )
            return position
        else:
            return result

    @acquire_lock()
    @retry_on_fail()
    def move_user(self, user_units, wait_and_readback=True):
        """Move device to provided position in user units (location = steps *
        calibration + microsteps * calibration / microsteps_per_step).

        :param user_units: location to move to in currently set user units
            (default: mm; see `__init__()`).
        :param wait_and_readback: whether to wait for move to complete and
            readback/check new position.

        :returns: Position as dict with fields `position`, `encoder_position` if
            `wait_and_readback` is `True`, else None.
        """
        if not self.has_calibration:
            raise XimcCalibrationError("XimcInstrument has no calibration initialised")
        result = ximc.lib.command_move_calb(
            self._instrument,
            ximc.c_float(user_units),
            ximc.byref(self.ximc_calibration),
        )
        if result != ximc.Result.Ok:
            raise XimcResultError(result)
        if wait_and_readback:
            # Check position and return
            self.wait(no_lock=True)
            position = self.get_position_user(no_lock=True)
            if position["position"] != user_units:
                raise XimcReadbackError(
                    f'move_user({user_units}) actually moved to {position["position"]}'
                )
            return position
        else:
            return result

    @acquire_lock()
    @retry_on_fail()
    def home(self):
        """Go to home position of device.

        :returns: success or failure.
        """
        result = ximc.lib.command_home(self._instrument)
        if result != ximc.Result.Ok:
            raise XimcResultError(result)
        return result

    @acquire_lock()
    @retry_on_fail()
    def zero(self):
        """Zero axis of device.

        :returns: success or failure.
        """
        result = ximc.lib.command_zero(self._instrument)
        if result != ximc.Result.Ok:
            raise XimcResultError(result)
        return result

    @acquire_lock()
    @retry_on_fail()
    def wait(self):
        """Wait for device to complete current action."""
        return ximc.lib.command_wait_for_stop(self._instrument)

    @acquire_lock()
    @retry_on_fail()
    def get_movement_settings(self, raw_object=False):
        """Get current movement settings for controller.

        :param raw_object: Whether to return a raw `pyximc.move_settings_t` object
            instead of a python dict.

        :returns: movement settings as dict with fields `speed`, `microspeed`,
            `acceleration`, `deceleration`, `antiplay_speed`, `antiplay_microspeed`,
            `movement_flags`.
            See https://libximc.xisupport.com/doc-en/structmove__settings__t.html
        """
        move_settings = ximc.move_settings_t()
        result = ximc.lib.get_move_settings(self._instrument, ximc.byref(move_settings))
        if result != ximc.Result.Ok:
            raise XimcResultError(result)
        if raw_object:
            return move_settings
        return {
            "speed": move_settings.Speed,
            "microspeed": move_settings.uSpeed,
            "acceleration": move_settings.Accel,
            "deceleration": move_settings.Decel,
            "antiplay_speed": move_settings.AntiplaySpeed,
            "antiplay_microspeed": move_settings.uAntiplaySpeed,
            "movement_flags": move_settings.MoveFlags,
        }

    @acquire_lock
    @retry_on_fail
    def set_movement_settings(
        self,
        speed=None,
        microspeed=None,
        acceleration=None,
        deceleration=None,
        antiplay_speed=None,
        antiplay_microspeed=None,
        movement_flags=None,
        readback=True,
    ):
        """Get current movement settings for controller.

        :param speed: speed in steps/second.
        :param microspeed: sub-speed in microsteps/second.
        :param acceleration: acceleration in steps/second^2.
        :param deceleration: deceleration in steps/second^2.
        :param antiplay_speed: antiplay speed in steps/second.
        :param antiplay_microspeed: antiplay sub-speed in microsteps/second.
        :param movement_flags: movement flags.
            See https://libximc.xisupport.com/doc-en/structmove__settings__t.html.

        :param readback: Whether to readback movement settings and return readback
            value.

        :returns: movement settings as dict with fields `speed`, `microspeed`,
            `acceleration`, `deceleration`, `antiplay_speed`, `antiplay_microspeed`,
            `movement_flags` if `readback` is `True`, else `None`.
        """
        move_settings = self.get_movement_settings(no_lock=True, raw_object=True)

        if speed is not None:
            move_settings.Speed = int(speed)
        if microspeed is not None:
            move_settings.uSpeed = int(microspeed)
        if acceleration is not None:
            move_settings.Accel = int(acceleration)
        if deceleration is not None:
            move_settings.Decel = int(deceleration)
        if antiplay_speed is not None:
            move_settings.AntiplaySpeed = int(antiplay_speed)
        if antiplay_microspeed is not None:
            move_settings.uAntiplaySpeed = int(antiplay_microspeed)
        if movement_flags is not None:
            move_settings.MoveFlags = int(movement_flags)

        result = ximc.lib.set_move_settings(self._instrument, move_settings)
        if result != ximc.Result.Ok:
            raise XimcResultError(result)
        if readback:
            move_settings_ = self.get_movement_settings()
        return move_settings_
