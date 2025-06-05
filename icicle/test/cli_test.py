#!/bin/env python3

from termcolor import colored
import sys
import subprocess
import difflib
import shlex
from enum import Enum
import re
import click


def get_edits_string(old, new):
    result = ""
    matcher = difflib.SequenceMatcher(a=old, b=new)
    codes = matcher.get_opcodes()
    for code in codes:
        if code[0] == "equal":
            result += colored(old[code[1] : code[2]], "white")
        elif code[0] == "delete":
            result += colored(old[code[1] : code[2]], "red")
        elif code[0] == "insert":
            result += colored(new[code[3] : code[4]], "green")
        elif code[0] == "replace":
            result += colored(old[code[1] : code[2]], "red") + colored(
                new[code[3] : code[4]], "green"
            )
    return matcher.ratio(), result


class ReadMode(Enum):
    NONE = 0
    NAME = 1
    COMMAND = 2
    OUTPUT = 3
    RETVAL = 3


def read_test_file(fname):
    with open(fname, "r") as fp:
        name = ""
        command = []
        expected_output = []
        retval = 0
        mode = ReadMode.NONE
        for line in fp:
            if not line:
                continue
            elif line.startswith("##"):
                if command:
                    yield name, command, expected_output, retval
                name, command, expected_output, retval = "", [], [], 0
                mode = ReadMode.NAME
            elif line.startswith("# COMMAND"):
                mode = ReadMode.COMMAND
            elif line.startswith("# OUTPUT"):
                mode = ReadMode.OUTPUT
            elif line.startswith("# RETURNS "):
                retval = int(line.replace("# RETURNS ", ""))
            elif mode == ReadMode.NAME:
                name = line
                mode = ReadMode.NONE
            elif mode == ReadMode.COMMAND:
                command.append(line.strip())
            elif mode == ReadMode.OUTPUT:
                expected_output.append(line.strip())
        if command:
            yield name, command, expected_output, retval


def remove_times(string):
    string = re.sub(
        r"(Mon|Tue|Wed|Thu|Fri|Sat|Sun)\s(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+[0-9]+\s(\d{2}:\d{2}:\d{2})\s(\d{4})",
        "{DATETIME}",
        string,
    )
    string = re.sub(r"[0-9]+/[0-9]+/[0-9]+\s*", "{DATE}", string)
    string = re.sub(r"[0-9]+:[0-9]+:[0-9]+\.?[0-9]*\s*", "{TIME}", string)
    string = re.sub(r"(?<=[^0-9.])[0-9]{10}\.?[0-9]{0,10}\s*", "{TIMESTAMP}", string)
    return string


def run_test(name, command, expected_output, retval=0):
    process = build_test(name, command)
    return eval_test(name, process, expected_output, retval)


def build_test(name, command):
    args = shlex.split(command)
    return subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def eval_test(name, process, expected_output, retval=0):
    process.wait()
    result = process.communicate()[0]

    ret = []
    ret.append(f"TEST: {name} -> ")

    output = "\n".join(
        s.strip()
        for s in result.decode().strip().split("\n")
        if not s.startswith("WARNING:icicle")
        or s.startswith("ERROR:icicle")
        or s.startswith("FATAL:icicle")
    )

    output = remove_times(output)

    ratio, diff = get_edits_string(expected_output, output)

    if ratio < 1 or process.returncode != retval:
        ret[0] += colored("FAILED", "red")
        if process.returncode != retval:
            ret.append(
                colored(f"; returned: {process.returncode}; expected: {retval}", "red")
            )
        ret.append(diff)
        return False, ret
    else:
        ret[0] += colored("PASSED", "green")
        return True, ret


def generate_test_file(fin, fout):
    for line in fin:
        if not line.strip() or line.strip().startswith("#"):
            continue
        name, command = line.split("|")
        name = name.strip()
        command = command.strip()
        print(f"=========== Now generating: {name} =============")
        fout.write("##\n")
        fout.write(f"{name}\n")
        fout.write("# COMMAND\n")
        print("# COMMAND")
        fout.write(f"{command.strip()}\n")
        print(f"{command.strip()}")
        args = shlex.split(command)
        result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        output = remove_times(
            "\n".join(
                s.strip()
                for s in result.stdout.decode().strip().split("\n")
                if not s.startswith("WARNING:icicle")
                or s.startswith("ERROR:icicle")
                or s.startswith("FATAL:icicle")
            )
        )
        fout.write("# OUTPUT\n")
        print("# OUTPUT")
        fout.write(f"{output}\n")
        print(f"{output}")
        fout.write(f"# RETURNS {result.returncode}\n")
        print(f"# RETURNS {result.returncode}\n")


@click.command("cli_test")
@click.argument("test_file", type=click.Path(), metavar="TEST_FILE")
@click.option(
    "-g", "--generate", type=click.File("r"), default=None, metavar="GENERATOR_FILE"
)
@click.option(
    "-j",
    "--parallel",
    is_flag=True,
)
def main(test_file, generate=None, parallel=False):
    if generate is not None:
        with open(test_file, "w") as fout:
            return generate_test_file(generate, fout)

    fails = 0
    output = []

    print(f'[{test_file}] "{sys.argv[0]} {test_file}"')

    if parallel:
        tests = [
            (name.strip(), " ".join(command), "\n".join(expected_output), retval)
            for name, command, expected_output, retval in read_test_file(test_file)
        ]
        print(f"[{test_file}] starting {len(tests)} parallel test processes...")
        processes = []
        for name, command, _, __ in tests:
            processes.append(build_test(name, command))

        for process, (name, _, expected_output, retval) in zip(processes, tests):
            success_, output_ = eval_test(name, process, expected_output, retval)
            fails += not success_
            output += output_

    else:
        print(f"[{test_file}] running tests sequentially...")
        for name, command, expected_output, retval in read_test_file(test_file):
            process = build_test
            success_, output_ = run_test(
                name.strip(), " ".join(command), "\n".join(expected_output), retval
            )
            fails += not success_
            # print('\n'.join(output_))
            output += output_

    print(f"=========== {test_file} =============")
    print("\n".join(output))

    sys.exit(fails)


if __name__ == "__main__":
    main()
