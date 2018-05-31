LOAD_PATH=$1
shift
GPU=$1
shift

ROOT=`python3 root_path.py`

NUM_ROLLOUTS=200
#NUM_ROLLOUTS=25

DIM=64
NUM_BLOCK=5

root=$ROOT/$LOAD_PATH game=elfgames.go.game model=df_pred model_file=elfgames.go.df_model3 python3 -u df_selfplay.py \
    --mode selfplay --selfplay_timeout_usec 10 --batchsize 128 --num_games 32 --keys_in_reply V rv --port 2341 --server_id $LOAD_PATH \
    --mcts_threads 8 --mcts_rollout_per_thread $NUM_ROLLOUTS --use_mcts --use_mcts_ai2 --mcts_use_prior --mcts_persistent_tree --mcts_puct 0.25 \
    --policy_distri_cutoff 30 \
    --mcts_virtual_loss 5 --mcts_epsilon 0.0 --mcts_alpha 0.00 \
    --resign_thres 0.0 \
    --num_block0 $NUM_BLOCK --dim0 $DIM \
    --num_block1 $NUM_BLOCK --dim1 $DIM \
    --verbose \
    --no_check_loaded_options0 \
    --no_check_loaded_options1 \
    --move_cutoff 3\
    --cheat_eval_new_model_wins_half \
    --replace_prefix0 resnet.module,resnet --replace_prefix1 resnet.module,resnet \
    --gpu $GPU\
    "$@" \
