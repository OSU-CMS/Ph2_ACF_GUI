#!/usr/bin/env bash

if ! [ -n "${VIRTUAL_ENV}" ]; then
    echo 'ERROR: Not within a virtual environment - refusing to patch pyvisa-sim'
    exit 1
fi

LIBDIR="${VIRTUAL_ENV}"/lib/python*/site-packages/

if ! [ -d ${LIBDIR}/pyvisa_sim/ ]; then
    echo 'ERROR: PyVISA-sim does not seem to be installed. Cannot patch!'
    exit 1
fi

echo "Will patch "${LIBDIR}/pyvisa_sim/

if grep 'python3\.7' <(echo $LIBDIR) > /dev/null; then
    echo 'Patching pyvisa-sim-0.5.1'
    patch -l -p 1 -d ${LIBDIR} < patch-pyvisa-sim-0.5.1.patch
else
    echo 'Patching pyvisa-sim-0.6.0'
    patch -l -p 1 -d ${LIBDIR} < patch-pyvisa-sim-0.6.0.patch
fi
