MACHINE=`hostname`
PREFIX=${MACHINE:6:3}
echo $PREFIX

ROOT=`python3 root_path.py`

save=$ROOT/$PREFIX game=elfgames.go.game model=df_kl model_file=elfgames.go.df_model3 python3 train.py --mode offline_train --batchsize 2048 --num_games 8192 --keys_in_reply V --num_block 5 --T 1 --use_data_parallel --num_minibatch 5000 --num_episode 1000000 "$@"
