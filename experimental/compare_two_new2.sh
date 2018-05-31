PUCT=$1
shift

DEBUG_FLAGS="--print_result --dump_record_prefix $RECORD_PREFIX  --num_reset_ranking 1000"

game=elfgames.go.game model=df_pred model_file=elfgames.go.df_model3 python3 eval_selfplay_aivsai3.py \
    --mode selfplay_eval --T 1 \
    --num_games 2 --batchsize 8 --selfplay_timeout_usec 10 \
    --replace_prefix0 resnet.module,resnet --replace_prefix1 resnet.module,resnet --num_block 5 --dim 64 \
    --mcts_puct $PUCT --mcts_virtual_loss 5 --resign_thres 0.0 --mcts_threads 16 --mcts_rollout_per_thread 100 --policy_distri_cutoff 0 --mcts_persistent_tree --use_mcts --use_mcts_ai2 --mcts_use_prior \
    --print_result \
    --num_games_per_thread 1 \
    --keys_in_reply V \
    "$@" \
    # --tqdm --num_eval $NUM_EVAL \
