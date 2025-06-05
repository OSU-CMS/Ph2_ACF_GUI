import os
import sys
import datetime

def checkIfFileExists(fileName):
    if os.path.exists(fileName):
        print ("File ", fileName, " already exists\naborting...")
        sys.exit(1)

def createFileFromTemplate(templateFile, outputFile):
    with open(templateFile, 'r') as file : filedata = file.read()

    # Replace the target string
    filedata = filedata.replace('CLASS_NAME_TEMPLATE', className)
    filedata = filedata.replace('CLASS_AUTOR'        , autor    )
    filedata = filedata.replace('TODAY_DATE'         , todayDate)

    # Write the file out again
    with open(outputFile, 'w') as file: file.write(filedata)
    print ("Created file ", outputFile)


if not 'PH2ACF_BASE_DIR' in os.environ:
    print ("Please source the setup.sh to set the enviromental variables\naborting...")
    sys.exit(1)

ph2acfDirectory = os.environ['PH2ACF_BASE_DIR']

className = input("Please enter the calibration name: ")
#className = className.decode(encoding='utf-8')

if not className.isalnum():
    print ("C++ class names can contain only letters or numbers\naborting...")
    sys.exit(1)

if ' ' in className:
    print ("C++ class names cannot contain spaces\naborting...")
    sys.exit(1)

if className[0].isnumeric():
    print ("C++ class names cannot start with a number\naborting...")
    sys.exit(1)

if not className[0].isupper():
    print ("Adopted convention required the class name to start with upper case\naborting...")
    sys.exit(1)

templateFile    = ph2acfDirectory + "/pythonUtils/CalibrationTemplates/{0}"
calibrationFile = ph2acfDirectory + "/tools/{0}"
dqmFile         = ph2acfDirectory + "/DQMUtils/DQMHistogram{0}"

checkIfFileExists(calibrationFile.format(className + ".h" ))
checkIfFileExists(calibrationFile.format(className + ".cc"))
checkIfFileExists(dqmFile        .format(className + ".h" ))
checkIfFileExists(dqmFile        .format(className + ".cc"))

autor = input("Please enter the autor name: ")

todayDate = datetime.datetime.now().strftime('%d/%m/%y')



createFileFromTemplate(templateFile.format("templateCalibrationHeader.txt"         ), calibrationFile.format(className + ".h" ))
createFileFromTemplate(templateFile.format("templateCalibrationImplementation.txt"), calibrationFile.format(className + ".cc"))
createFileFromTemplate(templateFile.format("templateDQMHeader.txt"                ), dqmFile        .format(className + ".h" ))
createFileFromTemplate(templateFile.format("templateDQMImplementation.txt"        ), dqmFile        .format(className + ".cc"))
