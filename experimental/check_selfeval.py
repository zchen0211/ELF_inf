import os
from collections import defaultdict
import sys
import re
import json
import multiprocessing

perf_matcher = re.compile(r"B/W: (\d+)/(\d+)")
batchsize_matcher = re.compile(r"--batchsize (\d+)")
num_thread_matcher = re.compile(r"--mcts_threads (\d+)")
load0_matcher = re.compile(r"--load0 ([^ ]+)")
load1_matcher = re.compile(r"--load1 ([^ ]+)")
comment_matcher = re.compile(r"--comment ([^ ]+)")

name_matcher = re.compile(r"save-(\d+)")

timer_matcher = re.compile(r"MCTS: (\d+)ms")
stop_matcher = re.compile(r"Prepare to stop")

root = "/mnt/vol/gfsai-flash-east/ai-group/users/yuandong/rts"

def parse_log(log):
    sum_time = 0
    count = 0
    n_win = 0
    n = 0

    try:
        for l in open(log):
            if stop_matcher.search(l):
                break

            m = perf_matcher.search(l)
            if m:
                n_win = int(m.group(1))
                n = int(m.group(1)) + int(m.group(2))

            m = timer_matcher.search(l)
            if m:
                sum_time += int(m.group(1))
                count += 1
    except:
        pass

    return dict(time=sum_time / count if count > 0 else None, res=(n_win, n))

def find_key(cmd):
    swap = False

    m = comment_matcher.search(cmd)
    if m:
        key = m.group(1)
    else:
        batchsize = int(batchsize_matcher.search(cmd).group(1))
        num_threads = int(num_thread_matcher.search(cmd).group(1))
        load0 = load0_matcher.search(cmd).group(1)
        load1 = load1_matcher.search(cmd).group(1)

        black = int(name_matcher.search(os.path.basename(load0)).group(1))
        white = int(name_matcher.search(os.path.basename(load1)).group(1))

        black_folder = os.path.basename(os.path.dirname(load0))
        white_folder = os.path.basename(os.path.dirname(load1))

        '''
        if black_folder > white_folder:
            black, white = white, black
            black_folder, white_folder = white_folder, black_folder
            swap = True
        else:
            swap = False
        '''
        key = "b=%d,w=%d,bf=%s,wf=%s,batchsize=%d,threads=%d " % (black, white, black_folder, white_folder, batchsize, num_threads)

    return key, swap

def check_one_job(job):
    if "command" not in job:
        return None

    cmd = job["command"]
    key, swap = find_key(cmd)

    log = os.path.join(root, str(job["id"]), "output%d-0.log" % job["job_idx"])

    result = dict(key=key, log=log, res=(0, 0), time=None, swap=swap)

    if os.path.exists(log):
        result.update(parse_log(log))
        if swap:
            result["res"] = (result["res"][1] - result["res"][0], result["res"][1])
        #if result["res"][1] > 0:
        #    print("%s %d %d" % (log, result["res"][0], result["res"][1]))

    return result

stats = defaultdict(lambda: dict(time=[0, 0], wr=[0, 0]))

jobs = json.load(open(sys.argv[1]))
jobs = jobs["jobs"]

#import pdb
#pdb.set_trace()

p = multiprocessing.Pool(32)
import tqdm
iterator = tqdm.tqdm(jobs, ncols=100)

for job, result in zip(iterator, p.imap(check_one_job, jobs)):
    if result is None:
        continue

    key = result["key"]
    res = result["res"]
    t = result["time"]

    stat = stats[key]

    stat["wr"][0] += res[0]
    stat["wr"][1] += res[1]

    if t is not None:
        stat["time"][0] += t
        stat["time"][1] += 1

    if res[1] == 0:
        print("missing " + result["log"])

for k, stat in stats.items():
    wr = stat["wr"]
    t = stat["time"]

    print("%s: %d/%d (%.2f%%), avg_time: %.2fms" % (k, wr[0], wr[1], float(wr[0]) * 100 / (wr[1] + 1e-10), t[0] / (t[1] + 1e-10)))

