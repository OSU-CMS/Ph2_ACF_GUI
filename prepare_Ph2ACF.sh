#!/bin/sh

cd $PH2ACF_BASE_DIR
source setup.sh
git config --global --add safe.directory /home/cmsTkUser/Ph2_ACF_GUI/Ph2_ACF
export PH2ACF_VERSION=$(git describe --tags)

cd ${GUI_dir}
source symlinks.sh

# For some reason we get "dubious ownership" issues with felis
git config --global --add safe.directory /home/cmsTkUser/Ph2_ACF_GUI/felis
git config --global --add safe.directory /home/cmsTkUser/Ph2_ACF_GUI/icicle
git config --global --add safe.directory /home/cmsTkUser/Ph2_ACF_GUI
git config --global --add safe.directory /home/cmsTkUser/Ph2_ACF_GUI/InnerTrackerTests

export PH2_ACF_GUI_VERSION=$(git describe --tags)
export FELIS_VERSION=$(git -C felis describe --tags)
export ICICLE_VERSION=$(git -C icicle describe --tags)
export INNER_TRACKER_TESTS_VERSION=$(git -C InnerTrackerTests describe --tags)

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

echo "Logging into DCA to cache credentials for database access..."
python3 python/rhapi.py --login --no-save-password --clean -u https://cmsdca.cern.ch/trk_rhapi1

echo "You can now open the GUI by doing 'python3 QtApplication.py'."

python3 QtApplication.py #this gets commented out for some reason (i forgot why) during testing sometimes
