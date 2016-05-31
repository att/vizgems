
configfile=$1
file=$2
efile=$3

. vg_hdr

. $configfile || exit 1

export DOCUMENT_ROOT=$VG_DWWWDIR/htdocs

typeset -n fdata=files.$file

if [[ ${fdata.name} == '' ]] then
    print -u2 ERROR cannot find file info in config
    exit 1
fi

ifs="$IFS"
IFS=';'
while read -r op id pass groups name info; do
    if [[ $op == '<u>'*'</u>' ]] then
        print -u5 "${op//@('<u>'|'</u>')/}"
        continue
    fi
    [[ $op != @(add|mod|del) ]] && continue
    swmaccounts "$op" "$id" "$pass" "$groups" "$name" "$info" || break
done < $efile
IFS="$ifs"

exit 0
