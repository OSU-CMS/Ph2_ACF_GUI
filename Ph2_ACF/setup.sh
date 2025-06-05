#!/bin/bash

###################################
# Enable devtools-10 for C++ > 14 #
###################################
[ -f /etc/redhat-release ] && OS_release="rh_$(cat /etc/redhat-release | tr -dc '0-9.'|cut -d \. -f1)"

if [[ $OS_release == "rh_9" ]]; then
  source scl_source enable gcc-toolset-12
else
  echo OS Release not supported
fi

###########
# Ph2_ACF #
###########
export PH2ACF_BASE_DIR=$(pwd)

##########
# CACTUS #
##########
export CACTUSROOT=/opt/cactus

##########
# PYTHON #
##########
alias PythonController.py="python3 ${PH2ACF_BASE_DIR}/pythonUtils/PythonController.py"
alias fpgaconfig.py="python3 ${PH2ACF_BASE_DIR}/pythonUtils/fpgaconfig.py"

########
# ROOT #
########
THISROOTSH=${ROOTSYS}/bin/thisroot.sh
[ ! -f ${THISROOTSH} ] || source ${THISROOTSH}
unset THISROOTSH

if ! command -v root &> /dev/null; then
  printf "%s\n" ">> ERROR -- CERN ROOT is not available; please install it before using Ph2_ACF (see README)"
  return 1
fi

####################
# External Plugins #
####################
export EXTERNAL_TOOLS_BASE_DIR=${PH2ACF_BASE_DIR%/*}
export POWERSUPPLYDIR=$EXTERNAL_TOOLS_BASE_DIR/power_supply

##############################################################################################
# These are git references for the dependencies that are included via CMake ExternalProjects #
##############################################################################################
export PH2_TCUSB_REF=889b673e9d9582dab64cfeb60990800970ee47af
export EUDAQ_REF=fa186e2fc893db41b56d4eff390ba9355ebc9be2
export PYBIND11_REF=2.9.2

#######
# ZMQ #
#######
export ZMQ_HEADER_PATH=/usr/include/zmq.hpp

###########
# ANTENNA #
###########
export ANTENNADIR=$EXTERNAL_TOOLS_BASE_DIR/CMSPh2_AntennaDriver
export ANTENNALIB=$ANTENNADIR/lib

###########
# HMP4040 #
###########
export USBINSTDIR=$EXTERNAL_TOOLS_BASE_DIR/Ph2_USBInstDriver
export USBINSTLIB=$USBINSTDIR/lib

#########
# EUDAQ #
#########
export EUDAQLIB=$EUDAQDIR/lib

############
# Pybind11 #
############
export PYTHONINCLUDE=/usr/include/python3.8/:${PYTHONINCLUDE}
if [ -d "$PH2ACF_BASE_DIR/../pybind11-${PYBIND11_REF}" ]; then
  export PYBIND11=$PH2ACF_BASE_DIR/../pybind11-${PYBIND11_REF}/
  export PYBIND11INCLUDE=$PYBIND11/include
fi

##########
# System #
##########
export PATH=$PH2ACF_BASE_DIR/bin:$PH2ACF_BASE_DIR/ProductionToolsIT/LDACLINCalibration:$PATH
export LD_LIBRARY_PATH=$USBINSTLIB:$ANTENNALIB:$PH2ACF_BASE_DIR/RootWeb/lib:$CACTUSROOT/lib:$PH2ACF_BASE_DIR/lib:$EUDAQLIB:$LD_LIBRARY_PATH

#########
# Flags #
#########
export HttpFlag='-D__HTTP__'
export ZmqFlag='-D__ZMQ__'
export USBINSTFlag='-D__USBINST__'
export Amc13Flag='-D__AMC13__'
export TCUSBFlag='-D__TCUSB__'
export AntennaFlag='-D__ANTENNA__'
export UseRootFlag='-D__USE_ROOT__'
export MultiplexingFlag='-D__MULTIPLEXING__'
export EuDaqFlag='-D__EUDAQ__'

#####################
# Compilation flags #
#####################

################
# C++ standard #
################
export STDCXX="17"

###################################################
# Stand-alone application, without data streaming #
###################################################
export CompileForHerd=false
export CompileForShep=false

################################################
# Stand-alone application, with data streaming #
################################################
# export CompileForHerd=true
# export CompileForShep=true

####################
# Herd application #
####################
# export CompileForHerd=true
# export CompileForShep=false

####################
# Shep application #
####################
# export CompileForHerd=false
# export CompileForShep=true

################################
# Compile with EUDAQ libraries #
################################
export CompileWithEUDAQ=false

###############################
# Compile with TC_USB library #
###############################
export CompileWithTCUSB=false

##########################
# Compile with sanitizer #
##########################
export EnableSanitizer=false
if [[ $EnableSanitizer == "true" ]]; then
  export UBSAN_OPTIONS=print_stacktrace=1
else
  unset UBSAN_OPTIONS
fi

########################
# Clang-format command #
########################
clang_command="clang-format"
alias formatAll="find ${PH2ACF_BASE_DIR} -type f \\( -name \"*.cc\" -o -name \"*.h\" \\) ! -path \"${PH2ACF_BASE_DIR}/MessageUtils/*\" ! -path \"${PH2ACF_BASE_DIR}/*/_deps/*\" | xargs ${clang_command} -i"

if [[ $1 == "ci" ]]; then
    export CompileForHerd=false
    export CompileForShep=false
    export CompileWithEUDAQ=false
    export CompileWithTCUSB=false
    export EnableSanitizer=false
fi

echo "=== DONE: you can now run cmake ==="
