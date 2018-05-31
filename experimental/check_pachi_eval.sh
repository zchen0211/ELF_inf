#!/bin/bash

JOBS=`pushd /home/yuandong/tools/clist > /dev/null; /home/yuandong/anaconda3/bin/python get_job_ids.py $1; popd > /dev/null`
#echo $JOBS

TOT_N=0
TOT_W=0
TOT_F=0
TOT_L=0

for job in $JOBS; do
  CNT=0
  while true;
  do
    f=$RTSROOT/$job/$CNT/test.dat
    if [ -e "$f" ]; then
      N=`grep -c -v "^#" $f`
      W=`grep -c "B+\(.?*\).B+\(.?*\).B+\(.?*\)" $f` 
      TOT_N=$((TOT_N+N))
      TOT_W=$((TOT_W+W))
      TOT_F=$((TOT_F+1))

      echo $f $N $W
    else
      break
    fi
    CNT=$((CNT+1))
  done
  if [ "$CNT" -gt 0 ]; then
    TOT_L=$((TOT_L+1))
  fi
done

wr=`echo "$TOT_W/$TOT_N" | bc -l`
echo Total valid files: $TOT_F/$TOT_L, Total games: $TOT_N, Win: $TOT_W, Winrate: $wr
