
dq_dt_inv_data=(
    deps='' rerun=''
    invnodefilters=() invnodefiltern=0
    invedgefilters=() invedgefiltern=0
    appendfilterfiles='' runinparallel=n
    filterstr='' keepfilterstr='' label='' tryasid=n rounds=1
    kvfilters=() kvfiltern=0
)

function dq_dt_inv_init { # $1 = dt query prefix
    typeset dtstr=$1
    typeset -n dt=$dtstr
    typeset -n infilters=dq_dt_inv_data.invnodefilters
    typeset -n infiltern=dq_dt_inv_data.invnodefiltern
    typeset infilteri
    typeset -n iefilters=dq_dt_inv_data.invedgefilters
    typeset -n iefiltern=dq_dt_inv_data.invedgefiltern
    typeset iefilteri
    typeset -n filterstr=dq_dt_inv_data.filterstr
    typeset -n keepfilterstr=dq_dt_inv_data.keepfilterstr
    typeset -n keeplevels=dq_main_data.query.args.invkeeplevels
    typeset -n kvfilters=dq_dt_inv_data.kvfilters
    typeset -n kvfiltern=dq_dt_inv_data.kvfiltern
    typeset kvfilteri
    typeset ifs line obj level id vs vi vii vil vir v s1 s2 rdir
    typeset fgroup param t attr key

    ifs="$IFS"
    IFS=':'
    $SHELL vg_accountinfo $SWMID | while read -r line; do
        [[ $line != vg_level_* ]] && continue
        level=${line%%=*}
        vs=${line##"$level"?(=)}
        if [[ :$vs: != *:__ALL__:* ]] then
            s1= s2=
            for vi in $vs; do
                s1+="|$vi"
                if [[ $STRICTEDGEUI == y ]] then
                    s2+="|$vi${VG_EDGESEP}$vi"
                else
                    s2+="|$vi${VG_EDGESEP}.*|.*${VG_EDGESEP}$vi"
                fi
            done
            s1=${s1#\|} s2=${s2#\|}
            typeset -n infilter=dq_dt_inv_data.invnodefilters._$infiltern
            infilter=(t=F k="${level#*level_}" v="${s1}")
            (( infiltern++ ))
            typeset -n iefilter=dq_dt_inv_data.invedgefilters._$iefiltern
            iefilter=(t=F k="${level#*level_}" v="${s2}")
            (( iefiltern++ ))
        fi
    done
    IFS="$ifs"

    if [[ ${dt.args.runinparallel} == y ]] then
        dq_dt_inv_data.runinparallel=y
    else
        dq_dt_inv_data.runinparallel=n
    fi
    dq_dt_inv_data.rounds=${dt.args.rounds:-1}

    set -f
    ifs="$IFS"
    IFS='|'
    if [[ ${vars.tryasid} != '' ]] then
        dq_dt_inv_data.tryasid=${vars.tryasid}
    fi
    if [[ ${vars.obj} != '' ]] then
        obj=${vars.obj}
        obj=${obj#*:}
        level=${obj%%:*}
        id=${obj#*:}
        typeset -n infilter=dq_dt_inv_data.invnodefilters._$infiltern
        infilter=(t=I k="${level}" v="${id}")
        (( infiltern++ ))
        typeset -n iefilter=dq_dt_inv_data.invedgefilters._$iefiltern
        iefilter=(
            t=I k="${level}" v="${id}${VG_EDGESEP}.*|.*${VG_EDGESEP}${id}"
        )
        (( iefiltern++ ))
    fi
    for level in "${!vars.level_@}"; do
        level=${level#vars.level_}
        typeset -n vs=vars.level_$level
        if (( ${#vs[@]} > 0 )) && [[ ${vs[0]} != '' ]] then
            s1= s2=
            for vi in "${vs[@]}"; do
                for vii in $vi; do
                    if [[ $vii == *${VG_EDGESEP}* ]] then
                        vil=${vii%%${VG_EDGESEP}*}
                        vir=${vii##*${VG_EDGESEP}}
                        if [[ $vil != '.*' ]] then
                            s1+="|$vil"
                        fi
                        if [[ $vir != '.*' ]] then
                            s1+="|$vir"
                        fi
                        s2+="|$vii"
                    else
                        s1+="|$vii"
                        vil="${vii}${VG_EDGESEP}.*"
                        vir=".*${VG_EDGESEP}${vii}"
                        s2+="|$vil|$vir"
                    fi
                done
            done
            s1=${s1#\|} s2=${s2#\|}
            typeset -n infilter=dq_dt_inv_data.invnodefilters._$infiltern
            infilter=(t=I k="$level" v="${s1}")
            (( infiltern++ ))
            typeset -n iefilter=dq_dt_inv_data.invedgefilters._$iefiltern
            iefilter=(t=I k="$level" v="${s2}")
            (( iefiltern++ ))
        fi
    done
    for level in "${!vars.lname_@}"; do
        level=${level#vars.lname_}
        typeset -n vs=vars.lname_$level
        if (( ${#vs[@]} > 0 )) && [[ ${vs[0]} != '' ]] then
            s1= s2=
            for vi in "${vs[@]}"; do
                if [[ $vi == *${VG_EDGESEP}* ]] then
                    vil=${vi%%${VG_EDGESEP}*}
                    vir=${vi##*${VG_EDGESEP}}
                    if [[ $vil != '.*' ]] then
                        s1+="|$vil"
                    fi
                    if [[ $vir != '.*' ]] then
                        s1+="|$vir"
                    fi
                    s2+="|$vi"
                else
                    s1+="|$vi"
                    vil="${vi}${VG_EDGESEP}.*"
                    vir=".*${VG_EDGESEP}${vi}"
                    s2+="|$vil|$vir"
                fi
            done
            s1=${s1#\|} s2=${s2#\|}
            typeset -n infilter=dq_dt_inv_data.invnodefilters._$infiltern
            infilter=(t=N k="$level" v="${s1}")
            (( infiltern++ ))
            typeset -n iefilter=dq_dt_inv_data.invedgefilters._$iefiltern
            iefilter=(t=N k="$level" v="${s2}")
            (( iefiltern++ ))
        fi
    done
    for attr in "${!vars.attr_@}"; do
        key=${attr#vars.attr_}
        typeset -n vs=vars.attr_$key
        if (( ${#vs[@]} > 0 )) && [[ ${vs[0]} != '' ]] then
            s1=
            for vi in "${vs[@]}"; do
                s1+="|$vi"
            done
            s1=${s1#\|}
            typeset -n kvfilter=dq_dt_inv_data.kvfilters._$kvfiltern
            kvfilter=(t=A k="$key" v="${s1}")
            (( kvfiltern++ ))
        fi
    done
    for fgroup in onempty always; do
        if [[ $fgroup == onempty ]] && (( infiltern > 0 || iefiltern > 0 )) then
            continue
        fi
        typeset -n params=$dtstr.filters.$fgroup
        for param in "${!params.@}"; do
            if [[ $param == *.level_* ]] then
                level=${param##*.level_}
                t=I
            elif [[ $param == *.lname_* ]] then
                level=${param##*.lname_}
                t=N
            fi
            typeset -n vs=$param
            if (( ${#vs[@]} <= 0 )) || [[ ${vs[0]} == '' ]] then
                continue
            fi
            s1= s2=
            for vi in "${vs[@]}"; do
                for vii in $vi; do
                    if [[ $vii == *${VG_EDGESEP}* ]] then
                        vil=${vii%%${VG_EDGESEP}*}
                        vir=${vii##*${VG_EDGESEP}}
                        if [[ $vil != '.*' ]] then
                            s1+="|$vil"
                        fi
                        if [[ $vir != '.*' ]] then
                            s1+="|$vir"
                        fi
                        s2+="|$vii"
                    else
                        s1+="|$vii"
                        vil="${vii}${VG_EDGESEP}.*"
                        vir=".*${VG_EDGESEP}${vii}"
                        s2+="|$vil|$vir"
                    fi
                done
            done
            s1=${s1#\|} s2=${s2#\|}
            found=n
            for (( infilteri = 0; infilteri < infiltern; infilteri++ )) do
                typeset -n infilter=dq_dt_inv_data.invnodefilters._$infilteri
                [[ ${infilter.k} != $level ]] && continue
                infilter.v+="|$s1"
                found=y
            done
            if [[ $found == n ]] then
                typeset -n infilter=dq_dt_inv_data.invnodefilters._$infiltern
                infilter=(t=$t k="$level" v="${s1}")
                (( infiltern++ ))
            fi
            found=n
            for (( iefilteri = 0; iefilteri < iefiltern; iefilteri++ )) do
                typeset -n iefilter=dq_dt_inv_data.invedgefilters._$iefilteri
                [[ ${iefilter.k} != $level ]] && continue
                iefilter.v+="|$s2"
                found=y
            done
            if [[ $found == n ]] then
                typeset -n iefilter=dq_dt_inv_data.invedgefilters._$iefiltern
                iefilter=(t=$t k="$level" v="${s2}")
                (( iefiltern++ ))
            fi
        done
    done
    IFS="$ifs"
    set +f

    rdir=${dq_main_data.rdir}

    for (( infilteri = 0; infilteri < infiltern; infilteri++ )) do
        typeset -n infilter=dq_dt_inv_data.invnodefilters._$infilteri
        v=$(printf '%#H' "${infilter.v}")
        case ${infilter.t} in
        I)
            filterstr+="&level_${infilter.k}=$v"
            if [[ " ${keeplevels} " == *' '${infilter.k}' '* ]] then
                keepfilterstr+="&level_${infilter.k}=$v"
            fi
            ;;
        N)
            filterstr+="&lname_${infilter.k}=$v"
            ;;
        esac
    done
    filterstr=${filterstr#'&'}
    keepfilterstr=${keepfilterstr#'&'}

    LEVELMAPFILE=$rdir/LEVELS-maps.dds
    INVMAPFILE=$rdir/inv-maps-uniq.dds
    INVNODEATTRFILE=$rdir/inv-nodes.dds
    INVEDGEATTRFILE=$rdir/inv-edges.dds
    INVNODENAMEATTRFILE=$rdir/inv-nodes.dds
    if [[ -f $rdir/inv.uniquebyname ]] then
        INVNODENAMEATTRFILE=$rdir/inv-nodes-uniq.dds
    fi
    export LEVELMAPFILE INVMAPFILE INVNODEATTRFILE INVEDGEATTRFILE
    export INVNODENAMEATTRFILE

    return 0
}

function dq_dt_inv_appendfilters {
# $1 = dt query prefix $rest = filter file names
# optional flag -kF to keep F filters
    if [[ $1 == -kF ]] then
        typeset keepf=y
        shift 1
    else
        typeset keepf=n
    fi
    typeset -n dt=$1; shift 1
    typeset files="$@"
    typeset -n infilters=dq_dt_inv_data.invnodefilters
    typeset -n infiltern=dq_dt_inv_data.invnodefiltern
    typeset -n iefilters=dq_dt_inv_data.invedgefilters
    typeset -n iefiltern=dq_dt_inv_data.invedgefiltern
    typeset infilteri iefilteri
    typeset -A ns es
    typeset ifs file line ls level val

    ifs="$IFS"
    IFS='|'
    for file in ${files//' '/'|'}; do
        set -f
        sort -u $file | while read -r line; do
            set -A ls -- $line
            [[ ${ls[0]} != @(I|F) ]] && continue
            level=${ls[1]}
            val=${ls[2]}
            case $val in
            *$VG_EDGESEP*)
                es[$level]+="|$val"
                ;;
            *)
                ns[$level]+="|$val"
                ;;
            esac
        done
        set +f
    done

    for (( infilteri = 0; infilteri < infiltern; infilteri++ )) do
        typeset -n infilter=dq_dt_inv_data.invnodefilters._$infilteri
        [[ ${infilter.t} == F && $keepf == y ]] && continue
        infilter.t=X
    done
    for (( iefilteri = 0; iefilteri < iefiltern; iefilteri++ )) do
        typeset -n iefilter=dq_dt_inv_data.invedgefilters._$iefilteri
        [[ ${iefilter.t} == F && $keepf == y ]] && continue
        iefilter.t=X
    done

    for level in "${!ns[@]}"; do
        ns[$level]=${ns[$level]#\|}
        typeset -n infilter=dq_dt_inv_data.invnodefilters._$infiltern
        infilter=(t=I k="${level}" v="${ns[$level]}")
        (( infiltern++ ))
    done
    for level in "${!es[@]}"; do
        es[$level]=${es[$level]#\|}
        typeset -n iefilter=dq_dt_inv_data.invedgefilters._$iefiltern
        iefilter=(t=I k="${level}" v="${es[$level]}")
        (( iefiltern++ ))
    done
    IFS="$ifs"

    return 0
}

function dq_dt_inv_run { # $1 = dt query prefix
    typeset -n dt=$1
    typeset -n infiltern=dq_dt_inv_data.invnodefiltern
    typeset -n iefiltern=dq_dt_inv_data.invedgefiltern
    typeset -n kvfiltern=dq_dt_inv_data.kvfiltern
    typeset infilteri iefilteri kvfilteri
    typeset rdir argi res i rounds altkvfiltern

    rdir=${dq_main_data.rdir}
    export INVEXACTLEVELMODE=0
    if [[ ${dq_dt_inv_data.appendfilterfiles} != '' ]] then
        dq_dt_inv_appendfilters $1 ${dq_dt_inv_data.appendfilterfiles}
        dq_dt_inv_data.appendfilterfiles=
        export INVEXACTLEVELMODE=1
    fi

    export INVNODEFILTERFILE=invnode.filter INVNODEFILTERSIZE=$infiltern
    export INVEDGEFILTERFILE=invedge.filter INVEDGEFILTERSIZE=$iefiltern
    export INVATTRFILE=inv.attr
    export KEYVALUEFILTERFILE=keyvalue.filter KEYVALUEFILTERSIZE=$kvfiltern

    rm -f $INVNODEFILTERFILE $INVEDGEFILTERFILE $INVATTRFILE
    rm -f inode*.dds iedge*.dds inv.dds

    (( infiltern + iefiltern == 0 )) && return 0

    (
        print -r -- "edgesep=$VG_EDGESEP"
        for argi in "${!dt.args.@}"; do
            typeset -n arg=$argi
            print -r -- "${argi##*.}=$arg"
        done
    ) > $INVATTRFILE

    for (( i = 0; i < 2; i++ )) do
        [[ $i == 1 && ${dq_dt_inv_data.tryasid} != y ]] && break
        for (( infilteri = 0; infilteri < infiltern; infilteri++ )) do
            typeset -n infilter=dq_dt_inv_data.invnodefilters._$infilteri
            if [[ ${infilter.t} == X ]] then
                (( INVNODEFILTERSIZE-- ))
                continue
            fi
            if [[ $i == 1 && ${infilter.t} == N ]] then
                print -r "I|${infilter.k}|${infilter.v}"
            else
                print -r "${infilter.t}|${infilter.k}|${infilter.v}"
            fi
        done > $INVNODEFILTERFILE
        for (( kvfilteri = 0; kvfilteri < kvfiltern; kvfilteri++ )) do
            typeset -n kvfilter=dq_dt_inv_data.kvfilters._$kvfilteri
            if [[ ${kvfilter.t} == X ]] then
                (( KEYVALUEFILTERSIZE-- ))
                continue
            fi
            print -r "${kvfilter.k}|${kvfilter.v}"
        done > $KEYVALUEFILTERFILE
        altkvfiltern=$KEYVALUEFILTERSIZE
        [[ ${dt.args.domain} == edge ]] && KEYVALUEFILTERSIZE=0
        if [[ ${dq_dt_inv_data.runinparallel} == n ]] then
            PARALLELMODE=0 \
            ddsconv -os vg_dq_dt_inv.schema -cso vg_dq_dt_inv_node.conv.so \
                $rdir/inv-nodes-uniq.dds \
            | ddssort $UIZFLAG -fast -u -ke 'cat type level1 id1' > inode.dds
        else
            ddsinfo $rdir/inv-nodes-uniq.dds | fgrep nrecs: | read j recn
            if (( recn < 100000 )) then
                PARALLELMODE=0 \
                ddsconv -os vg_dq_dt_inv.schema -cso vg_dq_dt_inv_node.conv.so \
                    $rdir/inv-nodes-uniq.dds \
                | ddssort $UIZFLAG \
                    -fast -u -ke 'cat type level1 id1' \
                > inode.dds
            else
                (
                    for (( reci = 0; reci < recn; reci += 100000 )) do
                        PARALLELMODE=1 RECI=$reci RECN=100000 \
                        ddsconv -os vg_dq_dt_inv.schema \
                            -cso vg_dq_dt_inv_node.conv.so \
                            $rdir/inv-nodes-uniq.dds \
                        | ddssort $UIZFLAG -fast -u -ke 'cat type level1 id1' \
                        > inode_$reci.dds &
                    done
                    wait
                    ddssort $UIZFLAG \
                        -fast -m -u -ke 'cat type level1 id1' inode_*.dds \
                    > inode.dds
                )
            fi
        fi
        res=inode.dds

        if [[ ${dt.args.domain} == edge ]] then
            KEYVALUEFILTERSIZE=$altkvfiltern
            for (( iefilteri = 0; iefilteri < iefiltern; iefilteri++ )) do
                typeset -n iefilter=dq_dt_inv_data.invedgefilters._$iefilteri
                if [[ ${iefilter.t} == X ]] then
                    (( INVEDGEFILTERSIZE-- ))
                    continue
                fi
                if [[ $i == 1 && ${infilter.t} == N ]] then
                    print -r "I|${iefilter.k}|${iefilter.v}"
                else
                    print -r "${iefilter.t}|${iefilter.k}|${iefilter.v}"
                fi
            done > $INVEDGEFILTERFILE
            INVOUTFILE=inode.dds \
            ddsconv -os vg_dq_dt_inv.schema -cso vg_dq_dt_inv_edge.conv.so \
                $rdir/inv-edges-uniq.dds \
            | ddssort $UIZFLAG \
                -fast -u -ke 'cat type level1 id1 level2 id2' \
            > iedge.dds
            res=iedge.dds
        fi
        ln -fs $res inv.dds
        [[ $(ddsinfo inv.dds 2> /dev/null) != *'nrecs: 0'* ]] && break
    done

    if (( dq_dt_inv_data.rounds > 1 )) then
        rounds=${dq_dt_inv_data.rounds}
        dq_dt_inv_data.rounds=1
        while (( rounds > 1 )) do
            ddsfilter -osm none -fso vg_dq_dt_inv2invfilter.filter.so inv.dds \
            > invinv.filter
            [[ ! -s invinv.filter ]] && break
            dq_dt_inv_appendfilters -kF $1 invinv.filter
            export INVEXACTLEVELMODE=1
            dq_dt_inv_run dq_main_data.query.dtools.inv
            (( rounds-- ))
        done
    fi

    return 0
}

function dq_dt_inv_emitlabel { # $1 = dt query prefix
    typeset -n dt=$1
    typeset -n infiltern=dq_dt_inv_data.invnodefiltern
    typeset infilteri

    for (( infilteri = 0; infilteri < infiltern; infilteri++ )) do
        typeset -n infilter=dq_dt_inv_data.invnodefilters._$infilteri
        dq_dt_inv_data.label+=" ${infilter.k}:${infilter.v}"
    done
    dq_dt_inv_data.label=${dq_dt_inv_data.label#' '}

    return 0
}

function dq_dt_inv_emitparams { # $1 = dt query prefix
    return 0
}

function dq_dt_inv_setup {
    dq_main_data.query.dtools.inv=(args=())
    typeset -n ir=dq_main_data.query.dtools.inv.args
    typeset -n qdir=querydata.dt_inv

    ir.poutlevel=${qdir.poutlevel}
    ir.soutlevel=${qdir.soutlevel}
    if [[ ${ir.poutlevel} == '' ]] then
        print -u2 "VG QUERY ERROR: poutlevel missing"
        return 1
    fi
    ir.domain=node
    if [[ ${ir.soutlevel} != '' ]] then
        ir.domain=edge
    fi
    ir.runinparallel=${qdir.runinparallel:-n}
    ir.rounds=${qdir.rounds:-1}

    if [[ ${qdir.filters} != '' ]] then
        typeset -C dq_main_data.query.dtools.inv.filters=qdir.filters
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_dt_inv_init dq_dt_inv_run dq_dt_inv_setup
    typeset -ft dq_dt_inv_appendfilters
    typeset -ft dq_dt_inv_emitlabel dq_dt_inv_emitparams
fi
