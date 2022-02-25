# Ph2_ACF_GUI
Grading GUI for CMS Tracker Phase2 Acquisition &amp; Control Framework

Uses the following python packages: PyQt5 (https://pypi.org/project/PyQt5/), pyqt-darktheme (https://pypi.org/project/pyqt-darktheme/)

The following instructions assume that you already have Ph2_ACF set up.

##########################
Set up the software environment:
##########################

1. Install PyQt5:
pip install PyQt5

2. Install pyqt-darktheme:
pip install pyqt-darktheme

3. Install MySQL connector:
pip install mysql-connector-python

4. Install Pillow:
pip install Pillow

5. Install NumPy:
pip install numpy

6. Install Matplotlib:
pip install matplotlib

7. Install lxml:
pip install lxml

8. Install pyvisa:
pip install pyvisa

9. Modify Setup.sh to include the paths to your Ph2_ACF working area and where you are locally storing test results.

10. Edit fc7_ip_address.txt so that the ip addresses for your fc7 boards are listed.


################################
Run the GUI
################################

```
source Setup.sh
cd Gui
python3 QtApplication.py
```

##############
SET UP USING ANACONDA (WORK IN PROGRESS) 
##############

1. Install the ANACONDA:
https://docs.anaconda.com/anaconda/install/

run "conda --version" to check if the  installation is succesful

2. Create environment for GUI:
Run:
conda create --name GUI python=3.9   # Create the new environment
conda activate GUI                   # Use the environment "GUI"

3. Install PyQt5:
pip install PyQt5

4. Install MySQL connector:
pip install mysql-connector-python

5. Install Pillow:
pip install Pillow

6. Install NumPy:
pip install numpy

7. Install Matplotlib
pip install matplotlib

8. Install lxml
pip install lxml

9. Install pyvisa:
pip install pyvisa

10. Modify Setup.sh to include the paths to your Ph2_ACF working area and where you are locally storing test results.

11. Edit fc7_ip_address.txt so that the ip addresses for your fc7 boards are listed.

With each new ternimal for GUI, run "conda activate GUI" to activate the environment















--------------------------- OLD RECIPE --------------------

Recipe for running pixel gui:
```
cd Gui && python setupDatabase.py
python acfGui.py
```

Add your working area to python PATH:
export PYTHONPATH=$PYTHONPATH:/PATH/TO/YOUR/WORKAREA
