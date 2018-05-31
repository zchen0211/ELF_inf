import os
import glob
import root_path
import argparse
from datetime import datetime

parser = argparse.ArgumentParser()
parser.add_argument("--first_n", type=int, default=10)

args = parser.parse_args()

filenames = sorted(glob.glob(os.path.join(root_path.get(), "*")), key=os.path.getmtime, reverse=True)

for filename in filenames[:args.first_n]:
    if not os.path.isdir(filename):
        continue
    str_time = datetime.fromtimestamp(os.path.getmtime(filename)).strftime("%Y%m%d")
    filename = os.path.basename(filename)
    if filename in ["140", "194", "216", "199"]:
        continue
    print("%s %s" % (str_time, filename))
