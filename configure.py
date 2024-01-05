#!/usr/bin/env python3
import subprocess
import argparse
from sys import platform

gn = ""
if platform == "linux":
    gn = "gn/bin/linux/gn"
elif platform == "win32":
    gn = "gn/bin/win/gn.exe"

parser = argparse.ArgumentParser("configure.py", description="generates build files")
parser.add_argument("-o", "--out", default="build/", help="output destination directory")
args = parser.parse_args();

subprocess.run([gn, "gen", args.out, "--add-export-compile-commands=//*"])
