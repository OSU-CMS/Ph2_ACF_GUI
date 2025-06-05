#!/usr/bin/env python3

# Adapted from https://github.com/code-freak/codeclimate-cpplint.git

import subprocess
import json
import re
import argparse
import linecache
import hashlib


ECLIPSE_REGEX = re.compile(r'^(.*):([0-9]+):\s+(.*?)\s+\[([^\]]+)\]\s+\[([0-9]+)\]')

parser = argparse.ArgumentParser()
parser.add_argument('--files', nargs='+')
args = parser.parse_args()


def get_issue(line):
    matches = ECLIPSE_REGEX.match(line)
    print(line)
    if matches is None:
        return None
    (fname, line, message, check, level) = matches.group(1, 2, 3, 4, 5)
    codeline = linecache.getline(fname, int(line))
    issue = {
        "fingerprint": f"{hashlib.md5(str(check+message+fname+codeline).encode()).hexdigest()}",
        "type": "issue",
        "severity": "minor",
        "check_name": check,
        "description": message,
        "categories": ["Style"],
        "location": {
            "path": fname,
            "lines": {
                "begin": int(line),
                "end": int(line)
            }
        }
    }
    return issue


def run_cpplint(files):
    cmd = ["cpplint", "--linelength=120", "--filter=-legal", "--recursive"]
    cmd.extend(files)
    try:
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
        print(output)
    except subprocess.CalledProcessError as e:
        output = e.output

    if len(output):
        output = output.decode('utf-8')
        issues = []
        for line in output.splitlines():
            result = get_issue(line)
            if result:
                issues.append(result)

        print(">>> Writing JSON report to cpplint-report.json")
        with open('cpplint-report.json', 'w') as f:
            json.dump(issues, f, indent=4)

def main():
    run_cpplint(args.files)
    print("\0")
    pass

if __name__ == "__main__":
    main()
