#!/usr/bin/env python
"""
Usage:
    build [options]
    build [options] <extra_args>

Options:
    -h --help           This message
    -c --clean          Entirely removes build folder
    -r --rebuild        Runs a make clean before running a make
    -d --doc            Builds docs
"""
import os
import shutil
from subprocess import check_output


def call(command, shell=True, verbose=None, cwd=None):
    cwd = os.getcwd() if cwd is None else cwd
    if verbose:
        print os.path.join(cwd, command)
    cmd = command.split() if shell else command
    output = check_output(cmd, shell=shell, cwd=cwd)
    if verbose:
        print output
    return output


def make(args="", clean=False, rebuild=False, doc=False, verbose=None):
    working_folder = os.getcwd()
    build_folder = os.path.join(working_folder, 'build')
    if os.path.exists(build_folder):
        if clean:
            shutil.rmtree(build_folder)
    if not os.path.exists(build_folder):
        os.mkdir(build_folder)
    call('cmake ../', cwd=build_folder, verbose=verbose)
    if rebuild:
        call('make clean', cwd=build_folder, verbose=verbose)
    if doc:
        call('make doc', cwd=build_folder, verbose=verbose)
    if args:
        call('make %s' % args, cwd=build_folder, verbose=verbose)
    else:
        call('make', cwd=build_folder, verbose=verbose)



if __name__ == "__main__":
    from docopt import docopt

    args = docopt(__doc__)
    kwargs = dict(
        clean=args.get('--clean', False),
        rebuild=args.get('--rebuild', False),
        doc=args.get('--doc', False),
        args=args.get('<extra_args>', ""),
        verbose=True
        )
    make(**kwargs)
