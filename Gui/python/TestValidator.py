import ROOT

ROOT.gROOT.SetBatch(ROOT.kTRUE)

import os
import sys
from Gui.GUIutils.settings import ModuleLaneMap
from Gui.python.logging_config import logger
from InnerTrackerTests.TestSequences import Test_to_Ph2ACF_Map
sys.path.append("./../felis") #temporary until a new docker image is built with the new python path
from felis import Felis


def ResultGrader(inputDir, testName, runNumber, moduleType):
    #rubric_name = "rubricV2.py"
    try:
        module_name = inputDir.split("Module")[1].split('_')[0]
        ROOT_file_path = "{0}/Run{1}_{2}.root".format(
            inputDir, runNumber, testName.split('_')[0].rstrip("Scan")
        )
        chip_canvases = [
            f"Detector/Board_0/OpticalGroup_0/Hybrid_0/Chip_{i}" for i in ModuleLaneMap[moduleType].values()
        ]
        relevant_files = [os.fsdecode(file) for file in os.listdir(inputDir)]
        
        my_felis = Felis("/home/cmsTkUser/Ph2_ACF_GUI/data/scratch", False)
        sanity, message = my_felis.set_result(
            ROOT_file_path,
            chip_canvases,
            relevant_files,
            module_name,
            testName,
            Test_to_Ph2ACF_Map[testName],
        )

        return {module_name:(sanity, message)}
    except Exception as err:
        print("Failed to get the score: {}".format(repr(err)))
        return {module_name:(False, repr(err))}