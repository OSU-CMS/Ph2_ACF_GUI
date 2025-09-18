import os
import ROOT
import traceback

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
    communicationTestResults,
    iref_match_status=None
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
       
        if "CommunicationTest" in testName:
            module_name = module_data["module"].getModuleName()
            comm_result = communicationTestResults.get(module_name)
            # Get IREF match status for this module
            iref_status = iref_match_status.get(module_name) if iref_match_status else None
            logger.info(f"iref match status is: {iref_match_status}")
            if comm_result is None:
                return {module_name: (False, "CommunicationTest did not complete")}, BBanalysis_root_files
            if comm_result is True:
                if iref_status is True:
                    return {module_name: (True, "CommunicationTest successful. IREF values match.")}, BBanalysis_root_files
                elif iref_status is False:
                    return {module_name: (False, "CommunicationTest successful but IREF values do not match")}, BBanalysis_root_files
                else:
                    return {module_name: (False, "CommunicationTest successful but IREF status unknown")}, BBanalysis_root_files
            else:
                return {module_name: (False, "CommunicationTest failed")}, BBanalysis_root_files


        root_file_name = testName.split("_")[0]
        if "SCurveScan" in root_file_name:
            root_file_name = "SCurve"
        elif "GainScan" in root_file_name:
            root_file_name = "Gain"
        elif "Threshold" in root_file_name:
            root_file_name = root_file_name.replace("Threshold", "Thr")
        if "IVCurve" in testName:
            root_file_name = testName.split("_")[0] + "_" + module_name

            ROOT_file_path = "{0}/Result_{1}.root".format(outputDir, root_file_name)

            relevant_files = [
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir)
            ]

            _1, _2 = felis.set_module(
                module_name,
                module_type.split(" ")[0],
                module_type.split(" ")[2].replace("Quad", "2x2"),
                module_version.strip("v"),
                True,
            )
            status, message, sanity, explanation = felis.set_result(
                relevant_files,
                module_name,
                f"{testIndexInSequence:02d}_{testName}",
                "ivcurve",
            )
        
        elif "SLDOScan" in testName:
            root_file_name = testName.split("_")[0] + "_" + module_name

            ROOT_file_path = "{0}/Result_{1}.root".format(outputDir, root_file_name)

            relevant_files = [
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir)
            ]
            _1, _2 = felis.set_module(
                module_name,
                module_type.split(" ")[0],
                module_type.split(" ")[2].replace("Quad", "2x2"),
                module_version.strip("v"),
                True,
            )
            status, message, sanity, explanation = felis.set_result(
                relevant_files,
                module_name,
                f"{testIndexInSequence:02d}_{testName}",
                "sldo",
            )
        else:
            ROOT_file_path = "{0}/Run{1}_{2}.root".format(
                outputDir, runNumber, root_file_name
            )
            if testName in (
                "PixelAlive_highcharge_xtalk",
                "PixelAlive_coupled_xtalk",
                "PixelAlive_uncoupled_xtalk",
            ):
                BBanalysis_root_files.extend(
                    outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir) if file.endswith(".root")
                )

            # Note: This may be useful
            # chip_canvas_path_template = "Detector/Board_{boardID}/OpticalGroup_{ogID}/Hybrid_{hybridID}/Chip_{chipID:02d}"

            # active_chips = [
            #     chip.getID()
            #     for chip in module_data["module"].getChips().values()
            #     if chip.getStatus()
            # ]

            relevant_files = [
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir)
            ]

            logger.debug(f"{relevant_files=}")
            _1, _2 = felis.set_module(
                module_name,
                module_type.split(" ")[0],
                module_type.split(" ")[2].replace("Quad", "2x2"),
                module_version.strip("v"),
                True,
                "link",
            )

            status, message, sanity, explanation = felis.set_result(
                relevant_files,
                module_name,
                f"{testIndexInSequence:02d}_{testName}",
                Test_to_Ph2ACF_Map[testName],
            )
        if not status:
            raise RuntimeError(message)

        return {module_name: (status and sanity, explanation)}, BBanalysis_root_files
    except Exception as err:
        logger.error("An error was thrown while grading: {}".format(repr(err)))
        logger.error(traceback.format_exc())
        return {module_name: (False, repr(err))}, BBanalysis_root_files
