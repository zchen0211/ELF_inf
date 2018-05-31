import re

param_matcher = re.compile(r"(--|-)([^ ]+)[ ]+([^\" -][^ ]*|\"[^\"]*\"|)")
env_matcher = re.compile(r"([^ ]+)=([^ ]+)")

def parse_params(command):
    envs = { m.group(1) : m.group(2) for m in env_matcher.finditer(command) }
    params = { m.group(2) : m.group(3) for m in param_matcher.finditer(command) }

    new_params = dict()
    for k, v in params.items():
        subenvs, subparams = parse_params(v)
        if len(subenvs) > 0 or len(subparams) > 0:
            new_params[k] = dict(params=subparams, envs=subenvs)
        else:
            new_params[k] = v
    return envs, new_params

if __name__ == "__main__":
    command = '''env LD_LIBRARY_PATH=$TARGET/mylibs LD_PRELOAD=$TARGET/mylibs/libstdc++.so:$TARGET/mylibs/libzmq.so.5:$TARGET/mylibs/libsqlite3.so.0 save=$TASK_DIR job_id=$CHRONOS_JOB_ID job_process_id=0 stdbuf -o 0 -e 0 /mnt/vol/gfsai-flash-east/ai-group/users/yuandong/rts/go_bots/gogui-1.4.9/bin/gogui-twogtp -black "env model_file=elfgames.go.df_model3 model=df_pred game=elfgames.go.game $PY -u ./experimental/df_console.py  --mode online --num_games 1 --verbose --mcts_verbose_time --ply_pass_enabled 0 --mcts_threads 1 --mcts_rollout_per_thread 1600 --following_pass --load /mnt/vol/gfsai-flash-east/ai-group/users/yuandong/agz_selfplay2/128_10_models/save-600000.bin --use_mcts --use_mcts_ai2 --mcts_persistent_tree --mcts_use_prior --mcts_virtual_loss 1 --mcts_epsilon 0.0 --mcts_alpha 0.0 --mcts_puct 0.85 --leaky_relu --resign_thres 0.05 --keys_in_reply V rv --gpu 0 --port 1234 --T 1 --selfplay_timeout_usec 10 --replace_prefix resnet.module,resnet --no_check_loaded_options --no_parameter_print --dim 128 --num_block 10" -white "taskset -c 0-11 /mnt/vol/gfsai-flash-east/ai-group/users/yuandong/rts/go_bots/leela_0110_linux_x64 -g --noponder" -games 2 -size 19 -sgffile $TASK_DIR/test_t1 -verbose -komi 7.5 -auto -alternate'''

    envs, params = parse_params(command)
    print("Envs:")
    print(envs)

    print("Params:")
    print(params)
