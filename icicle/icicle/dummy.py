"""This instrument is essentially just a placeholder for a non-existing instrument. The
main usage is together with the `InstrumentCluster` class to let a setup work, even when
an important instrument is missing.

:param metaclass: This might in the meantime be obsolete. It does work inside scripts
    and because the DummyInstrument does not have any `cli` instantiation, it should
    not matter.
"""

from .instrument import Singleton


class DummyInstrument(metaclass=Singleton):
    pass

    def __init__(self, **kwargs):
        pass

    def __enter__(self, **kwargs):
        pass

    def __exit__(self, **kwargs):
        pass
