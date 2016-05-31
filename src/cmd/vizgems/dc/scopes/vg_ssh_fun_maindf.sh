df=()
typeset -A dfvs

function vg_ssh_fun_maindf_init {
    df.varn=0
    df.total=1 # do not emit summary inventory line
    return 0
}

function vg_ssh_fun_maindf_term {
    return 0
}

function vg_ssh_fun_maindf_add {
    typeset var=${as[var]} inst=${as[inst]}

    typeset -n dfr=df._${df.varn}
    dfr.name=$name
    dfr.unit=KB
    dfr.type=$type
    dfr.var=$var
    dfr.inst=$inst
    (( df.varn++ ))
    return 0
}

function vg_ssh_fun_maindf_send {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="df -l -k | egrep -v '^Filesystem|^[a-zA-Z]' | egrep '%'"
        ;;
    *solaris*)
        cmd="df -l -k | egrep -v '^Filesystem|^[a-rt-zA-Z]|^/proc|^/devices|^/platform/sun' | egrep '%'"
        ;;
    *darwin*)
        cmd="df -l -k | egrep -v '^Filesystem|^[a-zA-Z]' | egrep '%'"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_maindf_receive {
    typeset vn fs

    set -f
    set -A vs -- $1
    set +f
    vn=${#vs[@]}
    case $targettype in
    *darwin*)
        while (( vn >= 5 )) && [[ ${vs[$(( vn - 2 ))]} != *% ]] do
            vs[$(( vn - 2 ))]+=" ${vs[$(( vn - 1 ))]}"
            unset vs[$(( vn - 1 ))]
            (( vn-- ))
        done
        if [[ ${vs[$(( vn - 2 ))]} == *% && ${vs[$(( vn - 5 ))]} == *% ]] then
            vs[$(( vn - 4 ))]=${vs[$(( vn - 1 ))]}
            unset vs[$(( vn - 3 ))]
            unset vs[$(( vn - 2 ))]
            unset vs[$(( vn - 1 ))]
            (( vn -= 3 ))
        fi
        ;;
    esac
    fs=${vs[$(( vn - 1 ))]}
    dfvs[fs_total.$fs]=${vs[$(( vn - 5 ))]}
    dfvs[fs_used.$fs]=${vs[$(( vn - 4 ))]}
    dfvs[fs_free.$fs]=${vs[$(( vn - 3 ))]}
    (( dfvs[fs_total._total] += ${vs[$(( vn - 5 ))]} ))
    (( dfvs[fs_used._total] += ${vs[$(( vn - 4 ))]} ))
    (( dfvs[fs_free._total] += ${vs[$(( vn - 3 ))]} ))
    return 0
}

function vg_ssh_fun_maindf_emit {
    typeset dfi

    for (( dfi = 0; dfi < df.varn; dfi++ )) do
        typeset -n dfr=df._$dfi
        [[ ${dfvs[${dfr.var}.${dfr.inst}]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${dfr.name}
        vref.type=${dfr.type}
        vref.num=${dfvs[${dfr.var}.${dfr.inst}]}
        vref.unit=${dfr.unit}
        if [[ ${dfr.inst} == _total ]] then
            vref.label="FS Used (SUM)"
        else
            vref.label="FS Used (${dfr.inst})"
        fi
    done
    return 0
}

function vg_ssh_fun_maindf_invsend {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="df -l -k | egrep -v '^Filesystem|^[a-zA-Z]' | egrep '%'"
        ;;
    *solaris*)
        cmd="df -l -k | egrep -v '^Filesystem|^[a-rt-zA-Z]|^/proc|^/devices|^/platform/sun' | egrep '%'"
        ;;
    *darwin*)
        cmd="df -l -k | egrep -v '^Filesystem|^[a-zA-Z]' | egrep '%'"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_maindf_invreceive {
    typeset vn fs val fi

    set -f
    set -A vs -- $1
    set +f
    vn=${#vs[@]}
    case $targettype in
    *darwin*)
        while (( vn >= 5 )) && [[ ${vs[$(( vn - 2 ))]} != *% ]] do
            vs[$(( vn - 2 ))]+=" ${vs[$(( vn - 1 ))]}"
            unset vs[$(( vn - 1 ))]
            (( vn-- ))
        done
        if [[ ${vs[$(( vn - 2 ))]} == *% && ${vs[$(( vn - 5 ))]} == *% ]] then
            vs[$(( vn - 4 ))]=${vs[$(( vn - 1 ))]}
            unset vs[$(( vn - 3 ))]
            unset vs[$(( vn - 2 ))]
            unset vs[$(( vn - 1 ))]
            (( vn -= 3 ))
        fi
        ;;
    esac
    fs=${vs[$(( vn - 1 ))]}
    val=${vs[$(( vn - 5 ))]}
    if [[ ${df.total} == '' ]] then
        print "node|o|$aid|si_fs_total|_total"
        df.total=y
    fi
    fi=$(print "$fs" | sum -x md5)
    fi=${fi%%' '*}
    print "node|o|$aid|si_fs$fi|$fs"
    vg_unitconv "$val" KB GB
    val=$vg_ucnum
    if [[ $val != '' ]] then
        print "node|o|$aid|si_sz_fs_used.$fi|$val"
    fi
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_maindf_init vg_ssh_fun_maindf_term
    typeset -ft vg_ssh_fun_maindf_add vg_ssh_fun_maindf_send
    typeset -ft vg_ssh_fun_maindf_receive vg_ssh_fun_maindf_emit
    typeset -ft vg_ssh_fun_maindf_invsend vg_ssh_fun_maindf_invreceive
fi
