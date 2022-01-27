#!/bin/bash
doxygen config
rsync -avzhe ssh --progress --delete html/ ancasses@lxplus.cern.ch:eos/www/private/cms/DevicesCode/
#rsync -avzhe --progress --delete html /home/antonio/cernbox/www/private/cms/MattiaCodeDoxygen/
