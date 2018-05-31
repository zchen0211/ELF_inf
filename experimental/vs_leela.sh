MODEL_NAME=$1
shift
GPU=$1
shift
NUM=$1
shift
GAME=$1
shift
PARAMS="$@"
LEELA=/home/qucheng/leela_0110_linux_x64

while true;
do
    echo $MODEL_NAME
    SUFFIX=${MODEL_NAME%.bin}
    SUFFIX=${SUFFIX##*-}
    DIR="leela-${SUFFIX}-$NUM"
    mkdir -p $DIR
    run=0
    for puct in 0.85; #1.0; # 1.0 5.0;
    do
        LOG_FILE=$DIR/output_$puct.log
        if [ ! -f "$LOG_FILE" ]; then
            /home/yuandong/gogui-1.4.9/bin/gogui-twogtp -white "$LEELA -g" -black "sh ./agz_gtp.sh $MODEL_NAME --verbose --gpu $GPU --mcts_puct $puct --mcts_threads 8 --mcts_rollout_per_thread 200 --resign_thres 0.05 --following_pass $PARAMS" -games $GAME -size 19 -sgffile $DIR/test_puct_$puct -verbose -komi 7.5 -auto -alternate 2> $LOG_FILE
            run=1
        else
            echo "$LOG_FILE already exist, skip..."
        fi
        if [ "$run" -eq "0" ]; then
            sleep 1m
        fi
    done
done
