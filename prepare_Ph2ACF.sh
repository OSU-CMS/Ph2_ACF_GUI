#!/bin/sh

cd $PH2ACF_BASE_DIR
source setup.sh
export PH2ACF_VERSION=$(git describe --tags)



cd ${GUI_dir}
source symlinks.sh
export PH2ACFGUI_VERSION=$(git describe --tags)

cd ${GUI_dir}/Gui/python


# Ensure rhapi.py is accessible by cmsTkUser
if [ -f "rhapi.py" ]; then
    # Check if the file is owned by cmsTkUser
    OWNER=$(stat -c '%U' "rhapi.py")
    if [ "$OWNER" != "cmsTkUser" ]; then
        rm rhapi.py 
        wget -O rhapi.py https://gitlab.cern.ch/cms-ph2-database/resthub-dev/-/raw/master/clients/python/src/main/python/rhapi.py
    fi
else
    wget -O rhapi.py https://gitlab.cern.ch/cms-ph2-database/resthub-dev/-/raw/master/clients/python/src/main/python/rhapi.py
fi

cd ${GUI_dir}/Gui

echo "You can now open the GUI by doing 'python3 QtApplication.py'."

python3 QtApplication.py
