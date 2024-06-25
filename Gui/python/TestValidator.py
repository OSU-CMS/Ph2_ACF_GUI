import ROOT

ROOT.gROOT.SetBatch(ROOT.kTRUE)

import os
from Gui.GUIutils.settings import ModuleLaneMap
from Gui.python.logging_config import logger
from InnerTrackerTests.TestSequences import Test_to_Ph2ACF_Map

def ResultGrader(felis, inputDir, testName, testIndexInSequence, runNumber, moduleType):
    try:
        module_name = inputDir.split("Module")[1].split('_')[0]
        ROOT_file_path = "{0}/Run{1}_{2}.root".format(
            inputDir, runNumber, testName.split('_')[0].rstrip("Scan")
        )
        chipCanvasPath = "Detector/Board_0/OpticalGroup_0/Hybrid_0/Chip_{0:02d}"
        chip_canvases = [
            chipCanvasPath.format(int(chipNumber)) for chipNumber in ModuleLaneMap[moduleType].values()
        ]
        relevant_files = [inputDir+"/"+os.fsdecode(file) for file in os.listdir(inputDir)] 
        
        success, message = felis.set_module(
            module_name, moduleType.split(" ")[2], moduleType.split(" ")[0], True, "link"
        )
        if not success:
            raise RuntimeError(message)
        
        status, message, sanity, explanation = felis.set_result(
            ROOT_file_path,
            chip_canvases,
            relevant_files,
            module_name,
            f"{testIndexInSequence:02d}_{testName}",
            Test_to_Ph2ACF_Map[testName],
        )
        if not status:
            raise RuntimeError(message)
                
        #return {module_name:(sanity, explanation)}
        return {module_name:(True, explanation)}
    except Exception as err:
        logger.error("An error was thrown while grading: {}".format(repr(err)))
        return {module_name:(False, err)}