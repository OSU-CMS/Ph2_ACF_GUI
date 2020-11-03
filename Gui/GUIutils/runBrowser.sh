#!/bin/bash
MACRO_PATH=$1
DQMFile=$2
root -l ${MACRO_PATH}'/OpenBrowser.C("'${DQMFile}'")'
