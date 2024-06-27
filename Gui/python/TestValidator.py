import ROOT

ROOT.gROOT.SetBatch(ROOT.kTRUE)

import os
from Gui.python.logging_config import logger
from InnerTrackerTests.TestSequences import Test_to_Ph2ACF_Map

def ResultGrader(felis, inputDir, testName, testIndexInSequence, runNumber, module):
    try:
        module_name = module.getModuleName()
        module_type = module.getModuleType()
        if 'IVCurve' in testName:
            explanation = 'No grading currently available for IVCurve'
            return {module_name:(True, explanation)}
        
        root_file_name = testName.split('_')[0]
        if 'SCurveScan' in root_file_name:
            root_file_name = 'SCurve'
        elif 'Threshold' in root_file_name:
            root_file_name = root_file_name.replace('Threshold', 'Thr')
        ROOT_file_path = "{0}/Run{1}_{2}.root".format(
            inputDir, runNumber, root_file_name
        )
        chip_canvas_path_template = "Detector/Board_0/OpticalGroup_0/Hybrid_0/Chip_{0:02d}"
        active_chips = [chip.getID() for chip in module.getChips().values() if chip.getStatus()]
        chip_canvases = [chip_canvas_path_template.format(int(chipID)) for chipID in active_chips]
        relevant_files = [inputDir+"/"+os.fsdecode(file) for file in os.listdir(inputDir)]
        
        _1, _2 = felis.set_module(
            module_name, module_type.split(" ")[2], module_type.split(" ")[0], True, "link"
        )
        
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
        return {module_name:(False, str(err))}