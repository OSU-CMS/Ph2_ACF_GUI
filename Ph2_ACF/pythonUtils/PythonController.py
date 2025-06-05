import sys
import os
import Ph2_ACF_StateMachine
import argparse
from argparse import RawTextHelpFormatter

sys.path.insert(1, os.getenv('PH2ACF_BASE_DIR'))
import lib.Ph2_ACF_PythonInterface as Ph2_ACF

def getNewRunNumber():
    runNumberFileName = os.getenv('PH2ACF_BASE_DIR') + "/RunNumbers.dat"
    
    try:
        file = open(runNumberFileName, "r")
        with file as runNumberFile:
            for line in runNumberFile:
                pass
            runNumber = int(line) + 1
    except FileNotFoundError:
        file = open(runNumberFileName, "w")
        runNumber = 0
    
    
    runNumberFile = open(runNumberFileName, "a")
    runNumberFile.write(str(runNumber) + "\n")
    runNumberFile.close()
    return runNumber

###########OPTIONS
theStateMachine = Ph2_ACF_StateMachine.StateMachine()
listOfCalibration = theStateMachine.getCalibrationList()

listOfCalibrationPrint = ""
for hardwareType, calibrationListPerHardware in listOfCalibration.items():
    listOfCalibrationPrint = listOfCalibrationPrint + "--------------------------------------------------------------------------\n";
    listOfCalibrationPrint = listOfCalibrationPrint + "\033[1m\033[32m" + hardwareType + " Calibrations\033[0m" + "\n"
    listOfCalibrationPrint = listOfCalibrationPrint + "--------------------------------------------------------------------------\n";
    for calibrationName, subCalibrationList in calibrationListPerHardware.items():
        listOfCalibrationPrint = listOfCalibrationPrint + "\033[1m\033[34m" + "\t" + calibrationName + "\033[0m" + "\n"
        for subCalibration in subCalibrationList:
            listOfCalibrationPrint = listOfCalibrationPrint + "\t\t" + subCalibration[0]
            if subCalibration[1] != "": listOfCalibrationPrint = listOfCalibrationPrint + ": " + subCalibration[1]
            listOfCalibrationPrint = listOfCalibrationPrint + "\n"

parser = argparse.ArgumentParser(description='Command line parser of skim options', formatter_class=RawTextHelpFormatter)
parser.add_argument('-f', dest='configurationFile', help='xml configuration file', required = True)
parser.add_argument('-c', dest='calibrationName'  , help='calibration name. Available calibrations:\n' + listOfCalibrationPrint, required = True)

args = parser.parse_args()
configurationFile = args.configurationFile
calibrationName   = args.calibrationName

Ph2_ACF.configureLogger(os.getenv('PH2ACF_BASE_DIR') + "/settings/logger.conf")

theStateMachine.setConfigurationFiles(configurationFile)
theStateMachine.setCalibrationName(calibrationName)
theStateMachine.setRunNumber(getNewRunNumber())

theStateMachine.runCalibration()

print("-------------------------------------------------")
print("Calibration " + calibrationName + " result:")
if(theStateMachine.isSuccess()):
    print("Success")
else:
    print("Failed, Error message = " + theStateMachine.getErrorMessage())
    sys.exit(999)
