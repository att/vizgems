
function vg_cm_alarmemail_check {
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
        if [[ $EDITALL != 1 && ${x.f_14} != *'|'@($SWMID|$SWMCMID) ]] then
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
        if [[ $EDITALL != 1 && ${x.f_14} != *'|'@($SWMID|$SWMCMID) ]] then
            print "ERROR|Account IDs do not match - cannot modify record"
            return
        fi
        typeset -n x=edit.n.r
        if [[ ${x.f_14} != "14|"@($SWMID|$SWMCMID) ]] then
            if [[ $SWMCMID != '' ]] then
                x.f_14="14|$SWMCMID"
            else
                x.f_14="14|$SWMID"
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
        if [[ ${x.f_14} != "14|"@($SWMID|$SWMCMID) ]] then
            if [[ $SWMCMID != '' ]] then
                x.f_14="14|$SWMCMID"
            else
                x.f_14="14|$SWMID"
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

    for (( kvi = 0; kvi < 15; kvi++ )) do
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
        if (( kvi == 5 || kvi == 6 )) then
            v=${v#+(\ )}
            v=${v%+(\ )}
            if [[ $v == {1,2}([0-9]):{1,2}([0-9]) ]] then
                v=$(printf '%02d:%02d' "${v%%:*}" "${v##*:}") || v=""
                ts[$kvi]=$(( ${v:0:2} * 60 + ${v:3:2} ))
            elif [[ $v == {1,2}([0-9]) ]] then
                v=$(printf '%02d:%02d' "$v" "00") || v=""
                ts[$kvi]=$(( ${v:0:2} * 60 + ${v:3:2} ))
            else
                v=""
                ts[$kvi]=''
            fi
        elif (( kvi == 7 || kvi == 8 )) then
            v=${v#+(\ )}
            v=${v%+(\ )}
            if [[ $v == {4}([0-9]).{1,2}([0-9]).{1,2}([0-9])* ]] then
                g=${v##{4}([0-9]).{1,2}([0-9]).{1,2}([0-9])}
                v=${v%$g}
                ts[$kvi]=$(TZ=$PHTZ printf '%(%#)T' "$v-00:00:00" 2> /dev/null)
                v=$(
                    TZ=$PHTZ printf '%(%Y.%m.%d %a)T' "$v-00:00:00" 2> /dev/null
                ) || v=""
            elif [[ $v == indefinitely ]] then
                ts[$kvi]='indefinitely'
            else
                v=""
                ts[$kvi]=''
            fi
        fi
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
                [[ $priv != *E* ]] && noprivs+=" $asset"
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
        print -n "WARNING|asset expression: '${nfs[0]}'"
        print " matches all known assets"
    elif (( assetl > .5 * assetn )) then
        print -n "WARNING|asset expression: '${nfs[0]}'"
        print " matches over 50% of all known assets"
    elif (( assetl != 1 )) then
        print -n "WARNING|asset expression: '${nfs[0]}'"
        print " matches $assetl out of $assetn known assets"
    fi
    if (( assetl == assetn && assetn > 0 )) then
        v=${nfs[2]//[!.a-zA-Z0-9_:-]/}
        if (( ${#v} < 12 )) then
            print -n "ERROR|message text: '${nfs[2]}'"
            print -n " has less than 12 alphanumeric chars"
            print -n " while the asset expression: '${nfs[0]}'"
            print "matches all known assets"
        fi
    fi
    if [[ $noprivs != '' ]] then
        print -n "WARNING|these assets cannot be operated on due to service"
        print " level restrictions: $noprivs"
    fi

    # check alarmid
    if [[ ${nfs[1]} != '' ]] then
        err=''
        print "" | egrep -S -- "${nfs[1]}" 2>&1 | read err
        err=${err#egrep:\ }
        if [[ $err != '' ]] then
            print "ERROR|alarm id regular expression error: $err"
        fi
    fi

    # check message text
    if [[ ${nfs[2]} != '' ]] then
        ire=${nfs[2]}
        ore=''
        for (( i = 0; i < ${#ire}; i++ )) do
            if [[
                ${ire:$i:1} == @(['(){}^$']|\[|\]) &&
                (i == 0 || ${ire:$((i - 1)):1} != '\\')
            ]] then
                ore="${ore}."
            elif [[
                ${ire:$i:1} == '*' && (i == 0 || ${ire:$((i - 1)):1} != '.')
            ]] then
                ore="${ore}."
            elif [[ ${ire:$i:1} == '\\' && ${ire:$((i + 1)):1} == '' ]] then
                ore="${ore}."
            else
                ore="${ore}${ire:$i:1}"
            fi
        done
        err=''
        print "" | egrep -S -- "$ore" 2>&1 | read err
        err=${err#egrep:\ }
        if [[ $err != '' ]] then
            print "ERROR|message text regular expression error: $err"
        fi
    fi

    # check start and end times
    if [[ ${ofs[5]} == '' || ${ofs[6]} == '' ]] then
        print "ERROR|both start and end times must be specified"
    else
        if [[ ${nfs[5]} == "" ]] then
            print "ERROR|cannot parse start time: '${nfs[5]}'"
        elif [[ ${nfs[5]} != ${ofs[5]} ]] then
            print -n "WARNING|parsed start time: '${nfs[5]}'"
            print " differs from input: '${ofs[5]}'"
        fi
        if [[ ${nfs[6]} == "" ]] then
            print "ERROR|cannot parse end time: '${nfs[6]}'"
        elif [[ ${nfs[6]} != ${ofs[6]} ]] then
            print -n "WARNING|parsed end time: '${nfs[6]}'"
            print " differs from input: '${ofs[6]}'"
        fi
        if [[
            ${ts[5]} != '' && ${ts[6]} != ''
        ]] && (( ${ts[5]} > ${ts[6]} )) then
            print -n "WARNING since start time: '${nfs[5]}'"
            print -n " is greater than end time: '${nfs[6]}'"
            print " end time will be assumed to be in the following day"
        fi
    fi

    # check start and end dates
    if [[ ${ofs[7]} == '' || ${ofs[8]} == "" ]] then
        print "ERROR|both start and end dates must be specified"
    else
        if [[ ${nfs[7]} == "" ]] then
            print "ERROR|cannot parse start date: '${nfs[7]}'"
        elif [[ ${nfs[7]} != ${ofs[7]} ]] then
            print -n "WARNING|parsed start date: '${nfs[7]}'"
            print " differs from input: '${ofs[7]}'"
        fi
        if [[ ${nfs[8]} == "" ]] then
            print "ERROR|cannot parse end date: '${nfs[8]}'"
        elif [[ ${nfs[8]} != ${ofs[8]} ]] then
            print -n "WARNING|parsed end date: '${nfs[8]}'"
            print " differs from input: '${ofs[8]}'"
        fi
        if [[
            ${nfs[8]} != indefinitely && ${ts[7]} != '' && ${ts[8]} != ''
        ]] && (( ${ts[7]} > ${ts[8]} )) then
            print -n "ERROR|start date: '${nfs[7]}'\c"
            print " is greater than end date: '${nfs[8]}'"
        fi
    fi

    # check repeat mode
    if [[
        ${nfs[9]} != once && ${nfs[9]} != daily &&
        ${nfs[9]} != weekly && ${nfs[9]} != monthly
    ]] then
        print "ERROR|the repeat mode must be once, daily, weekly, or monthly"
    fi
    if [[
       ${nfs[9]} != once && ${ts[7]} != '' && ${ts[8]} != ''
    ]] && (( ${ts[7]} == ${ts[8]} )) then
        print -n "WARNING|start and end date are the same"
        print " but repeat mode is not 'once'"
    fi

    # check ticket mode
    if [[ ${nfs[3]} != '' && ${nfs[3]} != keep && ${nfs[3]} != drop ]] then
        print "ERROR|the ticket mode must be any, keep, or drop"
    fi

    # check severity
    if [[
        ${nfs[4]} != '' && ${nfs[4]} != normal &&
        ${nfs[4]} != warning && ${nfs[4]} != minor &&
        ${nfs[4]} != major && ${nfs[4]} != critical
    ]] then
        print -n "ERROR|the severity must be any, normal, warning,"
        print " minor, major, critical"
    fi

    # check email info
    if [[ ${nfs[10]} != ?*@*? ]] then
        print "ERROR|the from address field is mandatory"
    fi
    if [[ ${nfs[10]} == *[\*\|]* ]] then
        print "ERROR|illegal characters in from address"
    fi
    if [[ ${nfs[11]} != ?*@*? ]] then
        print "ERROR|the to address field is mandatory"
    fi
    if [[ ${nfs[11]} == *[\*\|]* ]] then
        print "ERROR|illegal characters in to address"
    fi
    if [[
        ${nfs[12]} != html && ${nfs[12]} != text && ${nfs[12]} != sms
    ]] then
        print "ERROR|the style must be HTML, Text, SMS"
    fi
    if [[ ${nfs[13]} != ?* ]] then
        print "ERROR|subject field is mandatory"
    fi

    # check account id
    if [[
        $(egrep "^${nfs[14]};" $VG_DWWWDIR/accounts/current) == ''
    ]] then
        print "ERROR|account id ${nfs[14]} does not exist"
    fi

    if [[ $rw == y ]] then
        x=${spec%%'<n>'*}
        x+='<n><r>'
        l=
        for (( kvi = 0; kvi < 15; kvi++ )) do
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

function vg_cm_alarmemail_pack {
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
    for (( kvi = 0; kvi < 15; kvi++ )) do
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
        if (( kvi == 5 || kvi == 6 )) then
            v=${v#+(\ )}
            v=${v%+(\ )}
            if [[ $v == {1,2}([0-9]):{1,2}([0-9]) ]] then
                v=$(printf '%02d:%02d' "${v%%:*}" "${v##*:}") || v=""
                v=$(( ${v:0:2} * 60 + ${v:3:2} ))
            elif [[ $v == {1,2}([0-9]) ]] then
                v=$(printf '%02d:%02d' "$v" "00") || v=""
                v=$(( ${v:0:2} * 60 + ${v:3:2} ))
            else
                v=""
            fi
        elif (( kvi == 7 || kvi == 8 )) then
            v=${v#+(\ )}
            v=${v%+(\ )}
            if [[ $v == {4}([0-9]).{1,2}([0-9]).{1,2}([0-9])* ]] then
                g=${v##{4}([0-9]).{1,2}([0-9]).{1,2}([0-9])}
                v=${v%$g}
                v=$(printf '%(%#)T' "$v-00:00:00" 2> /dev/null)
            elif [[ $v != indefinitely ]] then
                v=""
            fi
        fi
        t+="|+|$v"
    done
    t=${t#'|+|'}
    print -r -u4 "$t"

    return 0
}

function vg_cm_alarmemail_split {
    typeset fmt=$1 rec=$2

    typeset -A fs
    typeset x l rest i f id g d t ts hm

    [[ $rec != *'|+|'* ]] && return 0

    rest=$rec
    for (( i = 0; i < 15; i++ )) do
        f=${rest%%'|+|'*}
        rest=${rest##"$f"?('|+|')}
        if (( i == 5 || i == 6 )) then
            if [[ $f != '' && $f != {1,2}([0-9]):{1,2}([0-9]) ]] then
                f=$(printf '%02d:%02d' $(( $f / 60 )) $(( $f % 60 ))) || f=""
            fi
        elif (( i == 7 || i == 8 )) then
            if [[ $f == {4}([0-9]).{1,2}([0-9]).{1,2}([0-9])* ]] then
                g=${f##{4}([0-9]).{1,2}([0-9]).{1,2}([0-9])}
                d=${f%$g}
                f=$(TZ=$PHTZ printf '%(%#)T' "$d-00:00:00" 2> /dev/null) || f=""
            fi
        fi
        fs[$i]=$f
    done

    if [[ ${fs[5]} != '' && ${fs[7]} != @(|indefinitely) ]] then
        hm=$(TZ=$PHTZ printf '%(%H:%M)T' "#${fs[7]}")
        if [[ $hm != 00:00 ]] then
            (( t = ${fs[7]} + (${fs[5]:0:2} * 60 + ${fs[5]:3:2}) * 60 ))
            set -A ts -- $(TZ=$PHTZ printf '%(%Y.%m.%d %a %H:%M)T' "#$t")
            fs[5]=${ts[2]}
            fs[7]="${ts[0]} ${ts[1]}"
        else
            fs[7]=$(TZ=$PHTZ printf '%(%Y.%m.%d %a)T' "#${fs[7]}")
        fi
    fi
    if [[ ${fs[6]} != '' && ${fs[8]} != @(|indefinitely) ]] then
        hm=$(TZ=$PHTZ printf '%(%H:%M)T' "#${fs[8]}")
        if [[ $hm != 00:00 ]] then
            (( t = ${fs[8]} + (${fs[6]:0:2} * 60 + ${fs[6]:3:2}) * 60 ))
            set -A ts -- $(TZ=$PHTZ printf '%(%Y.%m.%d %a %H:%M)T' "#$t")
            fs[6]=${ts[2]}
            fs[8]="${ts[0]} ${ts[1]}"
        else
            fs[8]=$(TZ=$PHTZ printf '%(%Y.%m.%d %a)T' "#${fs[8]}")
        fi
    fi
    id=${fs[14]}

    x='<r>'
    l=
    for (( i = 0; i < 15; i++ )) do
        f=$(printf '%#H' "${fs[$i]}")
        x+="<f>$i|$f</f>"
        l+=" - $f"
    done
    l=${l#' - '}
    x+="<l>$l</l><t>$(printf '%#H' "$rec")</t></r>"

    if [[ $id == @($SWMID|$SWMCMID) || $VIEWALL == 1 ]] then
        print -r "$x"
    fi

    return 0
}
