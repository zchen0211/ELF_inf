import os
import sys
import socket
import root_path
from collections import OrderedDict
import argparse
import json
from deploy_util import *

# Write a remote script for evaluation server.
train_script = """
# First check whether we start from scratch
LATEST={%root}/{%server_id}/latest0

if [ -f "$LATEST" ]; then
    LOAD_PARAM="--load $LATEST"
else
    LOAD_PARAM=""
fi

source ../scripts/devmode_set_pythonpath.sh

UNBUFFER="stdbuf -o 0 -e 0"

echo $$ >{%train_log}
save={%working_dir} \
    game=elfgames.go.game \
    model=df_kl \
    model_file=elfgames.go.df_model3 \
    $UNBUFFER {%py} -u train.py \
    --mode train \
    --batchsize 2048 \
    --num_games 8192 \
    --keys_in_reply V \
    --T 1 \
    --use_data_parallel \
    --num_minibatch {%num_minibatch_per_save} \
    --num_episode 1000000 \
    --mcts_threads 8 \
    --mcts_rollout 200 \
    --keep_prev_selfplay {%keep_prev_selfplay} \
    --use_mcts \
    --use_mcts_ai2 \
    --mcts_persistent_tree \
    {%mcts_unexplored_q_zero} \
    {%mcts_root_unexplored_q_zero} \
    --mcts_use_prior \
    --mcts_virtual_loss 5 \
    --mcts_epsilon 0.25 \
    --mcts_alpha 0.03 \
    --mcts_puct {%puct} \
    --resign_thres {%resign_thres}\
    --gpu 0 \
    --server_id {%server_id} \
    --selfplay_init_num {%selfplay_init_num} \
    --selfplay_update_num {%selfplay_update_num} \
    --eval_num_games {%num_eval_games} \
    --eval_winrate_thres {%winrate_thres} \
    --port 1234 \
    --q_min_size {%init_replay_buffer_size} \
    --q_max_size {%replay_buffer_size} \
    --save_first \
    --list_files {%load_offline_selfplay} \
    --no_check_loaded_options \
    --expected_num_clients {%num_jobs} \
    --num_cooldown 50 \
    --bn_momentum {%bn_momentum} \
    --num_block {%num_block} \
    --dim {%dim} \
    {%use_df_feature} \
    {%leaky_relu} \
    $LOAD_PARAM {%train_params} \
    1>>{%train_log} 2>&1 &
"""

client_job_script = """
cd ~/tools/sweeper
{%py} exp.py ./experiments/agz_eval.py \
    -m "{%client_comment}" \
    --repo {%repo} \
    --repeat {%num_jobs} \
    --params_config '{%client_params}' \
    --num_process 1
"""

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--python_path", type=str,
                        default="anaconda3/bin/python")
    parser.add_argument("--num_jobs", type=int, default=100)
    parser.add_argument("--num_eval_games", type=int, default=400)
    parser.add_argument("--selfplay_init_num", type=int, default=12000)
    parser.add_argument("--selfplay_update_num", type=int, default=1000)
    parser.add_argument("--num_minibatch_per_save", type=int, default=5000)
    parser.add_argument("--repo", type=str, default="DF2-AGZ9")
    parser.add_argument("--eval_gpus", type=str, default="0,1")
    parser.add_argument("--dim", type=int, default=64)
    parser.add_argument("--num_block", type=int, default=5)
    parser.add_argument("--train_params", type=str, default="")
    parser.add_argument("--winrate_thres", type=float, default=0.55)
    parser.add_argument("--unexplored_q_zero", action="store_true")
    parser.add_argument("--root_unexplored_q_zero", action="store_true")
    parser.add_argument("--leaky_relu", action="store_true")
    parser.add_argument("--bn_momentum", type=float, default=0.0)
    parser.add_argument("--load_offline_selfplay", type=str, nargs="*", default=[])
    parser.add_argument("--keep_prev_selfplay", action="store_true")
    parser.add_argument("--resign_thres", type=float, default=0.01)
    parser.add_argument("--working_dir", type=str, default=None)
    parser.add_argument(
        "--init_replay_buffer_size",
        type=int,
        default=200,
        help="Minimal buffer size when the training starts."
             "init_replay_buffer_size * 50 will be the true buffer size")
    parser.add_argument(
        "--replay_buffer_size",
        type=int,
        default=1000,
        help="replay_buffer_size * 50 will be the true buffer size")
    parser.add_argument("--puct", type=float, default=0.25)
    parser.add_argument("--client_params", type=str, default=None)
    parser.add_argument("--root", type=str, default=None)
    parser.add_argument("--use_df_feature", action="store_true")
    parser.add_argument(
        "--policy_distri_training_for_all",
        action="store_true")
    parser.add_argument("--dry_run", action="store_true")
    parser.add_argument("--confirms", type=str)

    '''
    parser.add_argument("--selfplay_dim", type=int, default=None)
    parser.add_argument("--selfplay_num_block", type=int, default=None)
    parser.add_argument("--selfplay_load_file", type=str, default=None)
    '''

    args = parser.parse_args()
    hostname = socket.gethostname()
    server_id = hostname[6:9]
    print(server_id)
    signature = sig()

    if args.root is None:
        args.root = root_path.get()

    if args.working_dir is None:
        args.working_dir = os.path.join(args.root, server_id)

    common_params = OrderedDict()
    common_params["server_id"] = server_id
    common_params["puct"] = args.puct
    common_params["dim"] = args.dim
    common_params["num_block"] = args.num_block
    common_params["policy_distri_training_for_all"] = \
        args.policy_distri_training_for_all
    common_params["use_df_feature"] = args.use_df_feature
    common_params["root"] = args.root
    common_params["leaky_relu"] = args.leaky_relu
    common_params["load_dir"] = args.working_dir

    client_params = OrderedDict(json.loads(args.client_params))
    client_params.update(common_params)

    # Selfplay-specific parameters.
    '''
    if args.selfplay_dim is not None:
        selfplay_params["dim"] = args.selfplay_dim
    if args.selfplay_num_block is not None:
        selfplay_params["num_block"] = args.selfplay_num_block
    if args.selfplay_load_file is not None:
        selfplay_params["load_file"] = args.selfplay_load_file
    '''

    eval_gpus = args.eval_gpus.split(",")
    tbl = dict(
        server_id=server_id,
        eval_gpu0=eval_gpus[0],
        eval_gpu1=eval_gpus[1],
        winrate_thres=args.winrate_thres,
        puct=args.puct,
        signature=signature,
        repo=args.repo,
        replay_buffer_size=args.replay_buffer_size,
        init_replay_buffer_size=args.init_replay_buffer_size)
    tbl["py"] = os.path.join("/home", os.environ["USER"], args.python_path)
    tbl["root"] = args.root
    tbl["dim"] = args.dim
    tbl["num_block"] = args.num_block
    tbl["use_df_feature"] = args.use_df_feature
    tbl["leaky_relu"] = args.leaky_relu
    tbl["train_params"] = args.train_params
    tbl["num_jobs"] = args.num_jobs
    tbl["resign_thres"] = args.resign_thres
    tbl["keep_prev_selfplay"] = args.keep_prev_selfplay
    tbl["working_dir"] = args.working_dir
    tbl["load_offline_selfplay"] = " ".join(args.load_offline_selfplay)
    tbl["bn_momentum"] = args.bn_momentum

    tbl["num_eval_games"] = args.num_eval_games
    tbl["selfplay_init_num"] = args.selfplay_init_num
    tbl["selfplay_update_num"] = args.selfplay_update_num
    tbl["num_minibatch_per_save"] = args.num_minibatch_per_save
    tbl["mcts_unexplored_q_zero"] = args.unexplored_q_zero
    tbl["mcts_root_unexplored_q_zero"] = args.root_unexplored_q_zero
    tbl["client_params"] = json.dumps(client_params)
    tbl["client_comment"] = "client---" + \
        ",".join(["%s=%s" % (k, v)
                  for k, v in client_params.items()
                  if k != "root" and k != "load_dir"])

    # Add logs
    result_path = os.path.join(args.root, server_id)
    if not os.path.exists(result_path):
        os.mkdir(result_path)
    tbl["train_log"] = os.path.join(result_path, "train_" + signature + ".log")

    # Put everything into a log file.
    with open(os.path.join(result_path, "task_" + signature + ".log"), "w") \
            as f:
        json.dump(args.__dict__, f, sort_keys=True, indent=4)
        json.dump(tbl, f, sort_keys=True, indent=4)

    script_templates = [
        # Step 1. Go to the repo and run training code.
        ("train_local", train_script, "Train log: " + tbl["train_log"]),
        # Step 2. Push evaluation jobs.
        ("client_job", client_job_script, None),
    ]

    scripts = get_scripts(script_templates, tbl)

    if args.confirms is not None:
        confirms = {key.split(":")[0]: key.split(":")[1]
                    for key in args.confirms.split(",")}
    else:
        confirms = {}

    # devgpu = "devgpu" + args.remote_eval_id + ".prn2.facebook.com"
    for k, script, prompt in scripts:
        if k in confirms:
            run = confirms[k] == "y"
        else:
            run = confirm(k) if not args.dry_run else True

        if run:
            run_local(script, dry_run=args.dry_run)
            if prompt is not None:
                print(prompt)
        else:
            print("Skipping " + k)
