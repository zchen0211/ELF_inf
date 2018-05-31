logfile=$1
start_raw=`head -1 $1 | awk -F "[" '{print $2}' | awk -F "]" '{print $1}'`
start=`date -d "$start_raw" +"%s"`
#end_raw=`tail -1 $1 | awk -F "[" '{print $2}' | awk -F "]" '{print $1}'`
#end=`date -d "$end_raw" +"%s"`
current=`date +"%s"`
diff=$(($current-$start))
hour=$(($diff / 3600))
minute=$(($diff / 60 - $hour * 60))
echo "[INFO] job has been running for $hour hours and $minute minutes"
sealed=`grep -c Sealed $1`
if [ $sealed -eq 0 ] 
  then
  echo "[INFO] No Sealed yet"
  if [ $hour -gt 4 ]
    then
    echo "[WARNING] 5 hours no sealed!"
  fi 
else
  sealed=`grep Sealed $1 | tail -1`
  latest_seal_raw=`echo $sealed | awk -F "[" '{print $3}' | awk -F "]" '{print $1}'` 
  latest_seal=`date -d "$latest_seal_raw" +"%s"`
  diff2=$(($current-$latest_seal)) 
  hour2=$(($diff2 / 3600)) 
  minute2=$(($diff2 / 60 - $hour2 * 60))
  echo "[INFO] latest seal is $hour2 hours and $minute2 minutes ago."
  status=`echo $sealed | awk -F "=" '{print $2}' | awk -F "]" '{print $1}'` 
  if [ $status -eq 0 -a $hour2 -gt 1 ] 
    then
    echo "[WARNING] last seal has pass=0 but took more than 2 hours"
  fi
  if [ $status -eq 1 -a $hour2 -gt 3 ] 
    then
    echo "[WARNING] last seal has pass=1 but took more than 4 hours"
  fi
fi
for i in `grep value_loss $1 | awk -F "." '{print $2}' | awk -F "," '{print $1}'`
do
  if [ $i -lt 30000 ] 
  then
    echo "[WARNING] value_loss dropped below 0.3! You might be overfitting."
    break
  fi
done
t=`grep Sealed $1 | tail -5 | grep -c "pass=0"`
echo "[INFO] currently you have $t latest models below 55%"
if [ $t -gt 4 ]
then
  echo "[WARNING] lastest 5 models failed to pass. Model is not improving!"
fi
