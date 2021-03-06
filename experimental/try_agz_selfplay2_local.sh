LOAD_PATH=$1
shift
GPU=$1
shift

ROOT=`python3 root_path.py`

NUM_ROLLOUTS=100
#NUM_ROLLOUTS=25

DIM=128
NUM_BLOCK=10

#DEBUG="--dump_record_prefix tree --num_games 1 --batchsize 8 --preload_sgf ../data/elfgames/go/sgf/sample3.sgf"

root=$ROOT/$LOAD_PATH game=elfgames.go.game model=df_pred model_file=elfgames.go.df_model3 python3 -u df_selfplay.py \
    --mode selfplay --selfplay_timeout_usec 10  --batchsize 128 --num_games 32 --num_minibatch 500000 --keys_in_reply V rv \
    --mcts_threads 8 --mcts_rollout_per_thread $NUM_ROLLOUTS --use_mcts --use_mcts_ai2 --mcts_use_prior --mcts_persistent_tree --mcts_puct 0.85 \
    --policy_distri_cutoff 30 \
    --mcts_virtual_loss 5 --mcts_epsilon 0.25 --mcts_alpha 0.03 \
    --resign_thres 0.05 \
    --num_block0 $NUM_BLOCK --dim0 $DIM \
    --num_block1 $NUM_BLOCK --dim1 $DIM \
    --verbose \
    --replace_prefix0 resnet.module,resnet --replace_prefix1 resnet.module,resnet \
    "$@" \
    $DEBUG \
    --gpu $GPU
