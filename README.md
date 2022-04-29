# Ph2_ACF_GUI
Grading GUI for CMS Tracker Phase2 Acquisition &amp; Control Framework

Uses the following python packages: PyQt5 (https://pypi.org/project/PyQt5/), pyqt-darktheme (https://pypi.org/project/pyqt-darktheme/)

The following instructions assume that you already have Ph2_ACF set up.

##########################
Set up the software environment:
##########################

1. Install PyQt5:
'''
pip install PyQt5
'''

2. Install pyqt-darktheme:
'''
pip install pyqt-darktheme
'''

3. Install MySQL connector:
'''
pip install mysql-connector-python
'''

4. Install Pillow:
'''
pip install Pillow
'''

5. Install NumPy:
'''
pip install numpy
'''

6. Install Matplotlib:
'''
pip install matplotlib
'''

7. Install lxml:
'''
pip install lxml
'''
8. Install pyvisa:
'''
pip install pyvisa
pip install pyvisa-py
'''
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

