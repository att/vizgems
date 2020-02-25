
n=$(uname -n)

dir=/shared/users/eugene/ulog_derived

if [[ ! -d $dir ]] then
    exit 0
fi

typeset -A dates
for (( i = 0; i < 10; i++ )) do
    dates[${ printf '%(%Y_%m_%d)T' "$i day ago"; }]=1
done

ifs="$IFS"
for date in ${!dates[@]}; do
    for file in $dir/$date*.log; do
        [[ ! -f $file ]] && continue
        IFS="|"
        ./vgtail $file "" ./state_${file##*/} \
        | while read -r j j j n t1 t2 j v; do
            [[ $v == '' || $v != +([0-9.-]) ]] && continue
            print -r "ID=$TAILHOST:KEY=$n:TI=${t2%.*}:VAL=$v:LABEL=$n"
        done
        IFS="$ifs"
    done
done

for state in state_[0-9]*.log; do
    [[ ! -f $state ]] && continue
    date=${state#*state_}
    date=${date:0:10}
    [[ ${dates[$date]} != 1 ]] && rm $state
done
