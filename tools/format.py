#!/usr/bin/env python

import argparse, sys, os, re


def freeft_dir():
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")


sys.path.insert(0, os.path.join(freeft_dir(), "libfwk", "tools"))
from format import CodeFormatter, find_files

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog=__file__,
        description="Tool for code formatting and format verification",
    )
    parser.add_argument("-c", "--check", action="store_true")
    args = parser.parse_args()

    formatter = CodeFormatter()
    os.chdir(freeft_dir())
    files = find_files(["src"], re.compile(".*[.](h|cpp)$"))
    formatter.format_cpp(files, args.check)
