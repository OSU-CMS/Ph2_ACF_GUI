"""
@author Simon Koch <simon.florian.koch@cern.ch>
@date Oct 2024

Example/use case: automatic stage movement for testbeam experiment,
using XIMC-based xy-stage, and Timepix4 slow control readout.

This also used the `tpx4sc` interface to talk to a MaltaMultiDAQ
instance and a Malta TLU instance, using the emulated slow control
channels implemented for each tool.
"""

from sys import argv, exit
from time import sleep
import subprocess
from datetime import datetime

from icicle.tpx4sc import TPX4SC
from icicle.ximc_instrument import XimcInstrument

TPX4_RESOURCE = "TCPIP::192.168.200.102::51000::SOCKET"
MMDAQ_RESOURCE = "TCPIP::localhost::51001::SOCKET"
TLU_RESOURCE = "TCPIP::localhost::51002::SOCKET"
VERT_RESOURCE = "xi-com:///dev/ximc/00007E24"
HORI_RESOURCE = "xi-com:///dev/ximc/00007E61"


class RunData:
    def __init__(self, run_number, ntrigs, vert, hori, start_time, end_time, comment):
        self.run_number = run_number
        self.start_time = start_time
        self.end_time = end_time
        self.ntrigs = ntrigs
        self.vert = vert
        self.hori = hori
        self.comment = comment


class RetakeRun(RunData):
    pass


class GoodRun(RunData):
    pass


def alert():
    for _ in range(20):
        print("\a", end="\r")
        sleep(0.1)


def init_clients():
    tpx = TPX4SC(resource=TPX4_RESOURCE)
    mmdaq = TPX4SC(resource=MMDAQ_RESOURCE)
    tlu = TPX4SC(resource=TLU_RESOURCE)
    return tpx, mmdaq, tlu


def init_stages():
    stage_vertical = XimcInstrument(resource=VERT_RESOURCE)
    stage_horizontal = XimcInstrument(resource=HORI_RESOURCE)
    return stage_vertical, stage_horizontal


def main():
    if len(argv) != 4:
        print(f"Usage: {argv[0]} POSITION_FILE START_RUN_NUMBER NUM_TRIGGERS")
        return -1
    input_file = argv[1]
    position_list = []
    with open(input_file, "r") as fp:
        for i, line in enumerate(fp):
            if not line.strip() or line.strip()[0] == "#":
                continue
            print(f"processing line... {line.strip()}")
            toks = line.strip().split()
            print(toks)
            if len(toks) < 2:
                alert()
                raise RuntimeError(
                    f"Bad input file format - line {i} "
                    f"does not contain two positions "
                    f"(horizontal, vertical, [comment])"
                )
            position_list.append(
                [
                    float(toks[0]),
                    float(toks[1]),
                    " ".join(toks[2:]) if len(toks) > 2 else "",
                ]
            )

    tpx, mmdaq, tlu = init_clients()
    stage_vertical, stage_horizontal = init_stages()

    run_number = int(argv[2])
    if "m" in argv[3].lower():
        ntrig = int(argv[3].lower().replace("m", "")) * 1e6
    elif "k" in argv[3].lower():
        ntrig = int(argv[3].lower().replace("k", "")) * 1e3
    else:
        ntrig = int(argv[3])

    for position in position_list:
        print(f"== START: Now will start position {position} ==")
        state = None
        attempt = 1
        while type(state) is not GoodRun:
            print(f"Position: {position} - attempt {attempt}")
            if check_run_exists(run_number):
                print(
                    f"== ERROR: Run number {run_number} already exists "
                    "- refusing to run and overwrite!"
                )
                return
            # SIMON: UNCOMMENT THIS TO PAUSE BETWEEN RUNS
            alert()
            # input('Are we ok to continue? (press ENTER)...')
            state = do_run(
                run_number,
                position,
                ntrig,
                tpx,
                mmdaq,
                tlu,
                stage_vertical,
                stage_horizontal,
            )
            log_run(
                "GOOD" if type(state) is GoodRun else "BAD",
                state.run_number,
                state.hori,
                state.vert,
                state.ntrigs,
                state.start_time,
                state.end_time,
                state.comment,
            )
            run_number += 1
            attempt += 1
        print(f"== END: Finished position {position} ==")


def log_run(*args):
    with open("run_log.txt", "a") as fp:
        fp.write("\t".join([str(a) for a in args]))
        fp.write("\n")


def do_run(
    run_number, position, ntrig, tpx, mmdaq, tlu, stage_vertical, stage_horizontal
):
    try:
        hori, vert, comment = position

        start_time = datetime.now()
        # check if configured
        for daq in (tpx, mmdaq, tlu):
            configured = False
            while not configured:
                try:
                    with daq:
                        if daq.status() == "UNCONFIGURED":
                            if not daq.set("CONFIGURE"):
                                raise RuntimeError(f"{daq=} could not configure")
                            if daq == tpx:
                                # try three times to set threshold
                                for _ in range(3):
                                    if not daq.set("THRESHOLD", 2000):
                                        raise RuntimeError(
                                            "tpx could not set threshold"
                                        )
                    configured = True
                except Exception as e:
                    print(f"==ERROR== Error encountered in CONFIGURE: {e}")
                    print(
                        "Please restart the offending DAQ software (see "
                        "shifter assistant) and press enter:"
                    )
                    alert()
                    input("Press enter to continue...")
                    continue

        # Move stage
        with stage_vertical:
            stage_vertical.move_user(vert, wait_and_readback=False)
        with stage_horizontal:
            stage_horizontal.move_user(hori, wait_and_readback=False)

        with stage_vertical:
            nvert = stage_vertical.get_position_user()["position"]
            while abs(nvert - vert) > 0.1:
                nvert = stage_vertical.get_position_user()["position"]
                sleep(0.2)
        with stage_horizontal:
            nhori = stage_horizontal.get_position_user()["position"]
            while abs(nhori - hori) > 0.1:
                nhori = stage_horizontal.get_position_user()["position"]
                sleep(0.2)

        print(f"== Now at position {nhori}, {nvert} for run {run_number} ==")

        # Configure Timepix before every run to avoid Beamclock issue
        # with tpx:
        #    tpx.set('CONFIGURE')
        #    tpx.set('THRESHOLD', 2000)
        #    tpx.set('THRESHOLD', 2000)
        #    tpx.set('THRESHOLD', 2000)

        # Start taking data
        for daq in (tpx, mmdaq, tlu):
            running = False
            while not running:
                try:
                    with daq:
                        if daq.status() != "CONFIGURED":
                            raise RuntimeError(
                                f"{daq=} in wrong state "
                                f"{daq.status()} (should "
                                f"be CONFIGURED) to start "
                                f"run- please fix manually"
                            )
                        if not daq.set("START_RUN", run_number):
                            raise RuntimeError(f"{daq=} could not start run")
                    running = True
                except Exception as e:
                    print(f"==ERROR== Error encountered in START_RUN: {e}")
                    print(
                        "Please restart the offending DAQ software (see "
                        "shifter assistant) and press enter:"
                    )
                    alert()
                    input("Press enter to continue...")
                    continue
            sleep(0.5)
        # wait for triggers:
        ntrig_act = 0
        progress = 0
        while ntrig_act < ntrig:
            try:
                with tlu:
                    ntrig_act = tlu.measure()[0]
                if ntrig_act > progress:
                    print(f"...{ntrig_act/ntrig * 100:.1f}% done...")
                    progress += ntrig / 20
                sleep(3)
            except Exception:
                pass

        # end run
        for daq in (tlu, tpx, mmdaq):
            running = True
            while running:
                try:
                    with daq:
                        if daq.status() != "RUNNING":
                            raise RuntimeError(
                                f"{daq=} in wrong state "
                                f"{daq.status()} (should "
                                f"be RUNNING) after run - "
                                f"please retake position"
                            )
                            return RetakeRun(
                                run_number,
                                ntrig_act,
                                nvert,
                                nhori,
                                start_time,
                                datetime.now(),
                                "ERROR IN STOP_RUN | " + comment,
                            )
                        if not daq.set("STOP_RUN"):
                            raise RuntimeError(f"{daq=} could not stop run")
                    running = False
                except Exception as e:
                    print(f"==ERROR== Error encountered in STOP_RUN: {e}")
                    print(
                        "Please manually fix the offending DAQ software (see "
                        "shifter assistant) and enter R to restart run, T to "
                        "retry stop command, or C to continue to next position:"
                    )
                    alert()
                    a = input(
                        "R to redo this run or T to try stop command again or C "
                        "to continue with manual stop of this DAQ system"
                    )
                    if "R" == a.upper()[0]:
                        return RetakeRun(
                            run_number,
                            ntrig_act,
                            nvert,
                            nhori,
                            start_time,
                            datetime.now(),
                            "ERROR IN STOP_RUN - REJECTED | " + comment,
                        )
                    if "C" == a.upper()[0]:
                        running = False
                        continue
                    else:
                        continue
            sleep(0.5)
        # check output files
        if check_for_output_files(run_number):
            return GoodRun(
                run_number,
                ntrig_act,
                nvert,
                nhori,
                start_time,
                datetime.now(),
                "GOOD | " + comment,
            )
        else:
            return RetakeRun(
                run_number,
                ntrig_act,
                nvert,
                nhori,
                start_time,
                datetime.now(),
                "ERROR IN OUTPUT FILES | " + comment,
            )

    except KeyboardInterrupt as ke:
        print("== INFO: Caught SIGINT: aborting this run and retrying ==")
        print(f" -> {ke}")
        # end run
        for daq in (tlu, tpx, mmdaq):
            running = True
            while running:
                try:
                    with daq:
                        if daq.status() != "RUNNING":
                            raise RuntimeError(
                                f"{daq=} in wrong state "
                                f"{daq.status()} (should "
                                f"be RUNNING) after run - "
                                f"please retake position"
                            )
                            return RetakeRun(
                                run_number,
                                ntrig_act,
                                nvert,
                                nhori,
                                start_time,
                                datetime.now(),
                                "ERROR IN STOP_RUN | " + comment,
                            )
                        try:
                            daq.set("STOP_RUN")
                        except Exception:
                            pass
                        running = False
                except Exception as e:
                    print(f"==ERROR== Error encountered in STOP_RUN: {e}")
                    print(
                        "Please manually fix the offending DAQ software (see "
                        "shifter assistant) and enter R to restart run, T to retry "
                        "stop command, or C to continue to next position:"
                    )
            sleep(0.5)
            return RetakeRun(
                run_number,
                ntrig_act,
                nvert,
                nhori,
                start_time,
                datetime.now(),
                "ABORTED BY SHIFTER | " + comment,
            )


def check_for_output_files(run_number):
    try:
        # MALTA planes
        command = f"ls -l /home/sbmuser/data/*{run_number}*.root.root | wc -l"
        n = subprocess.check_output(command, shell=True, text=True)
        if int(n) != 4:
            print(
                "=== ERROR: Missing MALTA output file! Check run {run_number} "
                "output ==="
            )
            command = f"ls -l /home/sbmuser/data/*{run_number}*.root.root"
            print(subprocess.check_output(command, shell=True, text=True))
            return False
        # Timepix
        command = f"ssh timepix-daq -- ls -l /data/timepix/run{run_number}/* | wc -l"
        n = subprocess.check_output(command, shell=True, text=True)
        if int(n) != 2:
            print(
                "=== ERROR: Missing TimePix4 output file! Check run {run_number} "
                "output ==="
            )
            command = f"ssh timepix-daq -- ls -l /data/timepix/run{run_number}/"
            print(subprocess.check_output(command, shell=True, text=True))
            return False
    except subprocess.CalledProcessError as e:
        print(f"Error in subprocess: {e}")
        print("== ERROR: files assumed not ok ==")
        return False
    return True


def check_run_exists(run_number):
    # MALTA planes
    commands = [
        f"test -e ~/data/run_{run_number}_0.root.root && echo 'YES' || echo 'NO'",
        f"test -e ~/data/run_{run_number}_1.root.root && echo 'YES' || echo 'NO'",
        f"test -e ~/data/run_{run_number}_2.root.root && echo 'YES' || echo 'NO'",
        f"test -e ~/data/run_{run_number}_3.root.root && echo 'YES' || echo 'NO'",
    ]
    for command in commands:
        try:
            o = subprocess.check_output(command, shell=True, text=True)
            if "YES" in o:
                return True
        except subprocess.CalledProcessError:
            pass

    # Timepix
    command = (
        f"ssh timepix-daq -- test -e /data/timepix/run{run_number}/ "
        "&& echo 'YES' || echo 'NO'"
    )
    try:
        o = subprocess.check_output(command, shell=True, text=True)
        if "YES" in o:
            return True
    except subprocess.CalledProcessError:
        pass
    return False


if __name__ == "__main__":
    exit(main())
