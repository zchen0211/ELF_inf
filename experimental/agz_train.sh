MACHINE=`hostname`
PREFIX=${MACHINE:6:3}
echo $PREFIX

ROOT=`python3 root_path.py`

DIM=64
NUM_BLOCK=5

NUM_ROLLOUTS=200

save=$ROOT/$PREFIX game=elfgames.go.game model=df_kl model_file=elfgames.go.df_model3 python3 -u train.py --mode train --server_id $PREFIX --batchsize 2048 --num_games 8192 --keys_in_reply V --num_block 10 --T 1 --use_data_parallel --num_minibatch 1000 --num_episode 1000000 \
    --mcts_threads 8 --mcts_rollout_per_thread $NUM_ROLLOUTS --use_mcts --use_mcts_ai2 --mcts_use_prior --mcts_persistent_tree --mcts_puct 0.85 \
    --mcts_virtual_loss 1 --mcts_epsilon 0.25 --mcts_alpha 0.03 \
    --num_block $NUM_BLOCK --dim $DIM \
    --resign_thres 0.01 \
    "$@"
