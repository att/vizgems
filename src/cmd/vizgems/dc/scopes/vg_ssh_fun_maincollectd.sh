collectd=( varn=0 typeset -A vars n2vs )

function vg_ssh_fun_maincollectd_init {
    collectd.socket=$COLLECTD_SOCKET
    collectd.host=$COLLECTD_HOST
    return 0
}

function vg_ssh_fun_maincollectd_term {
    return 0
}

function vg_ssh_fun_maincollectd_add {
    collectd.vars[${collectd.varn}]=(
        name=$name type=$type unit=${as[unit]}
        var=${as[var]} label=${as[label]}
    )
    collectd.n2vs[$name]=${collectd.varn}
    (( collectd.varn++ ))
    collectd.socket=${as[socket]}
    collectd.host=${as[host]}

    return 0
}

function vg_ssh_fun_maincollectd_send {
    typeset args vari

    if [[ ${collectd.socket} != '' ]] then
        args+=" -s ${collectd.socket}"
    fi
    for (( vari = 0; vari < ${collectd.varn}; vari++ )) do
        typeset -n r=collectd.vars[$vari]
        print -r "collectdctl $args getval ${collectd.host}/${r.var} | sed \"s!^!XYZ${r.name}ZYX !\""
    done

    return 0
}

function vg_ssh_fun_maincollectd_receive {
    typeset vs vn name vari kv k v
    typeset -F3 v

    set -f
    set -A vs -- $1
    set +f
    vn=${#vs[@]}

    name=${vs[0]}
    [[ $name != XYZ*ZYX ]] && return
    name=${name#XYZ}
    name=${name%ZYX}
    vari=${collectd.n2vs[$name]}
    [[ $vari == '' ]] && return

    typeset -n r=collectd.vars[$vari]

    kv=${vs[1]}
    [[ $kv != *=* ]] && return

    k=${kv%%=*}
    v=${kv#*=}
    [[ $k != value && $name != *_$k ]] && return

    r.val=$v
    r.haveval=1

    return 0
}

function vg_ssh_fun_maincollectd_emit {
    typeset vari

    for (( vari = 0; vari < ${collectd.varn}; vari++ )) do
        typeset -n r=collectd.vars[$vari]
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${r.name}
        vref.type=${r.type}
        vref.num=${r.val}
        vref.unit=${r.unit}
        vref.label="${r.label}"
    done

    return 0
}

function vg_ssh_fun_maincollectd_invsend {
    typeset args

    if [[ ${collectd.socket} != '' ]] then
        args+=" -s ${collectd.socket}"
    fi
    print -r "collectdctl $args listval | while read i; do collectdctl $args getval \$i | sed \"s!^!XYZ\${i}ZYX !\"; done"

    return 0
}

function vg_ssh_fun_maincollectd_invreceive {
    typeset vs vn hv host var label kv k v
    typeset -u u

    set -f
    set -A vs -- $1
    set +f
    vn=${#vs[@]}

    hv=${vs[0]}
    [[ $hv != XYZ*ZYX ]] && return
    hv=${hv#XYZ}
    hv=${hv%ZYX}
    host=${hv%%/*}
    var=${hv#*/}
    [[ $host != ${collectd.host} ]] && return

    cdvar=$var
    for w in ${var//@([-\/])/ \1 }; do
        [[ $w == '-' ]] && continue
        u=${w:0:1}
        label+=" $u${w:1}"
    done
    label=${label#' '}
    var=${var//[!-a-zA-Z0-9_\/]/}
    var=${var//-/_}
    var=${var//\//.}

    kv=${vs[1]}
    [[ $kv != *=* ]] && return

    k=${kv%%=*}
    v=${kv#*=}
    if [[ $k == value ]] then
        print "node|o|$aid|si_cdvar$var|$cdvar"
        print "node|o|$aid|si_cdlabel$var|$label"
    else
        print "node|o|$aid|si_cdvar${var}_$k|$cdvar"
        print "node|o|$aid|si_cdlabel${var}_$k|$label ($k)"
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_maincollectd_init vg_ssh_fun_maincollectd_term
    typeset -ft vg_ssh_fun_maincollectd_add vg_ssh_fun_maincollectd_send
    typeset -ft vg_ssh_fun_maincollectd_receive vg_ssh_fun_maincollectd_emit
    typeset -ft vg_ssh_fun_maincollectd_invsend vg_ssh_fun_maincollectd_invreceive
fi
