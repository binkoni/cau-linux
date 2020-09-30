for i in $(seq 1 $1)
do
    for j in $(seq 1 $2)
    do
        echo -n "$i*$j=$(expr $i \* $j) "
    done
    echo ""
done
