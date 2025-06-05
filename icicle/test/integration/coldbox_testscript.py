from icicle.psi_coldbox import PSIColdbox

coldbox = PSIColdbox(
    resource="TCPIP::cornetto.dhcp-int.phys.ethz.ch::1883::SOCKET",
    username="",
    password="",
    init=False,
)
coldbox.__enter__()

# Which functionalities are supposed to be tested
TEST_SETTINGS = {
    "TEST_QUERY": True,  # Run every possible QUERY function
    "TEST_READ": True,  # Run every possible READ function
    "TEST_SET": True,  # Run every possible SET function except POWER
    "TEST_VPROBE": True,  # Run a test on the TEPX needle card
    "TEST_CHANNEL": True,  # Run a test for each individual channel if possible
    "TEST_FUNCTION_CALL": True,  # Run a test for all callable functions
}

TEST_TOTAL = 0
TEST_PASSED = 0
TEST_FAILED = 0

if TEST_SETTINGS["TEST_QUERY"]:
    print("\033[94m### Test of QUERY command ### \033[0m")
    for key in coldbox.SETTINGS:
        print(f"###{key}")
        if "QUERY" in type(coldbox).SETTINGS[key]:
            print("allows to query:")
            try:
                print(f"--> Value: {coldbox.query(key, 0)}")
                TEST_PASSED += 1
            except Exception as e:
                print(f"--> Value: {e}")
                TEST_FAILED += 1

            if "channel" in coldbox.SETTINGS[key] and TEST_SETTINGS["TEST_CHANNEL"]:
                print("allows to query by channel:")
                for i in range(1, 9):
                    try:
                        print(f"--> Channel: {i}, Value: {coldbox.query(key, i)}")
                        TEST_PASSED += 1
                    except Exception as e:
                        print(f"--> Channel: {i}, Value: {e}")
                        TEST_FAILED += 1

if TEST_SETTINGS["TEST_READ"]:
    print("\033[94m ### Test of READ command ### \033[0m")
    for key in coldbox.SETTINGS:
        print(f"###{key}")
        if (
            "QUERY" in type(coldbox).SETTINGS[key]
            or "READ" in type(coldbox).SETTINGS[key]
        ):
            print("allows to read:")
            try:
                print(f"--> Value: {coldbox.read(key, 0)}")
                TEST_PASSED += 1

            except Exception as e:
                print(f"--> Value: {e}")
                TEST_FAILED += 1

if TEST_SETTINGS["TEST_VPROBE"]:
    print("\033[94m### Test of VOLTAGE PROBE ### \033[0m")
    key = "VOLTAGE_PROBE"
    print(f"###{key}")
    print("allows to query by channel:")
    for i in range(1, 9):
        try:
            print(f"--> Channel: {i}, Value: {coldbox.query(key, i)}")
            TEST_PASSED += 1
        except Exception as e:
            print(f"--> Channel: {i}, Value: {e}")
            TEST_FAILED += 1

if TEST_SETTINGS["TEST_SET"]:
    print("\033[94m ### Test of SET command ### \033[0m")
    for key in coldbox.SETTINGS:
        print(f"###{key}")
        settings_ = type(coldbox).SETTINGS[key]

        if "SET" in settings_:
            if key in ["FLUSH", "RINSE", "THROTTLE_N2", "MODE", "N2THROTTLE"]:
                print("allows to set '0' and '1':")
                try:
                    print(f"--> Set '0', Value: {coldbox.set(key, 1)}")
                    print(f"--> Set '1', Value: {coldbox.set(key, 0)}")
                    TEST_PASSED += 1
                except Exception as e:
                    print(f"-->  Value: {e}")
                    TEST_FAILED += 1

                if key == "MODE" and TEST_SETTINGS["TEST_CHANNEL"]:
                    print("allows to set '0' and '1' by channel:")
                    for i in range(1, 9):
                        try:
                            print(
                                f"--> Channel: {i}, Set '0', Value: "
                                f"{coldbox.set(key, 0, channel=i)}"
                            )
                            print(
                                f"--> Channel: {i}, Set '1', Value: "
                                f"{coldbox.set(key, 1, channel=i)}"
                            )
                            TEST_PASSED += 1
                        except Exception as e:
                            print(f"-->  Value: {e}")
                            TEST_FAILED += 1

            elif key in ["FLUSH_TOGGLE", "RINSE_TOGGLE"]:
                print("allows to toggle:")
                try:
                    print(f"--> Toggle, Value: {coldbox.set(key, None)}")
                    TEST_PASSED += 1
                except Exception as e:
                    print(f"-->  Value: {e}")
                    TEST_FAILED += 1

            elif key in ["POWER"]:
                pass

            elif key in [
                "CONTROL_VOLTAGE",
                "PID_KD",
                "PID_KI",
                "PID_KP",
                "PID_MIN",
                "PID_MAX",
                "TEMPERATURE_SET",
                "VOLTAGE",
            ]:
                print("allows to set float:")
                default = settings_["default"]

                try:
                    print(f"--> Set '0.0', Value: {coldbox.set(key, 0.0)}")
                    print(f"--> Set 'default', Value: {coldbox.set(key, default)}")
                    TEST_PASSED += 1
                except Exception as e:
                    print(f"-->  Value: {e}")
                    TEST_FAILED += 1

                if "channel" in settings_ and TEST_SETTINGS["TEST_CHANNEL"]:
                    print("allows to set float by channel:")
                    for i in range(1, 9):
                        try:
                            print(
                                f"--> Channel: {i}, Set '0.0', Value: "
                                f"{coldbox.set(key, 0.0, channel=i)}"
                            )
                            print(
                                f"--> Channel: {i}, Set 'default', Value: "
                                f"{coldbox.set(key, default, channel=i)}"
                            )
                            TEST_PASSED += 1
                        except Exception as e:
                            print(f"-->  Value: {e}")
                            TEST_FAILED += 1

            else:
                print(
                    f"The behaviour for setting {key} has "
                    f"not been implemented for testing yet."
                )

if TEST_SETTINGS["TEST_FUNCTION_CALL"]:
    print("\033[94m ### Test of FUNCTION CALLS ### \033[0m")
    print("### connected()")
    print(f"-->  Value: {coldbox.connected()}")

    print("### reset()")
    print(f"-->  Value: {coldbox.reset()}")

    print("### interlocked()")
    print(f"-->  Value: {coldbox.interlocked()}")

    print("### on() and off()")
    print("-->  Skipped, manual testing required.")

    print("### set()")
    print("-->  Skipped, to test use 'TEST_SET'.")

    print("### query()")
    print("-->  Skipped, to test use 'TEST_QUERY'.")

    print("### read()")
    print("-->  Skipped, to test use 'TEST_READ'.")

    print("### execute()")
    print("-->  Skipped, development only command.")

    print("### reboot()")
    print(f"-->  Value: {coldbox.reboot(channel=0)}")

    print("### save_variables_on_tec()")
    print(f"-->  Value: {coldbox.save_variables_on_tec(channel=0)}")

    print("### load_variables_from_tec()")
    print(f"-->  Value: {coldbox.load_variables_from_tec(channel=0)}")

    print("### load_variables_from_icicle()")
    print(f"-->  Value: {coldbox.load_variables_from_icicle(channel=0)}")

coldbox.__exit__()
print("\033[94m ### DONE ### \033[0m")
print(f"\033[92m # {TEST_PASSED} PASSED  \033[0m")
print(f"\033[91m # {TEST_FAILED} FAILED  \033[0m")
print(f"{100 * TEST_PASSED / (TEST_PASSED + TEST_FAILED)} % successful")
