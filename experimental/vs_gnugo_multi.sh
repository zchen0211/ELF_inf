MACHINE=$1
NUM_GPU=$2
NUM_PER_GPU=$3
GAME=$4
for i in `seq 0 $((NUM_GPU-1))`
do
  for j in `seq 0 $((NUM_PER_GPU-1))` 
  do
    sh vs_gnugo.sh $MACHINE $((7-i)) $((i*NUM_PER_GPU+j)) $GAME &
    sleep 1s
  done
done
