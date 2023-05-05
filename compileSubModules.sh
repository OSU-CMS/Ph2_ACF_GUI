#!/bin/sh

echo "-----Compiling Ph2_ACF-----"
echo "---------------------------"

export Ph2_ACF_VERSION="v4-12"

mkdir Ph2_ACF/build/
cd Ph2_ACF
source ./setup.sh
cd build
cmake3 ..
make -j$(nproc)
cd ..
source ./setup.sh

echo "-----Ph2_ACF Setup Complete-----"
echo "                                "

echo "-----Compiling power_supply-----"
echo "--------------------------------"

cd ../power_supply/
mkdir build
source ./setup.sh
cd build
cmake3 ..
make -j$(nproc)
cd ..
source ./setup.sh
cd ../

echo "-----power_supply Setup Complete-----"
echo "                                     "

echo "-----Settings up GUI Environment-----"
echo "-------------------------------------"

export DATA_dir=$PWD/data/TestResults
export GUI_dir=$PWD

export UsePowerSupplyLib=false

if [ ! -d $PH2ACF_BASE_DIR ]; then
    echo "Error: PH2ACF_BASE_DIR not found!"
fi;

if [ ! -d $DATA_dir ]; then
    echo "DATA_dir not found!  Making it now!"
    mkdir -p -m777 $DATA_dir
fi;

if [ ! -d $PH2ACF_BASE_DIR/test ]; then 
    echo "Test directory not found!  Making it now!"
    mkdir -p -m777 $PH2ACF_BASE_DIR/test;
fi;

if [ ! -d $DATA_dir ]; then
    echo "Failed to create TestResults folder under DATA_dir, please check"
fi;

if [ ! -d $PH2ACF_BASE_DIR/test ]; then
    echo "Failed to create test folder under PH2ACF_BASE_DIR, please check"
fi;

if [ "$UsePowerSupplyLib" = true ]
then
    export PowerSupplyArea=$PWD/power_supply
else
    unset PowerSupplyArea
fi
export PYTHONPATH=${PYTHONPATH}:${GUI_dir}

#export DATA_dir=/Users/czkaiweb/Research/data/
chmod 755 $PWD/Gui/GUIutils/*.sh

#cd $PH2ACF_BASE_DIR
#source setup.sh
#export Ph2_ACF_VERSION=$(git describe --tags --abbrev=0)

cd $GUI_dir

echo "-----GUI Environment Setup Complete-----"
echo "                                        "