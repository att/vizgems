
configfile=$1

. $configfile || exit 1

file=$2

typeset -n fdata=files.$file

headersep=${fdata.headersep}
footersep=${fdata.footersep}

if [[ $fdata == '' ]] then
    print -u2 SWIFT ERROR cannot find file info in config
    exit 1
fi

mode=$3

if [[ $mode != insert && $mode != remove && $mode != modify ]] then
    print -u2 SWIFT ERROR unknown operation: $mode
    exit 1
fi

ext=$4

if [[ $ext == '' ]] then
    print -u2 SWIFT ERROR missing temporary file extension
    exit 1
fi

nrec=$5

if [[ $mode != remove && $nrec == '' ]] then
    print -u2 SWIFT ERROR missing new data record
    exit 1
fi

orec=$6

if [[ $mode != insert && $orec == '' ]] then
    print -u2 SWIFT ERROR missing old data record
    exit 1
fi

if [[ $mode == insert ]] then
    orec=
fi

if [[ ! -f ${fdata.location} ]] then
    print -u2 SWIFT WARNING creating new ${fdata.location} file
    touch ${fdata.location}
    if [[ ! -f ${fdata.location} ]] then
        print -u2 SWIFT ERROR cannot create ${fdata.location} file
        exit 1
    fi
fi

if [[ ! -f ${fdata.location}.$ext ]] then
    cp ${fdata.location} ${fdata.location}.$ext
    if [[ ! -f ${fdata.location}.$ext ]] then
        print -u2 SWIFT ERROR cannot create ${fdata.location}.$ext temp file
        exit 1
    fi
fi

tfile=${fdata.location}.$ext

integer minn=0

if [[ $headersep != '' ]] then
    if ! fgrep "$headersep" $tfile > /dev/null 2>&1; then
        print -u2 SWIFT ERROR header line not present in $tfile
        exit 1
    fi
    (( minn++ ))
fi

if [[ $footersep != '' ]] then
    if ! fgrep "$footersep" $tfile > /dev/null 2>&1; then
        print -u2 SWIFT ERROR footer line not present in $file
        exit 1
    fi
    (( minn++ ))
fi

exec 4< $tfile
exec 5> $tfile.new.1

if [[ $headersep != '' ]] then
    while read -u4 -r line; do
        if [[ $line == *$headersep* ]] then
            break
        fi
        print -u5 -r -- $line
    done
fi

if [[ $headersep != '' ]] then
    print -u5 -r -- "$headersep"
fi

exec 5>&-
exec 5> $tfile.new.2

while read -u4 -r line; do
    if [[ $footersep != '' && $line == *$footersep* ]] then
        break
    fi
    if [[ $line == "$orec" ]] then
        case $mode in
        insert|modify)
            print -u5 -r -- "$nrec"
            done=y
            ;;
        remove)
            ;;
        esac
    else
        print -u5 -r -- "$line"
    fi
done
if [[ $mode == insert && $done != y ]] then
    print -u5 -r -- "$nrec"
fi
if [[ $mode == modify && $done != y ]] then
    print -u2 "SWIFT ERROR did not find record to modify: '$nrec'"
    exit 1
fi

if [[ $footersep != '' ]] then
    print -u5 -r -- "$footersep"
fi

exec 5>&-
exec 5> $tfile.new.3

while read -u4 -r line; do
    print -u5 -r -- $line
done

exec 4<&-
exec 5>&-

(
    cat $tfile.new.1
    sort -u $tfile.new.2
    cat $tfile.new.3
) > $tfile.new
rm $tfile.new.[123]

n=$(wc -l < $tfile.new)
if (( n < minn )) then
    print -u2 SWIFT ERROR generated file is too short
    exit 1
fi

if ! mv $tfile.new $tfile; then
    print -u2 SWIFT ERROR cannot update temp file $tfile
    exit 1
fi

exit 0
