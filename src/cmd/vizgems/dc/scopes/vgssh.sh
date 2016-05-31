#!/bin/ksh

# wrapper for using ssh in scripts while using a password

if [[ $DISPLAY == fake:0 && $SSH_ASKPASS == *vgssh ]] then
    if [[ $SSHPASS == SWMUL*SWMUL ]] then
        pass=${SSHPASS#SWMUL}
        pn=0
        while [[ $pass == *SWMUL ]] do
            ps[$pn]=${pass%%SWMUL*}
            (( pn++ ))
            pass=${pass#*SWMUL}
        done
        pm=0
        [[ -f ssh.lastmulti ]] && pm=$(< ssh.lastmulti)
        pi=$pm
        if [[ -f ssh.prevmulti ]] then
            pi=$(< ssh.prevmulti)
            (( pi++ ))
        fi
        (( pi = pi % pn ))
        print -r -- ${ps[$pi]}
        print -- $pi > ssh.prevmulti
    else
        print $SSHPASS
    fi
    exit 0
fi

if [[ $SSHPASS != '' ]] then
    [[ $DISPLAY != fake:0 && $SSHPASS == SWMUL*SWMUL ]] && rm -f ssh.prevmulti
    export DISPLAY=fake:0 SSH_ASKPASS=vgssh
fi

args=
if [[ $SSHPORT != '' ]] then
    args+=" -p $SSHPORT"
fi
if [[ $SSHKEY != '' ]] then
    args+=" -i $SSHKEY"
fi

VGNEWSESSIONTIMEOUT=240 vgnewsession ssh \
    -o ConnectTimeout=${SSHTIMEOUT:-15} -o ServerAliveInterval=15 \
    -o ServerAliveCountMax=2 -o StrictHostKeyChecking=no $args \
"$@"

ret=$?
if [[ $ret == 0 && $SSHPASS == SWMUL*SWMUL && -f ssh.prevmulti ]] then
    mv ssh.prevmulti ssh.lastmulti
fi
exit $ret
