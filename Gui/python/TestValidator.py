import os
import ROOT

from InnerTrackerTests.TestSequences import Test_to_Ph2ACF_Map

ROOT.gROOT.SetBatch(ROOT.kTRUE)


def ResultGrader(
    felis,
    outputDir,
    testName,
    testIndexInSequence,
    runNumber,
    module_data,
    BBanalysis_root_files,
):
    try:
        module_name = module_data["module"].getModuleName()
        module_type = module_data["module"].getModuleType()
        module_version = module_data["module"].getModuleVersion()
        if "SLDOScan" in testName or "CommunicationTest" in testName:
            explanation = (
                "No grading currently available for SLDOScan or CommunicationTest."
            )
            return {module_name: (True, explanation)}

        root_file_name = testName.split("_")[0]
        if "SCurveScan" in root_file_name:
            root_file_name = "SCurve"
        elif "GainScan" in root_file_name:
            root_file_name = "Gain"
        elif "Threshold" in root_file_name:
            root_file_name = root_file_name.replace("Threshold", "Thr")
        if "IVCurve" in testName:
            root_file_name = testName.split("_")[0] + "_" + module_name

            # NOTE: This may be useful
            # module_canvas_path = (
            #     "Detector/Board_{boardID}/OpticalGroup_{ogID}/Hybrid_{hybridID}".format(
            #         boardID=module_data["boardID"],
            #         ogID=module_data["ogID"],
            #         hybridID=module_data["hybridID"],
            #     )
            # )

            ROOT_file_path = "{0}/Result_{1}.root".format(outputDir, root_file_name)

            if root_file_name in (
                "PixelAlive_highcharge_xtalk",
                "PixelAlive_coupled_xtalk",
                "PixelAlive_uncoupled_xtalk",
            ):
                BBanalysis_root_files.append(ROOT_file_path)

            relevant_files = [
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir)
            ]
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
                "ivcurve",
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
                BBanalysis_root_files.append(ROOT_file_path)

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

        return {module_name: (True, explanation)}, BBanalysis_root_files
    except Exception as err:
        # logger.error("An error was thrown while grading: {}".format(repr(err)))
        return {module_name: (False, repr(err))}, BBanalysis_root_files
