MACHINE=`hostname`
PREFIX=${MACHINE:6:3}
echo $PREFIX

ROOT=`python3 root_path.py`

DIM=128
NBLOCK=10

# --batchsize 256 --num_games 1024 

save=$ROOT/$PREFIX game=elfgames.go.game model=df_kl model_file=elfgames.go.df_model3 python3 compare_likelihood.py --batchsize 256 --num_games 1024 --keys_in_reply V \
  --num_block0 $NBLOCK --dim0 $DIM --num_block1 $NBLOCK --dim1 $DIM --num_block $NBLOCK --dim $DIM --T 1 --num_minibatch 5000 --num_episode 1000000 --replace_prefix0 resnet.module,resnet --replace_prefix1 resnet.module,resnet "$@" 
