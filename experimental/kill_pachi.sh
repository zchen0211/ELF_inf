IDS=`ps aux | grep "[p]achi\|[d]f_console" | grep -v "kill_pachi" | awk '{print $2}'`

for i in $IDS; 
do 
  #echo $i
  kill -9 $i 
done 
