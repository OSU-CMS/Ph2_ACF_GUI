# IT middleware setup and use

## Recommended software and firmware versions

- Software git branch / tag : `Dev` / `v6-06`
- Firmware tag: `v5-00`

## Important webpages and information

- Mattermost forum: [`cms-it-daq`](https://mattermost.web.cern.ch/cms-it-daq/)

### Documentation

- Detailed description of the various calibrations: <https://cernbox.cern.ch/s/uSezc8ErG7F4tJ0>
- ROC tuning sequence: <https://www.overleaf.com/read/ffpkqnjjjscd>
- CROC testing guide (old): <https://croc-testing-user-guide.docs.cern.ch/>

### School

- Latest IT-DAQ school: <https://indico.cern.ch/event/1374747/>

### TUI/GUI/GS

- Text-based User Interface (TUI) - aka Dirigent: <https://gitlab.cern.ch/cms_tk_ph2/dirigent/>
- Graphical-based User Interface (GUI) - aka Ohio-GUI: <https://gitlab.cern.ch/cms_tk_ph2/Ph2_ACF_GUI/>
- Grading Software - aka Panthera: you can use Panthera purely via the web, or by using GUI or TUI.
If you want to use it purely by the web, just go to: <https://panthera.fit.edu/> and go to “Add Modules”
to add modules and then “Upload Electrical” to upload Sequences by hand, or “Upload Mechanical” to upload mechanical
information by hand. There are “?” icons that give you helpful hints. However, if you want to use Panthera using GUI
or TUI, you should get the requisite packages when you install GUI or TUI itself.
There’s nothing you need to install separately. For bug report use: <https://gitlab.cern.ch/sdemares/panthera/>,
while for Felis use <https://gitlab.cern.ch/sdemares/felis/>.
- Submodule providing common (TUI/GUI) calibration settings: <https://gitlab.cern.ch/cms_tk_ph2/inner-tracker-tests/>

### Miscellanea

- Program to generate enable/injection patterns for x-talk studies: `pyUtilsIT/ManipulateITchipMask.py`
- Mask converter from `Ph2_ACF` to `Alki's` code: `pyUtilsIT/ConvertPh2ACFMask2Alkis.py`
