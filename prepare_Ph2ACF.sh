#!/bin/sh

cd $PH2ACF_BASE_DIR
source setup.sh

cd ${GUI_dir}/Gui

echo "You can now open the GUI by doing 'python3 QtApplication.py'."

python3 QtApplication.py