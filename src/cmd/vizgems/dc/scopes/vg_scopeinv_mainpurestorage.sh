
export cid=$CID aid=$AID ip=$IP user=$USER pass=$PASS cs=$CS
export targettype=$TARGETTYPE scopetype=$SCOPETYPE servicelevel=$SERVICELEVEL

set -o pipefail

export INVMODE=y

tools=vg_exec_cmd_mainpurestorage

urlbase=https://$IP
if [[ $1 == *urlbase=?* ]] then
    urlbase=${1#*urlbase=}
    urlbase=${urlbase%%'&'*}
fi

apiversion=1.6
if [[ $1 == *apiversion=?* ]] then
    apiversion=${1#*apiversion=}
    apiversion=${apiversion%%'&'*}
fi

cat > inv.txt <<#EOF
storageinv|number||EXEC:purestorage?apiversion=$apiversion&urlbase=$urlbase
EOF

for tool in $tools; do
    EXECUSER=$USER EXECPASS=$PASS $SHELL $VG_SSCOPESDIR/current/exec/${tool} \
        $aid $ip $TARGETTYPE \
    < inv.txt
done
