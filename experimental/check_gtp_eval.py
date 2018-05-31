import os
from collections import defaultdict
import sys
import re
import json
import multiprocessing

import check_utils

winner = re.compile(r"B+(.*?).B+(.*?).B+(.*?)")
# param_matcher = re.compile(r"test_bs(\d+)_t(\d+)")

timer_matcher = re.compile(r"MCTS: (\d+)ms")
stop_matcher = re.compile(r"Prepare to stop")

root = "/mnt/vol/gfsai-flash-east/ai-group/users/yuandong/rts"

def parse_dat(filename):
    n_win = 0
    n = 0

    try:
        for l in open(filename):
            if l.startswith("#"): continue
            if winner.search(l): n_win += 1
            n += 1
    except:
        pass

    return n_win, n

def parse_log(log):
    sum_time = 0
    count = 0

    try:
        for l in open(log):
            if stop_matcher.search(l):
                break

            m = timer_matcher.search(l)
            if m:
                sum_time += int(m.group(1))
                count += 1
    except:
        pass

    return sum_time / count if count > 0 else None


def check_one_job(job):
    envs, params = check_utils.parse_params(job["command"])

    bot = params["black"]["params"]

    batchsize = int(bot.get("batchsize", 1))
    num_threads = int(bot["mcts_threads"])
    num_rollouts = int(bot["mcts_rollout_per_thread"]) * num_threads
    virtual_loss = int(bot["mcts_virtual_loss"])

    key = "bs=%d,threads=%d,#rollout=%d,#virtual_loss=%d: " % (batchsize, num_threads, num_rollouts, virtual_loss)

    f = os.path.join(root, str(job["id"]), str(job["job_idx"]), os.path.basename(params["sgffile"]) + ".dat")
    log = os.path.join(root, str(job["id"]), "output%d-0.log" % job["job_idx"])

    result = dict(key=key, f=f, res=(0, 0), time=None)

    if os.path.exists(f):
        result["res"] = parse_dat(f)
        # print("%s %d %d" % (f, result["res"][0], result["res"][1]))

    if os.path.exists(log):
        result["time"] = parse_log(log)

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
        print("missing " + result["f"])

for k, stat in stats.items():
    wr = stat["wr"]
    t = stat["time"]

    print("%s: %d/%d (%.2f%%), avg_time: %.2fms" % (k, wr[0], wr[1], float(wr[0]) * 100 / (wr[1] + 1e-10), t[0] / (t[1] + 1e10)))

