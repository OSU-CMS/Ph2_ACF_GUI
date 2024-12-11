#!/bin/sh

cd $PH2ACF_BASE_DIR
source setup.sh

cd ${GUI_dir}
source symlinks.sh

cd ${GUI_dir}/Gui/python

wget -O rhapi.py https://gitlab.cern.ch/cms-ph2-database/resthub-dev/-/raw/master/clients/python/src/main/python/rhapi.py

cd ${GUI_dir}/Gui

echo "You can now open the GUI by doing 'python3 QtApplication.py'."

python3 QtApplication.py