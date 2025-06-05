#!/bin/bash

echo Setting up this to create mini-websites on /tmp
echo This should be changed in order to point to a website reachable from afs
tar -xf style.tar -C /afs/cern.ch/work/m/mersi/Ph2BeamTest

mkdir -p lib
make && ./example.bin $1

echo You ened a valid kerberos ticket of one of the following users:
pts membership -nameorid  mersi:ph2beamtest
echo type
echo "# kinit youUserName@CERN.CH"
echo to obtain one
echo Now try looking at this:
echo http://cern.ch/cms-tkph2bt
