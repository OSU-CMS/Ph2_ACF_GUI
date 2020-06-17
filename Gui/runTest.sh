#! /bin/bash 

cd /home/bmanley/Ph2_ACF/
source setup.sh
mkdir newTest 
cp settings/RD53Files/CMSIT_RD53.txt newTest/
cp settings/CMSIT.xml newTest/
cd newTest/
CMSITminiDAQ