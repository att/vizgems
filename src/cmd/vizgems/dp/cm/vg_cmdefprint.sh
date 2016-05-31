
configfile=$1
file=$2
rfile=$3
fmt=$4

. vg_hdr

. $configfile || exit 1

typeset -n fdata=files.$file

if [[ ${fdata.name} == '' ]] then
    print -u2 ERROR cannot find file info in config
    exit 1
fi

filefuns=
for dir in ${PATH//:/ }; do
    if [[ -f $dir/../lib/vg/${fdata.filefuns} ]] then
        filefuns=$dir/../lib/vg/${fdata.filefuns}
        break
    fi
done
if [[ ! -f $filefuns ]] then
    print -u2 ERROR cannot find function file for data file: ${fdata.name}
    exit 1
fi
. $filefuns
if [[ $? != 0 ]] then
    print -u2 ERROR cannot load function file $filefuns
    exit 1
fi

if [[ $rfile == '' ]] then
    exec 0< ${fdata.location}
elif [[ $rfile == */* ]] then
    exec 0< $rfile
elif [[ $rfile != - ]] then
    exec 0< ${fdata.location}/$rfile
    if [[ ${fdata.locationmode} == dir ]] then
        if ! ${fdata.filefuns}_isallowed; then
            print -u2 ERROR user is not allowed to view this file
            exit 0
        fi
    fi
fi

if [[ ${fdata.fileprinthelper} != '' ]] then
    ${fdata.fileprinthelper} | while read -r line; do
        ${fdata.filefuns}_split $fmt "$line"
    done
else
    while read -r line; do
        ${fdata.filefuns}_split $fmt "$line"
    done
fi

exit 0
