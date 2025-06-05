# IT-DAQ setup and run

1. Folow instructions below to install all needed software packages (like `pugixml`, `boost`, `python`. etc ...)
2. `mkdir choose_a_name`
3. `cp settings/RD53Files/CMSIT_RD53A/B.txt choose_a_name` (if you are using an optical readout you need also: `cp settings/lpGBTFiles/CMSIT_LpGBT-v0(1).txt choose_a_name`)
4. `cp settings/CMSIT_RD53A/B.xml choose_a_name`
5. `cd choose_a_name`
6. Edit the file `CMSIT_RD53A/B.xml` in case you want to change some parameters needed for the calibrations or for configuring the chip
7. Run the command: `CMSITminiDAQ -f CMSIT_RD53A/B.xml -r` to reset the FC7 (just once)
8. Run the command: `CMSITminiDAQ -f CMSIT_RD53A/B.xml -c name_of_the_calibration` (or `CMSITminiDAQ --help` for help)

**N.B.:** to speed up the `IPbus` communication you can implement [this](https://ipbus.web.cern.ch/doc/user/html/performance.html) trick
