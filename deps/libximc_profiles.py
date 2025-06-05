from os import path

# libximc
try:
    from libximc import lib
except OSError as err:
    print(err)
    print(
        "Can't load libximc library. Please add all shared libraries to the "
        "appropriate places. It is decribed in detail in developers' documentation. "
        "On Linux make sure you installed libximc-dev package.\nMake sure that the "
        "architecture of the system and the interpreter is the same"
    )
    exit(1)


class XimcProfileNotFoundError(FileNotFoundError):
    """Exception for when profile file cannot be found."""

    pass


# end class XimcProfileNotFoundError


def load_profile(vendor, device):
    """Dynamically load profile file from `custom_profiles` or `libximc_profiles`.

    :param vendor: vendor directory name.
    :param device: device file name.
    :returns: callable profile function with interface `set_profile(ximc_lib,
        device_idx)`.
    """
    _before, _after = None, None
    if path.exists(
        path.join(path.dirname(__file__), "custom_profiles", vendor, f"{device}.py")
    ):
        with open(
            path.join(path.dirname(__file__), "custom_profiles", vendor, f"{device}.py")
        ) as fp:
            source = fp.read()
    elif path.exists(
        path.join(path.dirname(__file__), "libximc_profiles", vendor, f"{device}.py")
    ):
        with open(
            path.join(
                path.dirname(__file__), "libximc_profiles", vendor, f"{device}.py"
            )
        ) as fp:
            source = fp.read()
    else:
        raise XimcProfileNotFoundError(
            f"Profile {vendor}/{device} not found in deps/custom_profiles or "
            "deps/libximc_profiles"
        )
    _before = set(dir())
    exec(compile(source, device, "exec"))
    _after = set(dir())
    _diff = _after - _before
    if len(_diff) != 1:
        raise ImportError(
            "More or less than one new global found in imported profile plugin "
            "deps/libximc_profiles/{vendor}/{device}.py"
        )
    del _before, _after
    func = _diff.pop()
    return globals().get(func) or locals().get(func)


# end def load_profile()


def set_profile(vendor, device, instrument, lib=lib):
    """Dynamically load and write profile from `custom_profiles` or `libximc_profiles`
    to device.

    :param vendor: vendor directory name.
    :param device: device file name.
    :param instrument: libximc device idx.
    :param lib: libximc.lib handle.
    :returns: return value of profile execution.
    """
    profile = load_profile(vendor, device)
    return profile(lib, instrument)


# end def set_profile()
