
typeset -A sizes

sizesfile=$1

while read name size j; do
    sizes[$name]=$size
done < $sizesfile

while read line; do
    for name in "${!sizes[@]}"; do
        line=${line//$name/${sizes[$name]}}
    done
    if [[ $line == *SCHEMA_* ]] then
        print -u2 SWIFT ERROR missing size definition: $line
        exit 1
    fi
    print -r "$line"
done
