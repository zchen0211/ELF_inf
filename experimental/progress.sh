MACHINE=$1

ROOT=`python3 root_path.py`

LAST_TRAIN=`ls -t $ROOT/$MACHINE/train_*.log | head -n 1`

echo "Train log: $LAST_TRAIN"
grep "PID" $LAST_TRAIN
grep "^Version:" -m 1 $LAST_TRAIN
CLIENT_JOB_ID=`grep -o -e "Ctrl from \([0-9]\+\)" -m 1 $LAST_TRAIN | sed "s/Ctrl from //g"`
echo "ClientJob: " $CLIENT_JOB_ID
grep "^/home" $ROOT/../rts/$CLIENT_JOB_ID/output.log
echo "=============================="
tail -n 30 $LAST_TRAIN
echo "=============================="
grep "^Sealed" $LAST_TRAIN

