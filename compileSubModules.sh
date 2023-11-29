#!/bin/sh

echo "-----update submdoules-----"
git pull --recurse-submodules

echo "-----Compiling Ph2_ACF-----"
echo "---------------------------"

export Ph2_ACF_VERSION="v4-14"

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


echo "-----Settings up GUI Environment-----"
echo "-------------------------------------"

#export DATA_dir=$PWD/data/TestResults
#export GUI_dir=$PWD

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

#export Ph2_ACF_VERSION=$(git describe --tags --abbrev=0)

cd $GUI_dir

echo "-----GUI Environment Setup Complete-----"
echo "                                        "

#download and save arduino command line interface into the folder $PWD/bin
#echo "----downloading and installing Arduino command line interface---"
#curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
#download the compilation tool-kit for UNO board
#$PWD/bin/arduino-cli core install arduino:avr