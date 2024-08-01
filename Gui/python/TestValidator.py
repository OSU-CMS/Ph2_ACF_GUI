import ROOT

ROOT.gROOT.SetBatch(ROOT.kTRUE)

import os
from Gui.python.logging_config import logger
from InnerTrackerTests.TestSequences import Test_to_Ph2ACF_Map

def ResultGrader(felis, outputDir, testName, testIndexInSequence, runNumber, module_data):
    try:
        module_name = module_data['module'].getModuleName()
        module_type = module_data['module'].getModuleType()
        if 'IVCurve' in testName or 'SLDOScan' in testName:
            explanation = 'No grading currently available for IVCurve or SLDOScan.'
            return {module_name:(True, explanation)}
        
        root_file_name = testName.split('_')[0]
        if 'SCurveScan' in root_file_name:
            root_file_name = 'SCurve'
        elif 'Threshold' in root_file_name:
            root_file_name = root_file_name.replace('Threshold', 'Thr')
        ROOT_file_path = "{0}/Run{1}_{2}.root".format(
            outputDir, runNumber, root_file_name
        )
        chip_canvas_path_template = "Detector/Board_{boardID}/OpticalGroup_{ogID}/Hybrid_{hybridID}/Chip_{chipID:02d}"
        active_chips = [chip.getID() for chip in module_data['module'].getChips().values() if chip.getStatus()]
        chip_canvases = [
            chip_canvas_path_template.format(
                boardID=module_data['boardID'],
                ogID=module_data['ogID'],
                hybridID=module_data['hybridID'],
                chipID=int(chipID)
            )
            for chipID in active_chips
        ]
        relevant_files = [outputDir+"/"+os.fsdecode(file) for file in os.listdir(outputDir)]
        
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
        #logger.error("An error was thrown while grading: {}".format(repr(err)))
        return {module_name:(False, repr(err))}