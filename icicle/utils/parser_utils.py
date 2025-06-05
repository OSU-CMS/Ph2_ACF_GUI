"""Parser utilities."""


def numeric_bool(value, _):
    """Returns bool from '0' or '1'.

    :param value: value to convert.
    """
    return bool(int(value))


def truthy_bool(
    true_like=(True, 1, "1", "ON", "Y", "YES", "TRUE"),
    false_like=(False, 0, "0", "OFF", "N", "NO", "FALSE"),
):
    """Parser factory for truthy verifiers. What is truthy is controlled by `true_like`,
    `false_like` lists.

    :param true_like: List of things that should resolve to true_output.
    :param false_like: List of things that should resolve to false_output.

    :return: Verifier function.

    .. note:: FUNCTION FACTORY
    """

    def _truthy(value, _):
        if hasattr(value, "upper") and callable(value.upper):
            value = value.upper()
        if value in true_like:
            return True
        elif value in false_like:
            return False
        else:
            raise ValueError(f"Cannot convert {value} to boolean")

    _truthy.allowable = f'[{",".join(str(a) for a in (true_like + false_like))}]'
    return _truthy


def string_bool(true_string, false_string, case_sensitive=False):
    """Function factory for string to bool conversion functions.

    :param true_string: string corresponding to true.
    :param false_string: string corresponding to false.
    :param case_sensitive: whether to respect case sensitivity.
    """

    def _string_bool(value, _):
        """Returns bool from `true_string` or `false_string`

        :param value: value to convert.
        """
        if _string_bool.case_sensitive:
            if _string_bool.true_string in value.strip():
                return True
            if _string_bool.false_string in value.strip():
                return False
        else:
            if _string_bool.true_string in value.lower().strip():
                return True
            if _string_bool.false_string in value.lower().strip():
                return False
        raise ValueError(
            f"{value} is not one of {_string_bool.true_string} or "
            f"{_string_bool.false_string} boolean options"
        )

    _string_bool.true_string = (
        true_string if case_sensitive else true_string.lower()
    ).strip()
    _string_bool.false_string = (
        false_string if case_sensitive else false_string.lower()
    ).strip()
    _string_bool.case_sensitive = case_sensitive
    return _string_bool


def numeric_int(value, _):
    """Returns int.

    :param value: value to convert.
    """
    return int(value)


def int_map(mapping):
    """Function factory to parse int and map to output value.

    :param mapping: callable or dict definiting mapping. If callable, must take
        2 arguments: (value: int, setting: str)
    """

    def int_map_(value, setting):
        """Converts to int, then propagates through `mapping` dict or callable to get
        result.

        :param value: value to convert.
        :param setting: setting this value is tied to.
        """
        if callable(int_map_.mapping):
            return int_map_.mapping(int(value), setting)
        else:
            return int_map_.mapping[int(value)]

    int_map_.mapping = mapping
    return int_map_


def str_map(mapping, uppercase=False):
    """Function factory to map str to output value.

    :param mapping: callable or dict definiting mapping. If callable, must take
        2 arguments: (value: int, setting: str)
    """

    def str_map_(value, setting):
        """Propagates through `mapping` dict or callable to get result.

        :param value: value to convert.
        :param setting: setting this value is tied to.
        """
        value = value.strip()
        if str_map_.uppercase:
            value = value.upper()
        if callable(str_map_.mapping):
            return str_map_.mapping(value, setting)
        else:
            return str_map_.mapping[value]

    str_map_.mapping = mapping
    str_map_.uppercase = uppercase
    return str_map_


def numeric_float(value, _):
    """Returns float.

    :param value: value to convert.
    """
    return float(value)


def float_list(delimiter=","):
    """Function factory for float list parsing with adjustable delimiter.

    :param delimiter: delimiter to split string on. Defaults to `,`.
    """

    def float_list_(value, _):
        """Returns list of floats.

        :param value: value to convert.
        """
        return tuple(float(f) for f in value.split(float_list_.delimiter))

    float_list_.delimiter = delimiter
    return float_list_


def strip_float(*remove):
    """Function factory for string strip and float conversion functions.

    :param remove...: substrings which should be stripped from output prior to float
        conversion.
    """

    def _strip_float(value, _):
        """Returns float after stripping all instances of `remove`

        :param value: value to convert.
        """
        value = str(value).strip()
        for s in _strip_float.remove:
            value = value.replace(s, "")
        return float(value)

    _strip_float.remove = remove
    return _strip_float


def strip_str(*remove):
    """Function factory for string strip functions.

    :param remove...: substrings which should be removed (replaced by "") from output.
    """

    def _strip_str(value, _):
        """Returns float after stripping all instances of `remove`

        :param value: value to convert.
        """
        value = str(value).strip()
        for s in _strip_str.remove:
            value = value.replace(s, "")
        return value

    _strip_str.remove = remove
    return _strip_str


def float_or_strings(*strings, replace=None, case_sensitive=False):
    """Function factory for string strip and float conversion functions.

    :param strings...: strings which should be accepted as valid instead of float.
    :param replace: iterable of strings which should be stripped from output prior to
        float conversion.
    :param case_sensitive: whether strings to be accepted should be tested case-
        sensitively.
    """

    def _float_or_strings(value, setting):
        """Returns float after stripping all instances of `remove`

        :param value: value to convert.
        """
        value = str(value).strip()
        if _float_or_strings.case_sensitive:
            if value.strip() in _float_or_strings.strings:
                return value
        else:
            if value.strip().lower() in _float_or_strings.strings:
                return value
        return _float_or_strings.float(value, setting)

    _float_or_strings.strings = tuple(
        s.strip() if case_sensitive else s.strip().lower() for s in strings
    )
    _float_or_strings.float = (
        numeric_float if replace is None else strip_float(*replace)
    )
    _float_or_strings.case_sensitive = case_sensitive
    return _float_or_strings


def bitfield_bool(bit):
    """Function factory for bitfield decompositions.

    :param bit: which bit should be returned as bool on parsing.
    """

    def _bitfield_bool(value, _):
        """Returns bit at position `bit` of integer value as bool.

        :param value: value to convert.
        """
        return bool(int(value) >> _bitfield_bool.bit & 1)

    _bitfield_bool.bit = bit
    return _bitfield_bool


class PassOriginalValueOnError:
    pass


def catch_failure(verifier, value_on_error=PassOriginalValueOnError):
    """Function factory for failure/error-catching wrapper.

    :param verifier: verifier to be wrapped by try-catch block.
    """

    def _catch_failure(value, setting):
        """Attempts to run verifier on `value`, catches error.

        :param value: value to convert.
        """
        try:
            return _catch_failure.verifier(value, setting)
        except Exception as e:
            print(e)
            if value_on_error is not PassOriginalValueOnError:
                return value_on_error
            else:
                return value

    _catch_failure.verifier = verifier
    return _catch_failure
