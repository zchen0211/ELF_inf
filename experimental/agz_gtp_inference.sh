#ROOT=/mnt/vol/gfsai-flash-east/ai-group/users/yuandong/agz_selfplay2/140_Dec12_500gpu_rb50k_uniform_explore
#ROOT=/mnt/vol/gfsai-flash-east/ai-group/users/yuandong/agz_selfplay2/140_kl_30_Dec10
MODEL=$1
shift

game=elfgames.go.game_inference model=df_pred model_file=elfgames.go.df_model3 python3 df_console.py --mode online --keys_in_reply V rv \
    --use_mcts --mcts_verbose_time --mcts_use_prior --mcts_persistent_tree --load $MODEL \
    --no_check_loaded_options \
    --replace_prefix resnet.module,resnet \
    --no_check_loaded_options \
    --no_parameter_print \
    "$@"
