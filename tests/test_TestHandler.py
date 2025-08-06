import os
import pytest
from unittest.mock import patch, MagicMock
from Gui.python.TestHandler import TestHandler  # Adjust this import according to your project structure.

@pytest.fixture
def setup_test_handler():
    # Create a mock QObject and initialize the TestHandler
    mock_runwindow = MagicMock()
    mock_runwindow.ModuleType = "TFPX CROC Quad"
    mock_master = MagicMock()
    mock_info = "TestInfo"  # Example test info

    # Mock firmware object
    mock_module = MagicMock()
    mock_module.getEnabledChips.return_value = {"0": "15", "1": "14", "2": "13", "3": "12"}
    mock_module.getModuleType.return_value = "TFPX CROC Quad"  # or whatever is valid in ModuleLaneMap

    mock_firmware = MagicMock()
    mock_firmware.getModules.return_value = [mock_module]
    mock_firmware.getModuleData.return_value = {
        "type": "TFPX CROC Quad",
        "version": "TFPX CROC Quad",
        "hdiVersion": "v2"
    }

    firmware_list = [mock_firmware]
    handler = TestHandler(mock_runwindow, mock_master, mock_info, firmware_list)
    return handler

def test_copyMostRecentRootFile_success(setup_test_handler):
    handler = setup_test_handler
    run_number = '123456'
    base_dir = '/mock/base/dir'
    output_dir = '/mock/output/dir'
    test = 'SCurveScan'

    # Mock the files structure
    os.makedirs(f"{base_dir}/test/SCurveScan", exist_ok=True)
    with open(f"{base_dir}/test/SCurveScan/Run{run_number}_SCurve.root", 'w') as f:
        f.write("test data")
    with open(f"{base_dir}/test/SCurveScan/Run{run_number}_SCurve_old.root", 'w') as f:
        f.write("old test data")

    handler.copyMostRecentRootFile(run_number, base_dir, output_dir, test)

    copied_file_path = f"{output_dir}/SCurveScan_0_SCURVE.root"  # Adjust according to your naming convention
    assert os.path.exists(copied_file_path)

def test_copyMostRecentRootFile_no_files(setup_test_handler):
    handler = setup_test_handler
    run_number = '123456'
    base_dir = '/mock/base/dir'
    output_dir = '/mock/output/dir'
    test = 'SCurveScan'

    with pytest.raises(Exception, match="Failed to copy root file to output directory"):
        handler.copyMostRecentRootFile(run_number, base_dir, output_dir, test)

def test_copyMostRecentRootFile_empty_file(setup_test_handler):
    handler = setup_test_handler
    run_number = '123456'
    base_dir = '/mock/base/dir'
    output_dir = '/mock/output/dir'
    test = 'SCurveScan'

    os.makedirs(f"{base_dir}/test/SCurveScan", exist_ok=True)
    with open(f"{base_dir}/test/SCurveScan/Run{run_number}_SCurve.root", 'w') as f:
        f.write("")  # Create an empty file

    with pytest.raises(Exception, match="Failed to copy root file to output directory"):
        handler.copyMostRecentRootFile(run_number, base_dir, output_dir, test)

