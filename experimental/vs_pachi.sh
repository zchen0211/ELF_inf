MACHINE=$1
shift
GPU=$1
shift
NUM=$1
shift
GAME=$1
shift
PARAMS="$@"
ROOT=`python3 root_path.py`/$MACHINE
PACHI=/home/qucheng/pachi/pachi

BATCHSIZE=4

echo "to get best performance of pachi, copy /home/qucheng/pachi/patterns.spat and patterns.prob to current dir"
while true;
do
    MODEL_NAME=`readlink $ROOT/picked`
    echo $MACHINE: $MODEL_NAME
    SUFFIX=${MODEL_NAME%.bin}
    SUFFIX=${SUFFIX##*-}
    DIR="pachi-${MACHINE}-${SUFFIX}-$NUM"
    mkdir -p $DIR 
    run=0
    for puct in 0.85; #1.0; # 1.0 5.0; 
    do
        LOG_FILE=$DIR/output_$puct.log
        if [ ! -f "$LOG_FILE" ]; then 
            /home/yuandong/gogui-1.4.9/bin/gogui-twogtp -white "$PACHI -t =10000:20000 threads=4,max_tree_size=200" -black "sh ./agz_gtp.sh $ROOT/${MODEL_NAME} --verbose --gpu $GPU --mcts_puct $puct --batchsize $BATCHSIZE --mcts_rollout_per_batch $BATCHSIZE --mcts_threads 2 --mcts_rollout_per_thread 800 --mcts_virtual_loss 5 --resign_thres 0.05 --following_pass $PARAMS" -games $GAME -size 19 -sgffile $DIR/test_puct_$puct -verbose -komi 7.5 -auto -alternate 2> $LOG_FILE
            run=1
        else
            echo "$LOG_FILE already exist, skip..."
        fi
        if [ "$run" -eq "0" ]; then
            sleep 1m
        fi
    done
done

