
configfile=$1
file=$2

. vg_hdr

. $configfile || exit 1

if [[ $file != '' ]] then
    flist=$file
else
    flist=
    for i in "${!files.@}"; do
        [[ $i == *.*.* ]] && continue
        flist+=" ${i#*.}"
    done
fi

for file in $flist; do
    typeset -n fdata=files.$file
    if [[ ${fdata.locationmode} != dir ]] then
        # this is not available on uwin ksh yet
        #md5=${ sum -x md5 "${fdata.location}"; }
        md5=$(sum -x md5 "${fdata.location}")
        print "${md5%%' '*}|$file||${fdata.location}"
    else
        for i in ${fdata.location}/*; do
            [[ ! -f $i ]] && continue
            [[ ${i##*/} != ${fdata.locationre:-'*'} ]] && continue
            #md5=${ sum -x md5 "$i"; }
            md5=$(sum -x md5 "$i")
            print "${md5%%' '*}|$file|${i##*/}|$i"
        done
    fi
done

exit 0
