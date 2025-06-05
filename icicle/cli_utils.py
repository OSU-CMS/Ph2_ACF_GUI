"""Utility decorators and functions to used with Click library to generate instrument
CLIs.

Contains `@with_instrument` and `@print_output` decorators, as well as verbosity
conversion function and MonitoringLogger broadcaster class.
"""

import sys
import click
import logging
from functools import update_wrapper, wraps
from .scpi_instrument import ValidationError
from .instrument import ChannelError


# extend click's pass_context handler to unwrap instrument in with-statement
def with_instrument(function):
    """Wraps decorated function in `with <instrument>: ...` statement.

    Decorator for CLI function which wraps function in `with <instrument>: ...`
    statement to enter and exit instrument context before and after function is
    executed, respectively.

    .. note:: DECORATOR
    """

    @click.pass_context
    def new_function(ctx, *args, **kwargs):
        with ctx.obj as instrument:
            return ctx.invoke(function, instrument, *args, **kwargs)

    return update_wrapper(new_function, function)


def print_output(function, pretty_dict=True):
    """Decorator for CLI function which prints return value of function after
    completion.

    Also wraps function in `try: ... except SCPIInstrument.ValidationError,`
    `Instrument.ChannelError`, `NotImplementedError` statement to catch errors
    of these types and suppress traceback in these cases.

    .. note:: DECORATOR
    """

    @wraps(function)
    def _print_output(*args, **kwargs):
        try:
            ret = function(*args, **kwargs)
            if isinstance(ret, dict) and pretty_dict:
                cret = [(str(k), str(v)) for k, v in ret.items()]
                llen = ((max(len(ll[0]) for ll in cret) // 4) + 2) * 4
                rlen = ((max(len(ll[1]) for ll in cret) // 4) + 2) * 4
                for ll, lr in cret:
                    print(f"{ll:<{llen}}{lr:<{rlen}}")
            else:
                print(ret)
        except ValidationError as ve:
            print(f"ValidationError: {str(ve)}")
            sys.exit(1)
        except ChannelError as ce:
            print(f"ChannelError: {str(ce)}")
            sys.exit(1)
        except NotImplementedError as ne:
            print(f"NotImplementedError: {str(ne)}")
            sys.exit(1)

    return _print_output


def verbosity(level):
    """Convert numerical verbosity level to logging level.

    0 => WARNING 1 => INFO 2 => DEBUG
    """
    if level == 0:
        return logging.WARNING
    elif level == 1:
        return logging.INFO
    else:
        return logging.DEBUG


class InContextObjectField(click.Choice):
    def __init__(self, choices, field, case_sensitive=True):
        super().__init__(choices, case_sensitive=case_sensitive)
        self.field = field

    def convert(self, value, param, ctx):
        if ctx is not None:
            self.choices = getattr(ctx.obj, self.field)
        return super().convert(value, param, ctx)

    def shell_complete(self, ctx, param, incomplete):
        if ctx is not None:
            self.choices = getattr(ctx.obj, self.field)
        return super().shell_complete(ctx, param, incomplete)


class MonitoringLogger:
    """Multiple output logging interface.

    Multilogger that allows configurable writers (i.e. functions) to be registered, as
    well as flushers (also functions). On a write to a MonitoringLogger, all registered
    writers are called, then all flushers.
    """

    def __init__(self, *writers):
        """Initialise multilogger and immediately register writers if provided.

        :param writers: List of functions to register that should be called by logger on
            write().
        """
        for writer in writers:
            assert callable(writer)
        self.writers = list(writers)
        self.flushers = []

    def register_writer(self, writer):
        """Register writer.

        :param writer: Functions to register that should be called by logger on write().
        """
        assert callable(writer)
        self.writers.append(writer)

    def register_flusher(self, flusher):
        """Register flusher.

        :param flusher: Functions to register that should be called by logger after
            write().
        """
        assert callable(flusher)
        self.flushers.append(flusher)

    def write(self, *args, **kwargs):
        """Write to all registered writers, then flush all registered flushers.

        :param args...: arguments to be passed to writers. :param \\*kwargs: keyword
            arguments to be passed to writers.
        """
        for writer in self.writers:
            writer(*args, **kwargs)
        for flusher in self.flushers:
            flusher()

    def __call__(self, *args, **kwargs):
        """Shorthand for write()."""
        self.write(*args, **kwargs)
