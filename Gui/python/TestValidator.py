import os
import ROOT
import traceback
import re

from InnerTrackerTests.TestSequences import Test_to_Ph2ACF_Map, CompositeTests_Modules, OpenBumpTest
from Gui.GUIutils.guiUtils import isCompositeTest
from Gui.python.logging_config import get_logger

logger = get_logger(__name__)

ROOT.gROOT.SetBatch(ROOT.kTRUE)


def _get_open_bump_root_files(output_dir, board_id, hybrid_id):
    root_pattern = re.compile(
        rf"Run(?P<run_number>\d+)_PixelAlive_Board_{re.escape(str(board_id))}"
        rf"_Hybrid_{re.escape(str(hybrid_id))}\.root$"
    )
    matching_files = []

    for file_name in os.listdir(output_dir):
        match = root_pattern.search(file_name)
        if match:
            matching_files.append(
                (int(match.group("run_number")), os.path.join(output_dir, file_name))
            )

    matching_files.sort(key=lambda item: item[0])
    run_numbers = [run_number for run_number, _ in matching_files]

    if len(matching_files) != 3:
        raise ValueError(
            "OpenBumpTest requires exactly 3 ROOT files for "
            f"Board {board_id}, Hybrid {hybrid_id}; found {len(matching_files)} "
            f"with run numbers {run_numbers}."
        )

    expected_run_numbers = list(range(run_numbers[0], run_numbers[0] + 3))
    if run_numbers != expected_run_numbers:
        raise ValueError(
            "OpenBumpTest ROOT files must have sequential run numbers for "
            f"Board {board_id}, Hybrid {hybrid_id}; found {run_numbers}."
        )

    return [path for _, path in matching_files]


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
        if "TFPX" in module_type.split(" ")[0]:
            sensor_type = "planar"
        else:
            sensor_type = "unspecified"

            


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
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir) if module_name in file or file.endswith(".xml") or file.endswith(".json")
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
                type_sensor = sensor_type,
                link_production_db = f"https://www.physics.purdue.edu/cmsfpix/Phase2_Test/w.php?sn={module_name}",
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
                type_sensor = sensor_type,
                link_production_db = f"https://www.physics.purdue.edu/cmsfpix/Phase2_Test/w.php?sn={module_name}",
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
                type_sensor = sensor_type,
                link_production_db = f"https://www.physics.purdue.edu/cmsfpix/Phase2_Test/w.php?sn={module_name}",
            )
            status, message, sanity, explanation = felis.set_result(
                paths_files = relevant_files,
                name_module = module_name,
                name_test = f"{testIndexInSequence:02d}_{testName}",
                type_test = "trimbitscan",
            )


        elif "IREF" in testName:
            relevant_files = [
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir) if module_name in file or file.endswith(".xml") or file.endswith(".json")
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
                type_sensor = sensor_type,
                link_production_db = f"https://www.physics.purdue.edu/cmsfpix/Phase2_Test/w.php?sn={module_name}",
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
                type_sensor = sensor_type,
                link_production_db = f"https://www.physics.purdue.edu/cmsfpix/Phase2_Test/w.php?sn={module_name}",
            )
            status, message, sanity, explanation = felis.set_result(
                paths_files = relevant_files,
                name_module = module_name,
                name_test = f"{testIndexInSequence:02d}_{testName}",
                type_test = "commtest",
                comm_result=comm_result,
            )

        elif testName == "OpenBumpTest":
            # Collect all 3 XML files from the OpenBumpTest subtests
            relevant_files = []
            module_boardID = module_data["boardID"]
            
            # Collect all XML files (one for each subtest: highcharge_xtalk, coupled_xtalk, uncoupled_xtalk)
            for file in os.listdir(outputDir):
                if file.endswith(".xml"):
                    relevant_files.append(os.path.join(outputDir, file))
            
            # Only collect the three PixelAlive ROOT files for this module.
            relevant_files.extend(
                _get_open_bump_root_files(
                    outputDir, module_boardID, module_hybridID
                )
            )
            
            # Collect any .json files
            for file in os.listdir(outputDir):
                if file.endswith(".json"):
                    relevant_files.append(os.path.join(outputDir, file))
            
            logger.info(f"OpenBumpTest collected files: {relevant_files}")
            
            _1, _2 = felis.set_module(
                name_module = module_name,
                subdetector = module_type.split(" ")[0],
                type_module = module_type.split(" ")[2].replace("Quad", "2x2"),
                croc_version = module_version.strip("v"),
                has_sensor = True,
                type_sensor = sensor_type,
                link_production_db = f"https://www.physics.purdue.edu/cmsfpix/Phase2_Test/w.php?sn={module_name}",
            )
            status, message, sanity, explanation = felis.set_result(
                paths_files = relevant_files,
                name_module = module_name,
                name_test = f"{testIndexInSequence:02d}_{testName}",
                type_test = "crosstalk",
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
                outputDir + "/" + os.fsdecode(file) for file in os.listdir(outputDir) if module_name in file or file.endswith(".xml") or file.endswith(".json")
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
                type_sensor = sensor_type,
                link_production_db = f"https://www.physics.purdue.edu/cmsfpix/Phase2_Test/w.php?sn={module_name}",
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
