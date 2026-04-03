import matplotlib.pyplot as plt
import csv

def generate_plot(log_file, plot_file):
    times = []
    temps = []
    setpoints = []

    with open(log_file, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            times.append(float(row["time_s"]))
            temps.append(float(row["temperature"]))
            setpoints.append(float(row["setpoint"]))

    plt.figure()
    plt.plot(times, temps, label="Temperature")
    plt.plot(times, setpoints, linestyle="--", label="Setpoint")

    plt.xlabel("Time (s)")
    plt.ylabel("Temperature (C)")
    plt.title("Thermal Chamber Profile")
    plt.legend()

    plt.savefig(plot_file)
    plt.close()