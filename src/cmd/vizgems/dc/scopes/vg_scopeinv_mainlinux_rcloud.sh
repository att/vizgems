
export cid=$CID aid=$AID ip=$IP user=$USER pass=$PASS cs=$CS
export targettype=$TARGETTYPE scopetype=$SCOPETYPE servicelevel=$SERVICELEVEL

set -o pipefail

. vg_units

case $targettype in
*)
    export PROC_FILES='stat meminfo'
    tools='maindf mainnetstat mainproc mainuptime mainvmstat mainmegacli mainnvidia mainrcloud'
    ;;
esac

export SSHPASS=$pass
unset SSHKEY
if [[ $pass == sshkey=* ]] then
    print -- "${pass#*=}" > ssh.key
    chmod 600 ssh.key
    export SSHKEY=./ssh.key
    unset SSHPASS
fi

for tool in $tools; do
    . $VG_SSCOPESDIR/current/ssh/vg_ssh_fun_${tool}
    vg_ssh_fun_${tool}_init
done
for tool in $tools; do
    vg_ssh_fun_${tool}_invsend | sed "s!\$! | sed 's/^/A${tool}Z/'!"
done | vgssh $user@$ip 2> /dev/null | while read -r line; do
    [[ $line != A*Z* ]] && continue
    tool=${line%%Z*}
    tool=${tool#A}
    rest=${line##A"$tool"Z}
    vg_ssh_fun_${tool}_invreceive "$rest"
done
