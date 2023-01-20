export Ph2_ACF_AREA=/home/RD53A/workspace/v4.0.6/Ph2_ACF
export DATA_dir=/home/RD53A/data/TestResults
export GUI_dir=$PWD

UsePowerSupplyLib=false

if [ ! -d $Ph2_ACF_AREA ]; then
    echo "Error: Ph2_ACF_AREA not found!"
fi;

if [ ! -d $DATA_dir ]; then
    echo "Error: DATA_dir not found!"
fi;

if [ ! -d $Ph2_ACF_AREA/test ]; then
    mkdir -p $Ph2_ACF_AREA/test;
fi;

if [ ! -d $Ph2_ACF_AREA/test ]; then
    echo "Failed to create test folder under Ph2_ACF_AREA, please check"
fi;



if [ "$UsePowerSupplyLib" = true ]
then
    export PowerSupplyArea=$PWD/power_supply
else
    unset PowerSupplyArea
fi
export PYTHONPATH=/usr/lib64/root:${GUI_dir}

#export DATA_dir=/Users/czkaiweb/Research/data/
chmod 755 $PWD/Gui/GUIutils/*.sh

cd $Ph2_ACF_AREA
source setup.sh
export Ph2_ACF_VERSION=$(git describe --tags --abbrev=0)

if [ "$UsePowerSupplyLib" = true ]
then
    cd $PowerSupplyArea
    source setup.sh
fi
cd $GUI_dir

