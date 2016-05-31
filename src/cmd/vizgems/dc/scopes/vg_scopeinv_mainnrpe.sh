
export cid=$CID aid=$AID ip=$IP user=$USER pass=$PASS cs=$CS
export targettype=$TARGETTYPE scopetype=$SCOPETYPE servicelevel=$SERVICELEVEL

set -o pipefail

export INVMODE=y

tools=vg_exec_cmd_mainnrpe

urlbase=http://$IP:8088/ws/v1/cluster
if [[ $1 == *urlbase=?* ]] then
    urlbase=${1#*urlbase=}
    urlbase=${urlbase%%'&'*}
fi

for tool in $tools; do
    EXECUSER=$USER EXECPASS=$PASS $SHELL $VG_SSCOPESDIR/current/exec/${tool} \
        $aid $ip $TARGETTYPE \
    < /dev/null
done
