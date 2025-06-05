####################################
# Program that uses Ph2-ACF as API #
####################################
export USER_API=`pwd`
export PH2ACF_BASE_DIR=$HOME/Ph2_ACF
export CACTUSROOT=/opt/cactus
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:$CACTUSROOT/lib

source scl_source enable gcc-toolset-12
