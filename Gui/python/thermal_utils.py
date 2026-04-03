import os
from datetime import datetime

def get_thermal_test_dir():
    base_dir = os.environ.get("DATA_dir", os.path.join(os.getcwd(), "data", "TestResults"))
    thermal_dir = os.path.join(base_dir, "ThermalTest")
    os.makedirs(thermal_dir, exist_ok=True)
    return thermal_dir

def generate_thermal_file_paths():
    output_dir = get_thermal_test_dir()
    timestamp = datetime.now().strftime("%Y_%m_%d-%H_%M_%S")

    log_file = os.path.join(output_dir, f"thermal_log_{timestamp}.csv")
    plot_file = os.path.join(output_dir, f"thermal_plot_{timestamp}.png")

    return log_file, plot_file