
function vg_cm_filter_check {
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
        if [[ $EDITALL != 1 && ${x.f_12} != *'|'@($SWMID|$SWMCMID) ]] then
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
        if [[ $EDITALL != 1 && ${x.f_12} != *'|'@($SWMID|$SWMCMID) ]] then
            print "ERROR|Account IDs do not match - cannot modify record"
            return
        fi
        typeset -n x=edit.n.r
        if [[ ${x.f_12} != "12|"@($SWMID|$SWMCMID) ]] then
            if [[ $SWMCMID != '' ]] then
                x.f_12="12|$SWMCMID"
            else
                x.f_12="12|$SWMID"
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
        if [[ ${x.f_12} != "12|"@($SWMID|$SWMCMID) ]] then
            if [[ $SWMCMID != '' ]] then
                x.f_12="12|$SWMCMID"
            else
                x.f_12="12|$SWMID"
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

    for (( kvi = 0; kvi < 13; kvi++ )) do
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
        if (( kvi == 3 || kvi == 4 )) then
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
        elif (( kvi == 5 || kvi == 6 )) then
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
                [[ $priv != *F* ]] && noprivs+=" $asset"
            done
        fi
    else
        assetn=1
        assetl=1
    fi
    if (( assetl == 0 )) then
        print -n "WARNING|asset expression: '${nfs[0]}'"
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
        if swmuseringroups "vg_att_admin|vg_confpoweruser"; then
            if (( ${#v} < 12 )) then
                print -n "WARNING|message text: '${nfs[2]}'"
                print -n " has less than 12 alphanumeric chars"
                print -n " while the asset expression: '${nfs[0]}'"
                print "matches all known assets"
            fi
        elif swmuseringroups "vg_att_user"; then
            if (( ${#v} < 12 )) then
                print -n "ERROR|message text: '${nfs[2]}'"
                print -n " has less than 12 alphanumeric chars"
                print -n " while the asset expression: '${nfs[0]}'"
                print "matches all known assets"
            fi
        elif swmuseringroups "vg_cus_admin|vg_cus_user"; then
            if (( ${#v} < 12 )) then
                print -n "WARNING|message text: '${nfs[2]}'"
                print -n " has less than 12 alphanumeric chars"
                print -n " while the asset expression: '${nfs[0]}'"
                print "matches all known assets"
            fi
        else
            print -n "ERROR|asset field: '${nfs[0]}'"
            print " matches all known assets which is not allowed"
        fi
    fi
    if ! swmuseringroups "vg_att_admin|vg_confpoweruser"; then
        if [[ ${nfs[0]} == *[\[\]\(\)\{\}\^\$\|\*]* ]] then
            print -n "ERROR|asset field: '${nfs[0]}'"
            print " is a regular expression which is not allowed"
        fi
        if (( assetl != 1 )) then
            print -n "WARNING|asset field: '${nfs[0]}'"
            print " should match exactly one asset"
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
    if [[ ${ofs[3]} == '' || ${ofs[4]} == '' ]] then
        print "ERROR|both start and end times must be specified"
    else
        if [[ ${nfs[3]} == "" ]] then
            print "ERROR|cannot parse start time: '${nfs[3]}'"
        elif [[ ${nfs[3]} != ${ofs[3]} ]] then
            print -n "WARNING|parsed start time: '${nfs[3]}'"
            print " differs from input: '${ofs[3]}'"
        fi
        if [[ ${nfs[4]} == "" ]] then
            print "ERROR|cannot parse end time: '${nfs[4]}'"
        elif [[ ${nfs[4]} != ${ofs[4]} ]] then
            print -n "WARNING|parsed end time: '${nfs[4]}'"
            print " differs from input: '${ofs[4]}'"
        fi
        if [[
            ${ts[3]} != '' && ${ts[4]} != ''
        ]] && (( ${ts[3]} > ${ts[4]} )) then
            print -n "WARNING since start time: '${nfs[3]}'"
            print -n " is greater than end time: '${nfs[4]}'"
            print " end time will be assumed to be in the following day"
        fi
    fi

    # check start and end dates
    if [[ ${ofs[5]} == '' || ${ofs[6]} == "" ]] then
        print "ERROR|both start and end dates must be specified"
    else
        if [[ ${nfs[5]} == "" ]] then
            print "ERROR|cannot parse start date: '${nfs[5]}'"
        elif [[ ${nfs[5]} != ${ofs[5]} ]] then
            print -n "WARNING|parsed start date: '${nfs[5]}'"
            print " differs from input: '${ofs[5]}'"
        fi
        if [[ ${nfs[6]} == "" ]] then
            print "ERROR|cannot parse end date: '${nfs[6]}'"
        elif [[ ${nfs[6]} != ${ofs[6]} ]] then
            print -n "WARNING|parsed end date: '${nfs[6]}'"
            print " differs from input: '${ofs[6]}'"
        fi
        if [[
            ${nfs[6]} != indefinitely && ${ts[5]} != '' && ${ts[6]} != ''
        ]] && (( ${ts[5]} > ${ts[6]} )) then
            print -n "ERROR|start date: '${nfs[5]}'\c"
            print " is greater than end date: '${nfs[6]}'"
        fi
    fi

    # check repeat mode
    if [[
        ${nfs[7]} != once && ${nfs[7]} != daily &&
        ${nfs[7]} != weekly && ${nfs[7]} != monthly
    ]] then
        print "ERROR|the repeat mode must be once, daily, weekly, or monthly"
    fi
    if [[
       ${nfs[7]} != once && ${ts[5]} != '' && ${ts[6]} != ''
    ]] && (( ${ts[5]} == ${ts[6]} )) then
        print -n "WARNING|start and end date are the same"
        print " but repeat mode is not 'once'"
    fi

    # check ticket mode
    if [[ ${nfs[8]} != keep && ${nfs[8]} != drop ]] then
        print "ERROR|the ticket mode must be keep or drop"
    fi
    if [[ ${nfs[8]} == drop && ${nfs[9]} == drop && ${nfs[10]} != '' ]] then
        print -n "WARNING|ticket and viz modes are both set to: 'drop'"
        print " but severity is also changed: '${nfs[10]}'"
    fi

    # check viz mode mode
    if [[ ${nfs[9]} != keep && ${nfs[9]} != drop ]] then
        print "ERROR|the viz mode must be keep or drop"
    fi

    # check severity
    if [[
        ${nfs[10]} != '' && ${nfs[10]} != normal &&
        ${nfs[10]} != warning && ${nfs[10]} != minor &&
        ${nfs[10]} != major && ${nfs[10]} != critical
    ]] then
        print -n "ERROR|the severity must be any, normal, warning,"
        print " minor, major, critical"
    fi

    # check comment
    if (( ${#nfs[11]} == 0 )) then
        print "ERROR|comment field is mandatory"
    fi
    if (( ${#nfs[11]} > 127 )) then
        print "ERROR|comment longer than 127 characters: '${#nfs[11]}'"
    fi

    # check account id
    if [[
        $(egrep "^${nfs[12]};" $VG_DWWWDIR/accounts/current) == ''
    ]] then
        print "ERROR|account id ${nfs[12]} does not exist"
    fi

    if ! swmuseringroups "vg_att_admin|vg_confpoweruser"; then
        if [[ ${nfs[8]} != keep || ${nfs[9]} != keep ]] then
            if [[ ${nfs[6]} == indefinitely ]] then
                if [[ ${nfs[7]} == once ]] then
                    print -n "ERROR|cannot use indefinite as end date when"
                    print " repeat mode is 'once'"
                elif [[ ${nfs[7]} == daily ]] then
                    (( ts[4] < ts[3] )) && (( ts[4] += 24 * 60 ))
                    if (( ts[4] - ts[3] > 12 * 60 )) then
                        print -n "ERROR|cannot use indefinite as end date when"
                        print -n " repeat mode is 'daily' and duration is more"
                        print " than 12 hours"
                    fi
                fi
            elif [[ ${nfs[7]} == once ]] then
                if (( ts[6] - ts[5] > 7 * 24 * 60 * 60 )) then
                    print "ERROR|cannot use a duration of more than 7 days"
                fi
            fi
        fi
    fi

    if [[ $rw == y ]] then
        x=${spec%%'<n>'*}
        x+='<n><r>'
        l=
        for (( kvi = 0; kvi < 13; kvi++ )) do
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

function vg_cm_filter_pack {
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
    for (( kvi = 0; kvi < 13; kvi++ )) do
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
        if (( kvi == 3 || kvi == 4 )) then
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
        elif (( kvi == 5 || kvi == 6 )) then
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

function vg_cm_filter_split {
    typeset fmt=$1 rec=$2

    typeset fs x l rest i f g d id ls ts hm s t ifs
    typeset -Z2 hh mm

    [[ $rec != *'|+|'* ]] && return 0

    rest=${rec//'|+|'/$'\b'}
    ifs="$IFS"
    IFS=$'\b'
    set -f
    set -A fs -- $rest
    set +f
    IFS="$ifs"

    if [[ ${fs[3]} != '' && ${fs[3]} != {1,2}([0-9]):{1,2}([0-9]) ]] then
        (( hh = fs[3] / 60 ))
        (( mm = fs[3] % 60 ))
        fs[3]="$hh:$mm"
    fi
    if [[ ${fs[4]} != '' && ${fs[4]} != {1,2}([0-9]):{1,2}([0-9]) ]] then
        (( hh = fs[4] / 60 ))
        (( mm = fs[4] % 60 ))
        fs[4]="$hh:$mm"
    fi
    if [[ ${fs[5]} == {4}([0-9]).{1,2}([0-9]).{1,2}([0-9])* ]] then
        g=${fs[5]##{4}([0-9]).{1,2}([0-9]).{1,2}([0-9])}
        d=${fs[5]%$g}
        fs[5]=$(TZ=$PHTZ printf '%(%#)T' "$d-00:00:00" 2> /dev/null) || fs[5]=""
    fi
    if [[ ${fs[6]} == {4}([0-9]).{1,2}([0-9]).{1,2}([0-9])* ]] then
        g=${fs[6]##{4}([0-9]).{1,2}([0-9]).{1,2}([0-9])}
        d=${fs[6]%$g}
        fs[6]=$(TZ=$PHTZ printf '%(%#)T' "$d-00:00:00" 2> /dev/null) || fs[6]=""
    fi

    if [[ ${fs[3]} != '' && ${fs[5]} != @(|indefinitely) ]] then
        hm=$(TZ=$PHTZ printf '%(%H:%M)T' "#${fs[5]}")
        if [[ $hm != 00:00 ]] then
            (( t = ${fs[5]} + (${fs[3]:0:2} * 60 + ${fs[3]:3:2}) * 60 ))
            set -A ts -- $(TZ=$PHTZ printf '%(%Y.%m.%d %a %H:%M)T' "#$t")
            fs[3]=${ts[2]}
            fs[5]="${ts[0]} ${ts[1]}"
        else
            fs[5]=$(TZ=$PHTZ printf '%(%Y.%m.%d %a)T' "#${fs[5]}")
        fi
    fi
    if [[ ${fs[4]} != '' && ${fs[6]} != @(|indefinitely) ]] then
        hm=$(TZ=$PHTZ printf '%(%H:%M)T' "#${fs[6]}")
        if [[ $hm != 00:00 ]] then
            (( t = ${fs[6]} + (${fs[4]:0:2} * 60 + ${fs[4]:3:2}) * 60 ))
            set -A ts -- $(TZ=$PHTZ printf '%(%Y.%m.%d %a %H:%M)T' "#$t")
            fs[4]=${ts[2]}
            fs[6]="${ts[0]} ${ts[1]}"
        else
            fs[6]=$(TZ=$PHTZ printf '%(%Y.%m.%d %a)T' "#${fs[6]}")
        fi
    fi
    id=${fs[12]}

    for (( i = 0; i < 13; i++ )) do
        if [[ $i == @(0|1|2|11|12) ]] then
            fs[$i]=$(printf '%#H' "${fs[$i]}")
        fi
        ls[$i]="$s${fs[$i]}"
        fs[$i]="<f>$i|${fs[$i]}</f>"
        s=' - '
    done

    if [[ $id == @($SWMID|$SWMCMID) || $VIEWALL == 1 ]] then
        IFS=''
        print -r "<r>${fs[*]}<l>${ls[*]}</l><t>$(printf '%#H' "$rec")</t></r>"
        IFS="$ifs"
    fi

    return 0
}
