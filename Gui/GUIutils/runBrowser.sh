#!/bin/bash
MACRO_PATH=$1
DQMFile=$2
root ${MACRO_PATH}'/OpenBrowser.C("'${DQMFile}'")'
