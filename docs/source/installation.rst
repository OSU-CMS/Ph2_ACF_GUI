.. _installation:

Requirements
============

Python 3.7 at minimum is required for the installation of `ICICLE`; 3.9 or greater is recommended to ensure cross-compatibility with `dirigent` and the rest of the ETHZ module testing framework.


Environment
-----------
It is recommended to install ICICLE and associated dependencies in a separate python environment. One may use Python virtualenvs or anaconda environments for this purpose - it makes litte practical difference. An example of how to set up a python virtualenv for this purpose is below::

    cd /path/to/cloned/repository/
    python3 -m venv py3-env
    . py3-env/bin/activate


Dependencies
------------
Most package dependencies are automatically installed by ``pip``; no specific action is required.

Three sets of optional dependencies are available for extended features:
 - **sim**: installs ``pyvisa_sim`` as simulation backend for devices (only partially implemented so far).
 - **docs**: installs ``sphinx`` and ``sphinx-rtd-theme`` for generating documentation.
 - **dev**: installs ``pre-commit``. ``black``, and ``docformatter[tomli]`` for pre-commit hook and automatic format.


Installation
------------

Finally, the ``icicle`` package may be installed as a stand-alone installation using ``pip`` or a similar ``pyproject.toml``-compliant package manager::

    python3 -m pip install "."

To install the ``sim``, ``docs``, or ``dev`` additional dependency sets for simulating devices, generating documentation, or developing ICICLE respectively, specify any or all these in square braces after the project path on install::

    python3 -m pip install [-e] ".[sim,docs,dev]"

The ``-e`` (`editable installation`) flag can be included for developing or troubleshooting, and links the executables to the local repository files rather than installing/copying these into the python environment.
