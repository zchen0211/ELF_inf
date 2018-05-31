PREFIX=$1
A=`grep -r "+R\|\.5" $PREFIX*/*.dat | grep "B\|W" | grep -v "unexpected" | awk '{print $3}' | grep -c "+"`
B=`grep -r "+R\|\.5" $PREFIX*/*.dat | grep "B\|W" | grep -v "unexpected" | awk '{print $3}' | grep -c "B+"`
grep -r "+R\|\.5" $PREFIX*/*.dat | grep "B\|W" | grep -v "unexpected"
echo Game number: $A, B wins $B, W wins $((A-B))

