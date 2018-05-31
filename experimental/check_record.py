import os
import sys
import random
import json
from collections import Counter, defaultdict
import argparse

def coord2move(m):
    if m == 0: return "PASS"
    x = m % 21 - 1
    if x >= 8:
        x += 1
    y = m // 21 - 1
    return chr(x + 65) + str(y + 1)

def print_game(d, args):
    request = d["request"]
    result = d["result"]
    player_swap = request.get("player_swap", result.get("player_swap", request["client_ctrl"]["player_swap"]))
    reward = result["reward"]
    pb = request["vers"]["black_ver"]
    pw = request["vers"]["white_ver"]
    if player_swap:
        pb, pw = pw, pb

    content = d["result"]["content"]

    if reward == 1.0:
        res = "B+R"
    elif reward == -1.0:
        res = "W+R"
    else:
        res = ("B+%.1f" % reward) if reward > 0 else ("W+%.1f" % -reward)

    title = "idx: %d, swap: %s, result: %f" % (d["idx"], str(player_swap), reward)

    sgf_header = "SZ[19]KM[7.5]RU[CN]PW[%s]PB[%s]RE[%s]C[%s]" % (pw, pb, res, title)

    moves = content[2:-1].split(";")

    values = d["result"].get("values", [])
    if len(values) > 0 and len(moves) != len(values):
        print("Moves has %d entries while values has %d entries" % (len(moves), len(values)))
        print("Last value: %f" % values[-1])

    policies = d["result"].get("policies", [])

    if args.move_idx is not None:
        if args.move_idx < 0:
            move_idx = len(moves) + args.move_idx
        else:
            move_idx = args.move_idx
    else:
        move_idx = -1

    contents = []
    for i, m in enumerate(moves):
        comment = "%d: " % (i + 1)
        if i < len(values):
            comment += "%.3f" % values[i]
        if i < len(policies) and i == move_idx:
            comment += ", Policy: \n"
            sorted_policies = [ (j, p) for j, p in enumerate(policies[i]) ]
            sorted_policies.sort(key=lambda x : x[1], reverse=True)
            sum_value = sum(policies[i])
            for j, p in sorted_policies[:10]:
                comment += coord2move(j) + ": %f\n" % (float(p) / sum_value)

        contents.append(m + "C[" + comment + "]")
        content = ";".join(contents)
        sgf = "(;" + sgf_header + ";" + content + ")"

    #import pdb
    #pdb.set_trace()
    print(title)
    print(sgf)
    print("")

def print_games(data, indices, args):
    for idx in indices:
        print_game(data[idx], args)

def main():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('filename', type=str, help="Filename to check")
    parser.add_argument('--rand_n', type=int, default=None)
    parser.add_argument("--print_all", action="store_true")
    parser.add_argument('--idx', type=str, default=None)
    parser.add_argument("--move_idx", type=int, default=None)
    parser.add_argument("--print_never_resign", action="store_true")
    parser.add_argument("--print_never_resign_and_success", action="store_true")
    parser.add_argument("--print_never_resign_and_failed", action="store_true")
    parser.add_argument("--print_white_wins", action="store_true")
    parser.add_argument("--print_weird_pass", action="store_true")
    parser.add_argument("--print_large_value_change", type=float, default=None)

    args = parser.parse_args()

    data = json.load(open(args.filename))
    keys = Counter()

    selfplay_record = args.filename.startswith("selfplay")

    # B/W B/W
    wins = defaultdict(lambda : [0] * 4)
    black_never_resign_indices = []
    white_never_resign_indices = []
    never_resign_and_success = []
    never_resign_and_failed = []

    large_value_change = set()
    sum_game_length = 0

    for i, v in enumerate(data):
        request = v["request"]
        result = v["result"]
        black_ver = request["vers"]["black_ver"]
        white_ver = request["vers"]["white_ver"]
        key = "b:%d, w:%d" % (black_ver, white_ver)
        keys[key] += 1

        values = v["result"]["values"]
        
        player_swap = request.get("player_swap", result.get("player_swap", request["client_ctrl"]["player_swap"]))
        result["player_swap"] = player_swap
        r = result["reward"]

        if selfplay_record:
            player_swap = False

        sum_game_length += len(values)

        if player_swap:
            if r > 0:
                wins[key][3] += 1
            else:
                wins[key][2] += 1
        else:
            if r > 0:
                wins[key][0] += 1
            else:
                wins[key][1] += 1

        v["idx"] = i

        if result["black_never_resign"]:
            black_never_resign_indices.append(i)
            if r > 0:
                never_resign_and_success.append(i)
            else:
                never_resign_and_failed.append(i)

        if result["white_never_resign"]:
            white_never_resign_indices.append(i)
            if r < 0:
                never_resign_and_success.append(i)
            else:
                never_resign_and_failed.append(i)

        # check whether there is any large value changes.
        if args.print_large_value_change is not None:
            for j in range(len(values) - 1):
                if abs(values[j + 1] - values[j]) > args.print_large_value_change:
                    large_value_change.add(i)

        final_mcts = values[-2]
        if final_mcts > 0 and r < 0 or final_mcts < 0 and r > 0:
            print("Suspecious game")
            print_game(v, args)

    n = sum(keys.values())
    print("#record: %d, avg_game_length: %.3f" % (n, sum_game_length / n))
    for key, v in keys.items():
        win = wins[key]
        n = sum(win)
        winrate = float(win[0] + win[2]) / n
        wr_str = "B/W: %d/%d" % (win[0], win[1])
        if not selfplay_record:
            wr_str += ", B/W: %d/%d" % (win[2], win[3])

        print("%s: %d/%d, %s, black win: %.2f (%d)" % (key, v, n, wr_str, winrate, n))

    if args.print_large_value_change is not None:
        ratio = len(large_value_change) / n
        print("#Large value change: %d/%d (%.4f)" % (len(large_value_change), n, ratio))

    if args.idx is not None:
        for idx in args.idx.split(","):
            print_game(data[int(idx)], args)

    if args.rand_n is not None:
        for _ in range(args.rand_n):
            idx = random.randint(0, len(data) - 1)
            print_game(data[idx], args)

    if args.print_all is not None:
        for i in range(len(data)):
            print_game(data[i], args)

    if args.print_white_wins:
        for d in data:
            w_win1 = d["result"]["player_swap"] and d["result"]["reward"] > 0
            w_win2 = not d["result"]["player_swap"] and d["result"]["reward"] < 0
            if w_win1 or w_win2:
                print_game(d, args)

    if args.print_weird_pass:
        cnt = 0
        for d in data:
            b_win_w_pass_first = d["result"]["reward"] > 0 and d["result"]["content"].find("W[];B[]") >= 0
            w_win_b_pass_first = d["result"]["reward"] < 0 and d["result"]["content"].find("B[];W[]") >= 0
            if b_win_w_pass_first or w_win_b_pass_first:
                print_game(d, args)
                cnt += 1
        print("Total count: %d" % cnt)

    if args.print_never_resign:
        print("Black never resign[%d]" % len(black_never_resign_indices))
        print_games(data, black_never_resign_indices, args)

        print("White never resign[%d]" % len(white_never_resign_indices))
        print_games(data, white_never_resign_indices, args)

    if args.print_never_resign_and_success:
        print_games(data, never_resign_and_success, args)

    if args.print_never_resign_and_failed:
        print_games(data, never_resign_and_failed, args)

    if args.print_large_value_change is not None:
        print_games(data, large_value_change, args)

if __name__ == '__main__':
    main()
