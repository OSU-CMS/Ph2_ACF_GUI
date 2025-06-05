import sys
import os
import Ph2_ACF_StateMachine
import argparse

sys.path.insert(1, os.getenv('PH2ACF_BASE_DIR'))
import lib.Ph2_ACF_PythonInterface as Ph2_ACF

###########OPTIONS
parser = argparse.ArgumentParser(description='Command line parser of skim options')
parser.add_argument('-c', dest='configurationFile', help='xml configuration file'              , required = True)
parser.add_argument('-b', dest='boardId'          , help='boardId, default = 0'                , default = 0)
parser.add_argument('-l', dest='listFirmwares'    , help='list firmwares on SD card'           , action='store_true')
parser.add_argument('-i', dest='firmwareName'     , help='Without -f: load image from SD card to FPGA\nWith    -f: name of image written to SD card\n-f specifies the source filename', required = False)
parser.add_argument('-f', dest='uploadFile'       , help='Local FPGA Bitstream file (*.mcs format for GLIB or *.bit/*.bin format for CTA boards)', required = False)
parser.add_argument('-o', dest='downloadFile'     , help='Download an FPGA configuration from SD card to file (only for CTA boards)', required = False)
parser.add_argument('-d', dest='deleteFirmware'   , help='Delete a firmware image on SD card (works only with CTA boards)', required = False)

args = parser.parse_args()
configurationFile = args.configurationFile
boardId           = args.boardId
deleteFirmware    = args.deleteFirmware

Ph2_ACF.configureLogger(os.getenv('PH2ACF_BASE_DIR') + "/settings/logger.conf")

theStateMachine = Ph2_ACF_StateMachine.StateMachine()

if args.listFirmwares:
    firmwareList = theStateMachine.listFirmware(configurationFile, boardId)
    print(str(len(firmwareList)) + " firmware images on SD card:")
    for firmware in firmwareList:
        print(firmware)

if args.deleteFirmware is not None:
    print("Deleting " + args.deleteFirmware + " from SD card...")
    theStateMachine.deleteFirmware(configurationFile, args.deleteFirmware, boardId)


if args.firmwareName is not None:
    firmwareName = args.firmwareName
    if args.uploadFile is not None and args.downloadFile is None:
        print("Uploading " + args.uploadFile + " on SD card with name " + firmwareName + "...")
        theStateMachine.uploadFirmware(configurationFile, firmwareName, args.uploadFile, boardId)
    elif args.downloadFile is not None and args.uploadFile is None:
        print("Downloading " + firmwareName + " on SD card into " + args.downloadFile + "...")
        theStateMachine.downloadFirmware(configurationFile, firmwareName, args.downloadFile, boardId)
    elif args.downloadFile is None and args.uploadFile is None:
        print("Loading " + firmwareName + " into the FPGA...")
        theStateMachine.loadFirmware(configurationFile, firmwareName, boardId)
    else:
        print("Error: -f and -o options cannot be specified at the same time")
        sys.exit(1)



print("-------------------------------------------------")
print("Firmware query result:")
if(theStateMachine.isSuccess()):
    print("Success")
else:
    print("Failed, Error message = " + theStateMachine.getErrorMessage())
