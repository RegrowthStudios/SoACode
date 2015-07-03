#!/usr/bin/env python
"""
Syncs git submodules

Usage:
    prog [options]

Options:
    -h, --help      This message
"""
from __future__ import print_function
import re
import subprocess
from sys import stderr


def run_cmd(cmd, shell=False):
    """Simplifies running a shell command in Python"""
    shell = True if isinstance(cmd, basestring) else shell
    return subprocess.check_output(cmd, shell=shell)


def get_git_version():
    """Returns git version as a tuple"""
    version = tuple()
    version_pattern = "git version ([0-9]{1,}).([0-9]{1,}).([0-9]{1,})"
    version_string = run_cmd("git --version").strip()
    eng = re.compile(version_pattern)
    match = eng.search(version_string)
    if match:
        version = match.groups()
    return version


def has_error(error_string):
    """Checks git command for error"""


def sync(args):
    """Synchronizes submodules"""
    git_version = get_git_version()
    cmds = []
    if git_version >= (1, 7, 3):
        cmds = [
            "git fetch -a",
            "git pull --recurse-submodules",
            "git submodule update --recursive",
        ]
    elif git_version >= (1, 6, 1):
        cmds = [
            "git fetch -a",
            "git submodule foreach git fetch -a",
            "git submodule foreach git pull",
        ]
    for cmd in cmds:
        output = run_cmd(cmd)
        if has_error(output):
            print(output, file=stderr)
            break


if __name__ == "__main__":
    from docopt import docopt

    args = docopt(__doc__)
    sync(args)
