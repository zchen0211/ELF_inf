import sys
import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('key', type=str, default=None)
parser.add_argument('idx', type=str, default=None, help="format: 13:0")

args = parser.parse_args()

data = json.load(open(args.key + ".json"))
indices = [ int(v) for v in args.idx.split(":") ]

print(data["content"][indices[0]]["swap"])
print(data['content'][indices[0]]["games"][indices[1]])
