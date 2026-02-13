import ROOT
import os
from ctypes import c_double
from Gui.python.logging_config import get_logger
import traceback

logger = get_logger(__name__)

def extract_data_from_root(root_file_path, chip, measurement_type):
    """
    Extracts data from a ROOT file for a specific chip and measurement type.

    Args:
        root_file_path (str): Path to the ROOT file.
        chip (int): Chip number to extract data for.
        measurement_type (str): Measurement type (e.g., 'VDDD', 'VDDA').

    Returns:
        dict: A dictionary containing the extracted data.
    """
    if not os.path.exists(root_file_path):
        logger.error(f"ROOT file not found: {root_file_path}")
        return None

    try:

        # Open the ROOT file
        root_file = ROOT.TFile(root_file_path, "READ")
        if root_file.IsZombie():
            logger.error(f"Failed to open ROOT file: {root_file_path}")
            return None

        # Construct the path to the desired data
        detector_path = f"Detector/Board_0/OpticalGroup_0/Hybrid_0/Chip_{chip}/D_B(0)_O(0)_H(0)_DQM_{measurement_type}_Chip({chip});3"

        # Retrieve the TGraph object
        tgraph = root_file.Get(detector_path)
        if not tgraph:
            logger.error(f"TGraph not found at path: {detector_path}")
            root_file.Close()
            return None

        # Extract data points from the TGraph
        data = {
            "y": []   # e.g., measurement values
        }
        for i in range(tgraph.GetN()):
            x, y = c_double(), c_double()  # Use ctypes.c_double instead of ROOT.Double
            tgraph.GetPoint(i, x, y)
            data["x"].append(float(x.value))  # Access the value of c_double
            data["y"].append(float(y.value))  # Access the value of c_double

        root_file.Close()
        return data

    except Exception:
        logger.error(traceback.format_exc())
        return None

# Example usage
if __name__ == "__main__":
    root_file = "Ph2_ACF/test/Results/Run000277_MonitorDQM.root"
    chip = 12
    measurement_type = "VDDA"
    data = extract_data_from_root(root_file, chip, measurement_type)
    if data:
        logger.info(f"Extracted data for chip {chip}, measurement {measurement_type}: {data}")
