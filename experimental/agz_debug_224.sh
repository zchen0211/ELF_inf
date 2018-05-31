LOAD_PATH=$1
shift
GPU=$1
shift

ROOT=`python3 root_path.py`

NUM_ROLLOUTS=3200
#NUM_ROLLOUTS=25

DIM=224
NUM_BLOCK=20
BATCHSIZE=16

DEBUG="--dump_record_prefix tree --num_games 1 --batchsize $BATCHSIZE --preload_sgf ../data/elfgames/go/sgf/sample3.sgf"

root=$LOAD_PATH game=elfgames.go.game model=df_pred model_file=elfgames.go.df_model3 python3 -u df_selfplay.py \
    --mode selfplay --keys_in_reply V rv --selfplay_timeout_usec 10 \
    --mcts_threads 2 --mcts_rollout_per_thread $NUM_ROLLOUTS --mcts_rollout_per_batch $BATCHSIZE --use_mcts --use_mcts_ai2 --mcts_use_prior --mcts_persistent_tree --mcts_puct 0.85 \
    --policy_distri_cutoff 0 \
    --mcts_virtual_loss 1 --mcts_epsilon 0 --mcts_alpha 0 \
    --resign_thres 0.0 \
    --replace_prefix0 resnet.module,resnet --replace_prefix1 resnet.module,resnet \
    --num_block0 $NUM_BLOCK --num_block1 $NUM_BLOCK --dim0 $DIM --dim1 $DIM \
    --no_check_loaded_options0 \
    --no_check_loaded_options1 \
    --verbose \
    $DEBUG \
    "$@" \
    --gpu $GPU
