import os
import ROOT
import traceback
import re

from InnerTrackerTests.TestSequences import Test_to_Ph2ACF_Map, CompositeTests_Modules
from Gui.GUIutils.guiUtils import isCompositeTest
from Gui.python.logging_config import get_logger

logger = get_logger(__name__)

ROOT.gROOT.SetBatch(ROOT.kTRUE)


def ResultGrader(
    felis,
    outputDir,
    testName,
    testIndexInSequence,
    runNumber,
    module_data,
    BBanalysis_root_files,
    sequence,
    registerKey,
    communicationTestResults
):
    try:

        if isCompositeTest(sequence) and CompositeTests_Modules[registerKey][sequence][testIndexInSequence] != testName:
            logger.error(
                f"Test name didn't match expected test sequence name\n"
                f"Expected Test Name: {CompositeTests_Modules[registerKey][sequence][testIndexInSequence]}\n"
                f"Received Test Name: {testName}"
            )
            raise Exception("Test name doesn't match expected sequence name! Something went wrong.")


        module_name = module_data["module"].getModuleName()
        module_type = module_data["module"].getModuleType()
        module_version = module_data["module"].getModuleVersion()
        module_hybridID = module_data["module"].getFMCPort()
       

            


        root_file_name = testName.split("_")[0]
        if "SCurveScan" in root_file_name:
            root_file_name = "SCurve"
        elif "GainScan" in root_file_name:
            root_file_name = "Gain"
        elif "Threshold" in root_file_name:
            root_file_name = root_file_name.replace("Threshold", "Thr")
        if "IVCurve" in testName:
            root_file_name = testName.split("_")[0] + "_" + module_name

            relevant_files = [
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir) if module_name in file or file.endswith(".xml")
            ]
            dqmpattern = re.compile(rf"_Hybrid_{module_hybridID}\.root$")
            relevant_files.extend([outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir) if dqmpattern.search(file)])

            logger.info(f"{relevant_files=}")

            _1, _2 = felis.set_module(
                name_module = module_name,
                subdetector = module_type.split(" ")[0],
                type_module = module_type.split(" ")[2].replace("Quad", "2x2"),
                croc_version = module_version.strip("v"),
                has_sensor = True,
            )
            status, message, sanity, explanation = felis.set_result(
                paths_files = relevant_files,
                name_module = module_name,
                name_test = f"{testIndexInSequence:02d}_{testName}",
                type_test = "ivcurve",
            )
        
        elif "SLDOScan" in testName:
            root_file_name = testName.split("_")[0] + "_" + module_name

            relevant_files = [
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir)
            ]
            _1, _2 = felis.set_module(
                name_module = module_name,
                subdetector = module_type.split(" ")[0],
                type_module = module_type.split(" ")[2].replace("Quad", "2x2"),
                croc_version = module_version.strip("v"),
                has_sensor = True,
            )
            status, message, sanity, explanation = felis.set_result(
                paths_files = relevant_files,
                name_module = module_name,
                name_test = f"{testIndexInSequence:02d}_{testName}",
                type_test = "sldo",
            )

        elif "Trimbit" in testName:
            relevant_files = [
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir)
            ]
            _1, _2 = felis.set_module(
                name_module = module_name,
                subdetector = module_type.split(" ")[0],
                type_module = module_type.split(" ")[2].replace("Quad", "2x2"),
                croc_version = module_version.strip("v"),
                has_sensor = True,
            )
            status, message, sanity, explanation = felis.set_result(
                paths_files = relevant_files,
                name_module = module_name,
                name_test = f"{testIndexInSequence:02d}_{testName}",
                type_test = "trimbitscan",
            )


        elif "IREF" in testName:
            relevant_files = [
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir) if module_name in file or file.endswith(".xml")
            ]
            dqmpattern = re.compile(rf"_Hybrid_{module_hybridID}\.root$")
            relevant_files.extend([outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir) if dqmpattern.search(file)])

            print("relevant_files:", relevant_files)
            _1, _2 = felis.set_module(
                name_module = module_name,
                subdetector = module_type.split(" ")[0],
                type_module = module_type.split(" ")[2].replace("Quad", "2x2"),
                croc_version = module_version.strip("v"),
                has_sensor = True,
            )
            status, message, sanity, explanation = felis.set_result(
                paths_files = relevant_files,
                name_module = module_name,
                name_test = f"{testIndexInSequence:02d}_{testName}",
                type_test = "irefgadc",
            )

        elif "CommunicationTest" in testName:
            module_name = module_data["module"].getModuleName()
            comm_result = communicationTestResults.get(module_name)
            # Get IREF match status for this module
            relevant_files = [
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir)
            ]

            _1, _2 = felis.set_module(
                name_module = module_name,
                subdetector = module_type.split(" ")[0],
                type_module = module_type.split(" ")[2].replace("Quad", "2x2"),
                croc_version = module_version.strip("v"),
                has_sensor = True,
            )
            status, message, sanity, explanation = felis.set_result(
                paths_files = relevant_files,
                name_module = module_name,
                name_test = f"{testIndexInSequence:02d}_{testName}",
                type_test = "commtest",
                comm_result=comm_result,
            )

        else:

            # Note: This may be useful
            # chip_canvas_path_template = "Detector/Board_{boardID}/OpticalGroup_{ogID}/Hybrid_{hybridID}/Chip_{chipID:02d}"

            # active_chips = [
            #     chip.getID()
            #     for chip in module_data["module"].getChips().values()
            #     if chip.getStatus()
            # ]

            #relevant_files = [
            #    outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir)
            #]
            
            relevant_files = [
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir) if module_name in file or file.endswith(".xml")
            ]
            dqmpattern = re.compile(rf"_Hybrid_{module_hybridID}\.root$")
            relevant_files.extend([outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir) if dqmpattern.search(file)])
            #relevant_files.extend([outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir) if file.endswith(".root")])
            logger.debug(f"{relevant_files=}")
            _1, _2 = felis.set_module(
                name_module = module_name,
                subdetector = module_type.split(" ")[0],
                type_module = module_type.split(" ")[2].replace("Quad", "2x2"),
                croc_version = module_version.strip("v"),
                has_sensor = True,
                link_production_db = "https://www.physics.purdue.edu/cmsfpix/Phase2_Test/w.php?sn={module_name}",
            )

            status, message, sanity, explanation = felis.set_result(
                paths_files = relevant_files,
                name_module = module_name,
                name_test = f"{testIndexInSequence:02d}_{testName}",
                type_test = Test_to_Ph2ACF_Map[testName],
            )
        if not status:
            raise RuntimeError(message)

        return {module_name: (status and sanity, explanation)}, BBanalysis_root_files
    except Exception as err:
        logger.error("An error was thrown while grading: {}".format(repr(err)))
        logger.error(traceback.format_exc())
        return {module_name: (False, repr(err))}, BBanalysis_root_files
