export Ph2_ACF_AREA=/home/RD53A/workspace/v4.0.1/Ph2_ACF
UsePowerSupplyLib=true
if [ "$UsePowerSupplyLib" = true ]
then
    export PowerSupplyArea=/home/RD53A/workspace/PowerSupply/power_supply
fi
export GUI_dir=$PWD
export PYTHONPATH=${PYTHONPATH}:${GUI_dir}
export DATA_dir=/home/RD53A/data/TestResults
#export DATA_dir=/Users/czkaiweb/Research/data/
chmod 755 $PWD/Gui/GUIutils/*.sh

cd $Ph2_ACF_AREA
source setup.sh
if [ "$UsePowerSupplyLib" = true ]
then
    cd $PowerSupplyArea
    source setup.sh
fi
cd $GUI_dir
