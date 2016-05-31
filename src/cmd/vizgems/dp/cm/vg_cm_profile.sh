
vg_cm_profile_valuefields=()
vg_cm_profile_valuerec=
function vg_cm_profile_valueunpack {
    typeset -A keys
    typeset rest vorc minexpr maxexpr sev min per max ival v1 f1 v2 f2 n i

    vg_cm_profile_valuefields.keys=''
    [[ $vg_cm_profile_valuerec == '' ]] && return 0

    rest=$vg_cm_profile_valuerec
    if [[ $rest != *CLEAR:* ]] then
        rest="$rest:CLEAR:5:1:1"
    fi
    while [[ $rest != '' ]] do
        vorc=${rest%%:*}
        rest=${rest##"$vorc"?(:)}
        case $vorc in
        VR)
            f1= v1= f2= v2=
            f1=${rest%%:*}
            rest=${rest##"$f1"?(:)}
            v1=${rest%%:*}
            rest=${rest##"$v1"?(:)}
            f2=${rest%%:*}
            rest=${rest##"$f2"?(:)}
            v2=${rest%%:*}
            rest=${rest##"$v2"?(:)}
            [[ $f1 != @([nie]) || $f2 != @([nie]) ]] && return 1
            typeset -n x=vg_cm_profile_valuefields.vr
            x.f1=$f1
            x.v1=$v1
            x.f2=$f2
            x.v2=$v2
            keys[vr]=1
            ;;
        CLEAR)
            sev=${rest%%:*}
            rest=${rest##"$sev"?(:)}
            min=${rest%%:*}
            rest=${rest##"$min"?(:)}
            per=${rest%%:*}
            rest=${rest##"$per"?(:)}
            typeset -n x=vg_cm_profile_valuefields.clear
            x.sev=$sev
            x.min=$min
            x.per=$per
            keys[clear]=1
            ;;
        SIGMA|ACTUAL)
            f1= v1= f2= v2=
            f1=${rest%%:*}
            rest=${rest##"$f1"?(:)}
            v1=${rest%%:*}
            rest=${rest##"$v1"?(:)}
            f2=${rest%%:*}
            rest=${rest##"$f2"?(:)}
            v2=${rest%%:*}
            rest=${rest##"$v2"?(:)}
            sev=${rest%%:*}
            rest=${rest##"$sev"?(:)}
            min=${rest%%:*}
            rest=${rest##"$min"?(:)}
            per=${rest%%:*}
            rest=${rest##"$per"?(:)}
            max=${rest%%:*}
            rest=${rest##"$max"?(:)}
            ival=${rest%%:*}
            rest=${rest##"$ival"?(:)}
            [[ $f1 != @([nie]) || $f2 != @([nie]) ]] && return 1
            typeset -n x=vg_cm_profile_valuefields.sev$sev
            x.mode=$vorc
            x.f1=$f1
            x.v1=$v1
            x.f2=$f2
            x.v2=$v2
            x.sev=$sev
            x.min=$min
            x.per=$per
            x.max=$max
            x.ival=$ival
            keys[sev$sev]=1
            ;;
        *)
            return 1
            ;;
        esac
    done
    vg_cm_profile_valuefields.keys=${!keys[@]}

    return 0
}

function vg_cm_profile_valuepack {
    typeset s key

    vg_cm_profile_valuerec=''
    [[ ${vg_cm_profile_valuefields.keys} == '' ]] && return 0

    s=
    if [[ " ${vg_cm_profile_valuefields.keys} " == *' 'vr' '* ]] then
        typeset -n x=vg_cm_profile_valuefields.clear
        s+="VR:${x.f1}:${x.v1}:${x.f2}:${x.v2}"
    fi
    for key in ${vg_cm_profile_valuefields.keys}; do
        [[ $key != sev* ]] && continue

        typeset -n x=vg_cm_profile_valuefields.$key
        if [[ ${x.v1} == '' && ${x.v2} == '' ]] then
            return 1
        fi
        [[ $s != '' ]] && s+=':'
        s+="${x.mode}:${x.f1}:${x.v1}:${x.f2}:${x.v2}"
        s+=":${x.sev}:${x.min}:${x.per}:${x.max}:${x.ival}"
    done
    if [[ " ${vg_cm_profile_valuefields.keys} " == *' 'clear' '* ]] then
        typeset -n x=vg_cm_profile_valuefields.clear
        [[ $s != '' ]] && s+=':'
        s+="CLEAR:${x.sev}:${x.min}:${x.per}"
    fi
    vg_cm_profile_valuerec=$s

    return 0
}

function vg_cm_profile_valuecheck {
    typeset n key sev ret nu='+([0-9.-])' nup='+([0-9.%-])'

    if [[ ${vg_cm_profile_valuefields.keys} == '' ]] then
        print "ERROR|empty spec"
        return 0
    fi

    n=0
    ret=0
    if [[ " ${vg_cm_profile_valuefields.keys} " == *' 'vr' '* ]] then
        typeset -n x=vg_cm_profile_valuefields.vr
        if [[ ${x.v1} != '' && ${x.v2} != '' ]] then
            if (( ${x.v1} > ${x.v2} )) then
                print "WARNING|min > max for severity $sev"
                ret=1
            fi
        fi
    fi
    for key in ${vg_cm_profile_valuefields.keys}; do
        [[ $key == @(vr|clear) ]] && continue
        (( n++ ))

        typeset -n x=vg_cm_profile_valuefields.$key
        sev=${key#sev}
        if (( sev < 1 || sev > 4 )) then
            print "ERROR|unknown severity $sev"
        fi
        if [[ ${x.f1} != @(|n|i|e) || ${x.f2} != @(|n|i|e) ]] then
            print "ERROR|range type fields incorrect for severity $sev"
        fi
        if [[ ${x.v1} != @(|$nup) || ${x.v2} != @(|$nup) ]] then
            print "ERROR|value fields not numeric (or empty) for severity $sev"
        elif [[ ${x.v1} == '' && ${x.v2} == '' ]] then
            print "ERROR|value fields empty for severity $sev"
        elif [[ ${x.v1} != '' && ${x.v2} != '' ]] then
            if (( ${x.v1%'%'} > ${x.v2%'%'} )) then
                print "WARNING|min > max for severity $sev"
            fi
        fi
        if [[ ${x.min} != $nu || ${x.max} != $nu || ${x.per} != $nu ]] then
            print "ERROR|num of matches fields not numeric for severity $sev"
        elif (( ${x.min} > ${x.per} )) then
            x.per=${x.min}
            print "WARNING|first alarm matches changed to ${x.min}/${x.per}"
            ret=1
        fi
        if [[ ${x.ival} != $nu ]] then
            print "ERROR|interval field not numeric for severity $sev"
        elif (( ${x.ival} != 3600 )) then
            (( x.max = (3600 / ${x.ival}) * ${x.max} ))
            x.ival=3600
            print "WARNING|repeat alarm spec changed to ${x.max}/${x.ival}"
            ret=1
            if (( ${x.max} > 4 )) then
                x.max=4
                print "WARNING|repeat alarm count changed to ${x.max}"
                ret=1
            fi
        fi
        if [[ ${x.mode} != @(VR|SIGMA|ACTUAL) ]] then
            print "ERROR|mode field incorrect for severity $sev"
        fi
    done
    if (( n < 1 )) then
        print "WARNING|no severity selected"
    fi
    if [[ " ${vg_cm_profile_valuefields.keys} " == *' 'vr' '* ]] then
        typeset -n x=vg_cm_profile_valuefields.vr
        if [[ ${x.f1} != @(|n|i|e) || ${x.f2} != @(|n|i|e) ]] then
            print "ERROR|VR range type fields incorrect"
        fi
        if [[ ${x.v1} != @(|$nup) || ${x.v2} != @(|$nup) ]] then
            print "ERROR|VR value fields not numeric (or empty)"
        elif [[ ${x.v1} != '' && ${x.v2} != '' ]] then
            if (( ${x.v1%'%'} > ${x.v2%'%'} )) then
                print "WARNING|VR min > max"
            fi
        fi
    fi
    if [[ " ${vg_cm_profile_valuefields.keys} " != *' 'clear' '* ]] then
        print "ERROR|no clear data specified"
    else
        typeset -n x=vg_cm_profile_valuefields.clear
        if [[ ${x.min} != $nu || ${x.per} != $nu ]] then
            print "ERROR|num of matches fields not numeric for clear"
        elif (( ${x.min} > ${x.per} )) then
            x.per=${x.min}
            print "WARNING|clear alarm matches changed to ${x.min}/${x.per}"
            ret=1
        fi
        if [[ ${x.sev} != [12345] ]] then
            print "ERROR|severity field incorrect for clear"
        fi
    fi

    return $ret
}

function vg_cm_profile_check {
    typeset spec=$1

    typeset -A ofs nfs ts
    typeset kvi str kv k v g err i ire ore rw x l
    typeset assetn assetl asset rest priv noprivs

    rw=n
    eval $(vgxml -topmark edit -ksh "$spec")
    case ${edit.a} in
    remove)
        v=${edit.o.r.l//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        print -r "OLD RECORD|$v"
        typeset -n x=edit.o.r
        if [[ $EDITALL != 1 && ${x.f_4} != *'|'@($SWMID|$SWMCMID) ]] then
            print "ERROR|Account IDs do not match - cannot remove record"
            return
        fi
        print -r "REC|$spec"
        return
        ;;
    modify)
        v=${edit.o.r.l//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        print -r "OLD RECORD|$v"
        v=${edit.n.r.l//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        print -r "NEW RECORD|$v"
        typeset -n x=edit.o.r
        if [[ $EDITALL != 1 && ${x.f_4} != *'|'@($SWMID|$SWMCMID) ]] then
            print "ERROR|Account IDs do not match - cannot modify record"
            return
        fi
        typeset -n x=edit.n.r
        if [[ ${x.f_4} != "4|"@($SWMID|$SWMCMID) ]] then
            if [[ $SWMCMID != '' ]] then
                x.f_4="4|$SWMCMID"
            else
                x.f_4="4|$SWMID"
            fi
            rw=y
        fi
        # continue with new record check
        ;;
    insert)
        v=${edit.n.r.l//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        print -r "NEW RECORD|$v"
        typeset -n x=edit.n.r
        if [[ ${x.f_4} != "4|"@($SWMID|$SWMCMID) ]] then
            if [[ $SWMCMID != '' ]] then
                x.f_4="4|$SWMCMID"
            else
                x.f_4="4|$SWMID"
            fi
            rw=y
        fi
        # continue with new record check
        ;;
    *)
        print "ERROR|unknown action ${edit.a}"
        return
        ;;
    esac

    for (( kvi = 0; kvi < 5; kvi++ )) do
        if (( kvi == 0 )) then
            str=f
        else
            str=f_$kvi
        fi
        typeset -n kv=edit.n.r.$str
        k=${kv%%'|'*}
        v=${kv#"$k"'|'}
        v=${v//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        ofs[$kvi]=$v
        nfs[$kvi]=$v
        [[ ${ofs[$kvi]} != ${nfs[$kvi]} ]] && rw=y
    done

    # check asset field
    if [[ ${nfs[0]} != '' ]] then
        err=''
        print "" | egrep -S -- "${nfs[0]}" 2>&1 | read err
        err=${err#egrep:\ }
        if [[ $err != '' ]] then
            print "ERROR|asset regular expression error: $err"
        fi
    fi
    noprivs=
    if [[ ${nfs[0]} != '' ]] then
        export MAXN=1
        $SHELL vg_dq_invhelper 'o' "" | egrep '^A\|o\|' | read assetn
        $SHELL vg_dq_invhelper 'o' "${nfs[0]}" | egrep '^A\|o\|' | read assetl
        assetn=${assetn##*'|'}
        assetl=${assetl##*'|'}
        if ! swmuseringroups "vg_att_admin|vg_att_user"; then
            MAXN=0 SHOWATTRS=1 $SHELL vg_dq_invhelper 'o' "${nfs[0]}" \
            | egrep '^D\|.*\|custpriv\|' | while read -r line; do
                rest=${line#*'|'}
                rest=${rest#*'|'}
                asset=${rest%%'|'*}
                rest=${rest#*'|'}
                rest=${rest#*'|'}
                priv=$rest
                [[ $priv != *T* ]] && noprivs+=" $asset"
            done
        fi
    else
        assetn=1
        assetl=1
    fi
    if (( assetl == 0 )) then
        print -n "ERROR|asset expression: '${nfs[0]}'"
        print " does not match any known assets"
    elif (( assetl == assetn )) then
        if swmuseringroups "vg_att_admin|vg_confpoweruser"; then
            print -n "WARNING|asset expression: '${nfs[0]}'"
            print " matches all known assets"
        elif swmuseringroups "vg_att_user"; then
            print -n "ERROR|asset expression: '${nfs[0]}'"
            print " matches all known assets"
        elif swmuseringroups "vg_cus_admin|vg_cus_user"; then
            print -n "WARNING|asset expression: '${nfs[0]}'"
            print " matches all known assets"
        else
            print -n "ERROR|asset expression: '${nfs[0]}'"
            print " matches all known assets"
        fi
    elif (( assetl > .5 * assetn )) then
        print -n "WARNING|asset expression: '${nfs[0]}'"
        print " matches over 50% of all known assets"
    elif (( assetl != 1 )) then
        print -n "WARNING|asset expression: '${nfs[0]}'"
        print " matches $assetl out of $assetn known assets"
    fi
    if [[ $noprivs != '' ]] then
        print -n "WARNING|these assets cannot be operated on due to service"
        print " level restrictions: $noprivs"
    fi

    # encode any '|' characters
    if [[ ${nfs[0]} == *'|'* ]] then
        nfs[0]=$(printf '%#H' "${nfs[0]}")
        rw=y
    fi

    # check stat name
    if [[ ${nfs[1]} != +([.a-zA-Z0-9_:-]) ]] then
        print "ERROR|the statistic id can only contain: . a-z A-Z 0-9 _ : -"
    fi

    # check stat mode
    if [[ ${nfs[2]} != +(keep|drop|noalarm) ]] then
        print "ERROR|the statistic status must be keep, drop, or noalarm"
    fi

    # check stat value
    if [[ ${nfs[3]} != '' ]] then
        vg_cm_profile_valuerec="${nfs[3]}"
        vg_cm_profile_valueunpack
        vg_cm_profile_valuecheck
        case $? in
        0)
            ;;
        1)
            vg_cm_profile_valuepack
            nfs[3]=$vg_cm_profile_valuerec
            ;;
        esac
    fi

    # check for dups
    if [[
        ${edit.a} == insert &&
        $(
            egrep "^${nfs[0]}\|${nfs[1]}\|.\|*($SWMID|$SWMCMID)" \
            $VGCM_PROFILEFILE \
        ) != ''
    ]] then
        print "WARNING|a profile for this asset and statistic already exists"
    fi

    # check account id
    if [[
        $(egrep "^${nfs[4]};" $VG_DWWWDIR/accounts/current) == ''
    ]] then
        print "ERROR|account id ${nfs[4]} does not exist"
    fi

    if [[ $rw == y ]] then
        x=${spec%%'<n>'*}
        x+='<n><r>'
        l=
        for (( kvi = 0; kvi < 5; kvi++ )) do
            f=$(printf '%#H' "${nfs[$kvi]}")
            x+="<f>$kvi|$f</f>"
            l+=" - $f"
        done
        l=${l#' - '}
        x+="<l>$l</l></r></n>"
        print -r "REC|$x"
    else
        print -r "REC|$spec"
    fi

    return 0
}

function vg_cm_profile_pack {
    typeset spec=$1

    typeset or kvi str kv k v f v g t

    eval $(vgxml -topmark edit -ksh "$spec")
    print -r -u5 "${edit.u}"
    case ${edit.a} in
    remove)
        or=${edit.o.r.t//'+'/' '}
        or=${or//@([\'\\])/'\'\1}
        eval "or=\$'${or//'%'@(??)/'\x'\1"'\$'"}'"
        print -r -u3 "$or"
        return
        ;;
    modify)
        or=${edit.o.r.t//'+'/' '}
        or=${or//@([\'\\])/'\'\1}
        eval "or=\$'${or//'%'@(??)/'\x'\1"'\$'"}'"
        print -r -u3 "$or"
        # continue with new record pack
        ;;
    insert)
        # continue with new record pack
        ;;
    *)
        print "ERROR|unknown action ${edit.a}"
        return
        ;;
    esac

    t=
    for (( kvi = 0; kvi < 5; kvi++ )) do
        if (( kvi == 0 )) then
            str=f
        else
            str=f_$kvi
        fi
        typeset -n kv=edit.n.r.$str
        k=${kv%%'|'*}
        v=${kv#"$k"'|'}
        v=${v//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        t+="|$v"
    done
    t=${t#'|'}
    print -r -u4 "$t"

    return 0
}

function vg_cm_profile_split {
    typeset fmt=$1 rec=$2

    typeset x l r rest i f fl id

    [[ $rec != *'|'* ]] && return 0

    x='<r>'
    l=
    r=
    rest=$rec
    for (( i = 0; i < 5; i++ )) do
        f=${rest%%'|'*}
        fl=$f
        rest=${rest##"$f"?('|')}
        if (( i == 0 )) then
            if [[ $f == *'%'??* ]] then
                fl=${f//'+'/' '}
                fl=${fl//@([\'\\])/'\'\1}
                eval "fl=\$'${fl//'%'@(??)/'\x'\1"'\$'"}'"
            fi
        elif (( i == 1 )) then
            if [[ $f == *__LABEL__* ]] then
                f=${f#*__LABEL__}
                fl="${f} (${fl%%__LABEL__*})"
            fi
        elif (( i == 4 )) then
            id=$f
        fi
        r+="|$f"
        if (( i == 0 )) then
            f=$fl
        fi
        f=$(printf '%#H' "$f")
        fl=$(printf '%#H' "$fl")
        x+="<f>$i|$f</f>"
        l+=" - $fl"
    done
    l=${l#' - '}
    r=${r#'|'}
    x+="<l>$l</l><t>$(printf '%#H' "$r")</t></r>"

    if [[ $id == @($SWMID|$SWMCMID) || $VIEWALL == 1 ]] then
        print -r "$x"
    fi

    return 0
}
