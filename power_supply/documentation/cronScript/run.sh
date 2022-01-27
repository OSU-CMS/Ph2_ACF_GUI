#!/bin/bash
cd /afs/cern.ch/user/a/ancasses/eos/www/private/cms/power_supply/documentation/
doxygen config
rsync -avzhe ssh --progress --delete html/ /afs/cern.ch/user/a/ancasses/eos/www/private/cms/DevicesCode/
#rsync -avzhe --progress --delete html /home/antonio/cernbox/www/private/cms/MattiaCodeDoxygen/
