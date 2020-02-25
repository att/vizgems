
usage=$'
[-1p1?
@(#)$Id: vg_scopemgr (AT&T) 2007-03-25 $
]
[+NAME?vg_scopemgr - scope manager]
[+DESCRIPTION?\bvg_scopemgr\b handles all the interactions with the scopes.
]
[200:scopehb?process heartbeat information from the scopes and if a scope has
not sent a heartbeat messege recently, mark it as down and redistribute its
load to other scopes.
]
[201:validate?validate all scope to asset assignments.
]
[202:scopestate?print the state of all configured scopes.
]
[203:scopemap?print the mapping of which scopes monitor which assets.
]
[204:reinv?invalidate autoinv of specified asset so that it will be recomputed.
]:[asset]
[205:rebalance?change asset assignments so that the load is more evenly
balanced across available scopes.
the argument to this option specifies the minimum change in load that will
trigger an asset to move to another scope.
it should be a small number, e.g. 5 - 50.
]#[mincost]
[206:getfiles?copy to local system all the scope files for the specified asset.
]:[asset]
[300:noauto?disable the automatic inventory collection.
]
[301:nopush?disable the push of schedules to scopes.
]
[302:nowait?if another instance of \bvg_scopemgr\b is running, exit instead
of wait until the other instance finishes.
]
[400:admindown?set the state of the scope specified as the argument to down.
this will take the scope out of the pool of active scopes.
]:[scope]
[401:adminup?set the state of the scope specified as the argument to up.
this will put the scope back into the pool of active scopes.
]:[scope]
[999:v?increases the verbosity level. May be specified multiple times.]
'

dontcareattr='si_timestamp'

typeset -A costmap
costmap[bi]=1.5
costmap[cisco]=1.0
costmap[gsr]=1.5
costmap[mds]=1.5
costmap[linux.i386]=1.0
costmap[linux.i386-leg]=1.0
costmap[netdev]=1.0
costmap[netscaler]=10.0
costmap[solaris.sun4]=1.0
costmap[url]=1.0
costmap[vmware.i386]=1.0
costmap[win32.i386]=1.0

function showusage {
    OPTIND=0
    getopts -a vg_scopemgr "$usage" opt '-?'
}

function timedrun { # $1 = timeout, $2-* cmd and args
    typeset maxt jid i

    maxt=$1
    shift 1

    set -o monitor
    "$@" &
    jid=$!
    for (( i = 0; i < maxt; i += 0.1 )) do
        kill -0 $jid 2> /dev/null || break
        sleep 0.1
    done
    if (( i >= maxt )) then
        if kill -9 -$jid 2> /dev/null; then
            print -u2 scopemgr: timeout for job "$@"
        fi
    fi
    wait $jid 2> /dev/null
    return $?
}

ipinbits=
function ipinnets { # $1 = aip $2-n = bits
    typeset aip=$1; shift
    typeset baip bits

    [[ $# == 0 ]] && set -- ""
    ipinbits=
    baip=$(ip2bm $aip)
    if [[ $baip == '' && $aip != '' ]] then
        return 0 # assuming a host name is used instead of ip
    fi
    for bits in "$@"; do
        if [[ ${baip:0:${#bits}} == $bits ]] then
            ipinbits=$bits
            return 0
        fi
    done

    return 1
}
function ip2bm { # $1 = ip
    typeset ip=$1
    typeset acs cs cn ecs os f1 fmt
    ip=${ip//[$'\t ']/}
    case $ip in
    *:*:*)
        acs=':::::::'
        cs=${ip//[!:]/}; cn=${#cs}
        if (( cn < 7 )) then
            ecs=${acs:0:$(( 9 - cn ))}
            ip=${ip/::/$ecs}
        fi
        set -A os -- 16#${ip//:/'|16#'}
        f1='%016..2d'
        fmt="x$f1$f1$f1$f1$f1$f1$f1$f1"
        printf $fmt ${os[@]}
        ;;
    +([0-9.]))
        acs='.0.0.0.0'
        cs=${ip//[!.]/}; cn=${#cs}
        if (( cn < 3 )) then
            ecs=${acs:0:$(( (3 - cn) * 2 ))}
            ip=$ip$ecs
        fi
        set -A os -- 10#${ip//./'|10#'}
        f1='%08..2d'
        fmt="d$f1$f1$f1$f1"
        printf $fmt ${os[@]}
        ;;
    *) # assuming it's a hostname instead of an IP address
        ;;
    esac
}

function tagintags { # $1 = needtags $2 = scopetags
    typeset ntags=$1 stags=$2
    typeset tag

    stags=${stags:-default}
    for tag in ${ntags//' '/'|'}; do
        if [[ " $stags " == *" $tag "* ]] then
            return 0
        fi
    done

    return 1
}

bestsid= bestfosid=
function findscope { # alv aid aip ast asl ant sst
    typeset alv=$1 aid=$2 aip=$3 ast=$4 asl=$5 ant=$6 sst=$7
    typeset sid cost minsid mincost

    ant=${ant:-default}

    bestsid= bestfosid=

    sid=${alvaidsst2fosid[$alv:$aid:$sst]}
    if [[ $sid != '' && ${sid2state[$sid]} == up ]] then
        print -u2 MESSAGE reverting to original scope $sid for asset $alv:$aid
        bestsid=$sid
        return 0
    fi
    bestfosid=$sid

    sid=${alvaidsst2sid[$alv:$aid:$sst]}
    if [[ $sid != '' ]] then
        if [[ ${sid2state[$sid]} == up ]] then
            print -u2 MESSAGE keeping scope $sid for asset $alv:$aid
            bestsid=$sid
            return 0
        else
            print -u2 MESSAGE failing over scope $sid for asset $alv:$aid
            [[ $bestfosid == '' ]] && bestfosid=$sid
        fi
    fi

    minsid=
    mincost=
    for sid in ${sst2sid[$sst]}; do
        [[ ${sid2state[$sid]} != up ]] && continue
        if ! tagintags "$ant" "${sid2tags[$sid]}"; then
            continue
        fi
        if ! ipinnets "$aip" ${sid2bits[$sid]}; then
            continue
        fi
        ((
            cost = ${sid2cost[$sid]} +
            ${costmap[$ast]:-1} * ${sidbits2weights[$sid:$ipinbits]:-1.0}
        ))
        if [[ $sid == $aid && $alv == o ]] then
            cost=10000 # avoid having scopes monitor themselves
        fi
        if [[ $mincost == '' ]] then
            mincost=$cost
            minsid=$sid
        elif (( mincost > cost )) then
            mincost=$cost
            minsid=$sid
        fi
    done
    if [[ $minsid == '' ]] then
        print -u2 ERROR ip $aip for asset $alv:$aid not covered by $sst scopes
    fi
    bestsid=$minsid

    return 0
}

function changescope { # alv aid aip ast asl ant sst
    typeset alv=$1 aid=$2 aip=$3 ast=$4 asl=$5 ant=$6 sst=$7
    typeset sid cost minsid mincost

    ant=${ant:-default}

    bestsid=${alvaidsst2sid[$alv:$aid:$sst]}
    bestfosid=${alvaidsst2fosid[$alv:$aid:$sst]}

    if [[ $bestsid == '' || $bestfosid != '' ]] then
        print -u2 MESSAGE RB skipping rebalance for asset $alv:$aid
        return 0
    fi

    minsid=
    mincost=
    for sid in ${sst2sid[$sst]}; do
        [[ ${sid2state[$sid]} != up ]] && continue
        if ! tagintags "$ant" "${sid2tags[$sid]}"; then
            continue
        fi
        if ! ipinnets "$aip" ${sid2bits[$sid]}; then
            continue
        fi
        ((
            cost = ${sid2cost[$sid]} +
            ${costmap[$ast]:-1} * ${sidbits2weights[$sid:$ipinbits]:-1.0}
        ))
        if [[ $sid == $aid && $alv == o ]] then
            cost=10000 # avoid having scopes monitor themselves
        fi
        if [[ $mincost == '' ]] then
            mincost=$cost
            minsid=$sid
        elif (( mincost > cost )) then
            mincost=$cost
            minsid=$sid
        fi
    done
    if [[ $minsid == '' || $mincost == '' ]] then
        return 0 # if there is a coverage issue findscope will print an error
    fi
    if [[ $bestsid == $minsid ]] || ((
        ${sid2cost[$bestsid]} < ${sid2cost[$minsid]} + $rbmincost
    )) then
        print -u2 MESSAGE RB keeping scope $bestsid for asset $alv:$aid
        return 0
    fi

    bestsid=$minsid
    print -u2 MESSAGE RB changing to scope $bestsid for asset $alv:$aid

    return 0
}

function validatescope { # alv aid aip ast asl ant sst
    typeset alv=$1 aid=$2 aip=$3 ast=$4 asl=$5 ant=$6 sst=$7
    typeset sid ret

    ret=0
    ant=${ant:-default}

    sid=${alvaidsst2fosid[$alv:$aid:$sst]}
    if [[ $sid != '' ]] then
        if [[ ${sid2sip[$sid]} == '' ]] then
            print -u2 ERROR scope $sid for asset $aid not in scope list
            alvaidsst2fosid[$alv:$aid:$sst]=
            sid=
            ret=1
        elif ! tagintags "$ant" "${sid2tags[$sid]}"; then
            print -u2 ERROR scope $sid cannot monitor for asset $aid
            alvaidsst2fosid[$alv:$aid:$sst]=
            sid=
            ret=1
        elif ! ipinnets "$aip" ${sid2bits[$sid]}; then
            print -u2 ERROR scope $sid cannot reach network for asset $aid
            alvaidsst2fosid[$alv:$aid:$sst]=
            sid=
            ret=1
        fi
    fi
    bestsid=$sid

    sid=${alvaidsst2sid[$alv:$aid:$sst]}
    if [[ $sid != '' ]] then
        if [[ ${sid2sip[$sid]} == '' ]] then
            print -u2 ERROR scope $sid for asset $aid not in scope list
            alvaidsst2sid[$alv:$aid:$sst]=
            sid=
            ret=1
        elif ! tagintags "$ant" "${sid2tags[$sid]}"; then
            print -u2 ERROR scope $sid cannot monitor for asset $aid
            alvaidsst2sid[$alv:$aid:$sst]=
            sid=
            ret=1
        elif ! ipinnets "$aip" ${sid2bits[$sid]}; then
            print -u2 ERROR scope $sid cannot reach network for asset $aid
            alvaidsst2sid[$alv:$aid:$sst]=
            sid=
            ret=1
        fi
    fi
    bestfosid=$sid

    return $ret
}

typeset -A multiks multivs
integer multin multii
function expandmulti { # mn alv aid impl
    typeset mn=$1 alv=$2 aid=$3 impl=$4

    typeset kp k v l ks s kp2 k2 v2

    multin=0
    if [[ $impl == *\[\[*\]\]* ]] then
        kp=${impl#*'[['}
        kp=${kp%%']]'*}
        kp=${kp#*!P!}
        kp=${kp%!S!*}
        kp=${kp%!X!*}
        kp=${kp#!}
        kp=${kp%!*}
        typeset -n ar=aattr.[$alv:$aid]
        for k in "${!ar.ks[@]}"; do
            if [[ $k == $kp* ]] then
                ks=${k#"$kp"}
            else
                continue
            fi
            v=${ar.ks[$k]//'&'/%26}
            v=${v//'['/___lbracket___}
            v=${v//']'/___rbracket___}
            multiks[$multin]=$ks
            multivs[$multin]=${impl//"[[$kp]]"/$v}
            while [[ ${multivs[$multin]} == *"[[$kp!X!"*"]]"* ]] do
                kp2=${multivs[$multin]#*"[[$kp!X!"}
                kp2=${kp2%%']]'*}
                v2=
                k2=$kp2$ks
                if [[ ${ar.ks[$k2]} != '' ]] then
                    v2=${ar.ks[$k2]//'&'/%26}
                    v2=${v2//'['/___lbracket___}
                    v2=${v2//']'/___rbracket___}
                fi
                multivs[$multin]=${multivs[$multin]//"[[$kp!X!$kp2]]"/$v2}
            done
            if [[ ${multivs[$multin]} == *"[[$kp!S!"*"]]"* ]] then
                s=${multivs[$multin]#*"[[$kp!S!"}
                s=${s%%']]'*}
                multivs[$multin]=${multivs[$multin]//"[[$kp!S!$s]]"/${v%%$s*}}
            fi
            if [[ ${multivs[$multin]} == *"[[$kp!"*"]]"* ]] then
                l=${multivs[$multin]#*"[[$kp!"}
                l=${l%%']]'*}
                if [[ $ks == _total ]] then
                    multivs[$multin]=${multivs[$multin]//"[[$kp!$l]]"/$l}
                else
                    multivs[$multin]=${multivs[$multin]//"[[$kp!$l]]"/$v}
                fi
            fi
            if [[ ${multivs[$multin]} == *"[["*"!P!$kp]]"* ]] then
                s=${multivs[$multin]%%"!P!"*}
                s=${s#*"[["}
                multivs[$multin]=${multivs[$multin]//"[[$s!P!$kp]]"/${v#*$s}}
            fi
            if [[ ${multivs[$multin]} == *"[[!$kp]]"* ]] then
                multivs[$multin]=${multivs[$multin]//"[[!$kp]]"/$ks}
            fi
            (( multin++ ))
        done
    else
        multiks[$multin]=""
        multivs[$multin]=${impl}
        (( multin++ ))
    fi
}

univ=
function expanduni { # alv aid impl
    typeset alv=$1 aid=$2 impl=$3

    typeset k v f

    univ=
    f=y
    while [[ $impl == *\[*\]* ]] do
        k=${impl#*'['}
        k=${k%%']'*}
        typeset -n ar=aattr.[$alv:$aid]
        v=${ar.ks[$k]//'&'/%26}
        v=${v//'['/___lbracket___}
        v=${v//']'/___rbracket___}
        [[ $v == '' ]] && f=n
        impl=${impl//"[$k]"/${v}}
    done
    impl=${impl//'%'/%25}
    impl=${impl//'|'/%7C}
    impl=${impl//';'/%3B}
    impl=${impl//'<'/%3C}
    impl=${impl//'>'/%3E}
    impl=${impl//'+'/%2B}
    impl=${impl//' '/'+'}
    impl=${impl//___lbracket___/'['}
    impl=${impl//___rbracket___/']'}
    univ=${impl}
    [[ $f == n ]] && univ=''
}

function genschedule { # cid asl ast aip alv aid sid sst isprime
    typeset cid=$1 asl=$2 ast=$3 aip=$4 alv=$5 aid=$6 sid=$7 sst=$8 isprime=$9

    typeset -A mnmap
    typeset mns mn mni impl unit type mrep rep alarm k v s ispec tmode tid sival
    typeset rest prest vorc sev minexpr maxexpr min max ival per v1 v2 f1 f2 n i
    typeset rid nu='@(|+([0-9.-])?(%))' vars alarms vstr astr grp aerr

    for mn in ${slast2mn[$asl:$ast]}; do
        if [[ ${slastmn2sst[$asl:$ast:$mn]} == $sst ]] then
            mns[mnorder[$asl:$ast:$sst:$mn]]=$mn
        fi
    done

    if (( ${#mns[@]} < 1 )) then
        print -u2 MESSAGE no metrics to collect from $aid
        return
    fi

    print -r -u4 "<cfg>"
    print -r -u4 "<version>$VG_S_VERSION</version>"
    print -r -u4 "<jobid>$sid.$aid</jobid>"
    print -r -u4 "<comment>collect stats from $aid</comment>"
    typeset -n ar=aattr.[$alv:$aid]
    if [[ $isprime == y || $failoverinv == y ]] then
        # only send inventory if primary scope
        ispec="$cid|$aid|$aip|${ar.ks[scope_user]}|${ar.ks[scope_pass]}"
        ispec+="|${ar.ks[scope_snmpcommunity]}|$ast|$sst|$asl"
        ispec+="|${ar.ks[scope_invargs]}"
        ispec=$(printf '%#H' "$ispec")
    else
        ispec=
    fi
    tmode=${ar.ks[scope_ticketmode]:-$defaulttmode}
    tid=${ar.ks[scope_ticketid]:-$aid}
    rid=${ar.ks[scope_realid]:-$aid}
    sival=${ar.ks[scope_schedinterval]:-$statinterval}
    if (( sival < 15 )) then
        print -u2 MESSAGE schedinterval for $aid less than 15 sec - reset to 300
        sival=300
    fi
    print -r -u4 "<inv>$ispec</inv>"
    [[ $tid != '' ]] && print -r -u4 "<tid>$tid</tid>"
    print -n -r -u4 "<scope>"
    print -n -r -u4 "<name>$sid</name><type>$sst</type>"
    print -n -r -u4 "<addr>${sid2sip[$sid]}</addr>"
    print -r -u4 "</scope>"
    print -n -r -u4 "<target>"
    print -n -r -u4 "<name>$rid</name><type>$ast</type>"
    print -n -r -u4 "<level>$alv</level><addr>$aip</addr>"
    print -r -u4 "</target>"
    print -r -u4 "<schedule>"
    print -r -u4 "<tz>x</tz>"
    print -r -u4 "<sdate>2006.01.01</sdate>"
    print -r -u4 "<edate>2036.12.01</edate>"
    print -r -u4 "<stime>00:00</stime>"
    print -r -u4 "<etime>24:00</etime>"
    print -r -u4 "<mode>daily</mode>"
    print -r -u4 "<freq><count>1</count><ival>$sival</ival></freq>"
    print -r -u4 "</schedule>"
    print -r -u4 "<tmode>$tmode</tmode>"
    if [[ ${ar.ks[scope_group]} == +([0-9]):* ]] then
        grp=${ar.ks[scope_group]}
        grp=${grp//[\<\>]/_}
        print -r -u4 "<group><id>${grp#*:}</id><max>${grp%%:*}</max></group>"
    fi

    for mni in "${!mns[@]}"; do
        mn=${mns[$mni]}
        [[ $mn == collection ]] && continue

        impl=${slastmn2impl[$asl:$ast:$mn]}
        unit=${slastmn2unit[$asl:$ast:$mn]}
        type=${slastmn2type[$asl:$ast:$mn]}
        mrep=${slastmn2rep[$asl:$ast:$mn]}
        [[ ${aid2status[$alv:$aid:$mn]} == drop ]] && mrep=n
        [[ ${aid2status[$alv:$aid:$mn]} == keep ]] && mrep=y

        if [[ ${ar.ks[scope_implappend_prot${impl%%:*}]} != '' ]] then
            impl+="&${ar.ks[scope_implappend_prot${impl%%:*}]}"
        fi
        if [[ ${ar.ks[scope_implappend_${mn//./_}]} != '' ]] then
            impl+="&${ar.ks[scope_implappend_${mn//./_}]}"
        fi
        if [[ $mn == *.* ]] then
            if [[ ${ar.ks[scope_implappend_${mn%%.*}]} != '' ]] then
                impl+="&${ar.ks[scope_implappend_${mn%%.*}]}"
            fi
        fi

        expandmulti "$mn" "$alv" "$aid" "$impl" # sets multiks multivs multin

        for (( multii = 0; multii < multin; multii++ )) do
            vstr=
            k=${multiks[$multii]}; v=${multivs[$multii]}
            expanduni "$alv" "$aid" "$v" # sets univ
            [[ $univ == '' ]] && continue
            s=
            [[ $k != '' ]] && s=.$k
            if [[
                ${aid2status[$alv:$aid:$mn$s]} == drop || (
                    ${aid2status[$alv:$aid:$mn$s]} == '' &&
                    ${aid2status[$alv:$aid:$mn]} == drop || (
                        ${aid2status[$alv:$aid:$mn]} == '' &&
                        ${aid2status[$alv:$aid:${mn%.*}]} == drop || (
                            ${aid2status[$alv:$aid:${mn%.*}]} == '' &&
                            ${aid2status[$alv:$aid:allstats]} == drop
                        )
                    )
                )
            ]] then
                continue
            fi
            rep=$mrep
            [[ ${aid2status[$alv:$aid:$mn$s]} == drop ]] && rep=n
            [[ ${aid2status[$alv:$aid:$mn$s]} == keep ]] && rep=y
            mnmap[$mn$s]=1
            vstr+="<var>"
            vstr+="<name>$mn$s</name>"
            vstr+="<type>$type</type><unit>$unit</unit>"
            vstr+="<impl>$(printf '%#H' "$univ")</impl><rep>$rep</rep>"
            vstr+="</var>"
            vars[${#vars[@]}]=$vstr
        done

        [[ ${aid2status[$alv:$aid:allstats]} == @(noalarm|drop) ]] && continue

        for (( multii = 0; multii < multin; multii++ )) do
            astr=
            aerr=
            k=${multiks[$multii]}; v=${multivs[$multii]}
            s=
            [[ $k != '' ]] && s=.$k
            if [[
                ${aid2status[$alv:$aid:$mn$s]} == @(noalarm|drop) || (
                    ${aid2status[$alv:$aid:$mn$s]} == '' &&
                    ${aid2status[$alv:$aid:$mn]} == @(noalarm|drop) || (
                        ${aid2status[$alv:$aid:$mn]} == '' &&
                        ${aid2status[$alv:$aid:${mn%.*}]} == @(noalarm|drop)
                    )
                )
            ]] then
                continue
            fi
            [[ ${mnmap[$mn$s]} != 1 ]] && continue
            alarm=${aid2alarm[$alv:$aid:$mn$s]}
            [[ $alarm == '' ]] && alarm=${aid2alarm[$alv:$aid:${mn%.*}]}
            [[ $alarm == '' ]] && alarm=${slastmn2alarm[$asl:$ast:$mn]}
            [[ $alarm == '' ]] && continue
            maxval=
            if [[ $alarm == *%* ]] then
                if [[ ${mn2maxvals[${mn%%.*}]} == +([0-9.-]) ]] then
                    maxval=${mn2maxvals[${mn%%.*}]}
                else
                    if [[ ${ar.ks[si_sz_$mn$s]} == +([0-9.-]) ]] then
                        maxval=${ar.ks[si_sz_$mn$s]}
                    else
                        print -u2 MESSAGE no max value for $aid / $mn$s
                        continue
                    fi
                fi
                if [[ $maxval == 0*([.0]) ]] then
                    print -u2 MESSAGE zero max value for $aid / $mn$s
                    continue
                fi
            fi
            astr+="<alarm>"
            astr+="<alarmid>$mn$s</alarmid>"
            astr+="<cond>Threshold Alarm</cond>"
            rest=${alarm//[ 	]/}
            if [[ $rest != '' && $rest != *CLEAR:* ]] then
                rest="$rest:CLEAR:5:1/1"
            fi
            while [[ $rest != '' ]] do
                prest=$rest
                vorc=${rest%%:*}
                rest=${rest##"$vorc"?(:)}
                case $vorc in
                CLEAR)
                    sev=${rest%%:*}
                    rest=${rest##"$sev"?(:)}
                    minexpr=${rest%%:*}
                    rest=${rest##"$minexpr"?(:)}
                    min=${minexpr%/*}
                    per=${minexpr##"$min"?(/)}
                    astr+="<clear>"
                    astr+="<cond>Threshold Alarm Cleared</cond>"
                    astr+="<sev>$sev</sev>"
                    astr+="<min><count>$min</count><per>$per</per></min>"
                    astr+="</clear>"
                    ;;
                *)
                    v1= v2= f1= f2=
                    if [[ $vorc == +([0-9.%-]) ]] then
                        v1=$vorc v2=$vorc
                        f1=i f2=i
                    else
                        case $vorc in
                        "("*")") f1=e f2=e ;;
                        "("*"]") f1=e f2=i ;;
                        "["*")") f1=i f2=e ;;
                        "["*"]") f1=i f2=i ;;
                        ">="*) f1=i v1=${vorc:2} ;;
                        ">"*) f1=e v1=${vorc:1} ;;
                        "<="*) f2=i v2=${vorc:2} ;;
                        "<"*) f2=e v2=${vorc:1} ;;
                        "="*) f1=i f2=i v1=${vorc:1} v2=${vorc:1} ;;
                        *)
                            print -u2 ERROR bad alarm range: $vorc
                            aerr=y
                            break
                            ;;
                        esac
                        if [[ $v1 == '' && $v2 == '' ]] then
                            vorc=${vorc:1}
                            vorc=${vorc%?}
                            n=${#vorc}
                            for (( i = 1; i < n; i++ )) do
                                [[ ${vorc:$i:1} == - ]] && break
                            done
                            if [[ $i == $n ]] then
                                print -u2 ERROR bad alarm range: $vorc
                                break
                            fi
                            v1=${vorc:0:$i} v2=${vorc:$(( i + 1 ))}
                        fi
                    fi
                    if [[ $v1 != $nu || $v2 != $nu ]] then
                        print -u2 ERROR bad alarm range numbers: $v1 $v2
                        aerr=y
                        break
                    fi
                    if [[ $v1 == *%* ]] then
                        (( v1 = maxval * (${v1%'%'*} / 100.0) ))
                    fi
                    if [[ $v2 == *%* ]] then
                        (( v2 = maxval * (${v2%'%'*} / 100.0) ))
                    fi
                    sev=${rest%%:*}
                    rest=${rest##"$sev"?(:)}
                    minexpr=${rest%%:*}
                    rest=${rest##"$minexpr"?(:)}
                    min=${minexpr%/*}
                    per=${minexpr##"$min"?(/)}
                    maxexpr=${rest%%:*}
                    rest=${rest##"$maxexpr"?(:)}
                    max=${maxexpr%/*}
                    ival=${maxexpr##"$max"?(/)}
                    astr+="<expr>"
                    astr+="<type>nrange</type>"
                    astr+="<var>$mn$s</var>"
                    [[ $maxval != '' ]] && astr+="<cap>$maxval</cap>"
                    [[ $v1 != '' ]] && \
                    astr+="<${f1}min><num>$v1</num></${f1}min>"
                    [[ $v2 != '' ]] && \
                    astr+="<${f2}max><num>$v2</num></${f2}max>"
                    astr+="<sev>$sev</sev>"
                    astr+="<min><count>$min</count><per>$per</per></min>"
                    astr+="<max><count>$max</count><ival>$ival</ival></max>"
                    astr+="</expr>"
                    ;;
                esac
                if [[ $prest == $rest ]] then
                    print -u2 ERROR no progress in $alarm
                    aerr=y
                    break
                fi
            done
            astr+="</alarm>"
            [[ $aerr != y ]] && alarms[${#alarms[@]}]=$astr
        done
    done

    print -r -u4 "<vars>"
    for vstr in "${vars[@]}"; do
        print -u4 "$vstr"
    done
    print -r -u4 "</vars>"
    print -r -u4 "<alarms>"
    for astr in "${alarms[@]}"; do
        print -u4 "$astr"
    done
    print -r -u4 "</alarms>"

    alarm=
    for mni in "${!mns[@]}"; do
        mn=${mns[$mni]}
        [[ $mn != collection ]] && continue
        alarm=${aid2alarm[$alv:$aid:$mn]}
        [[ $alarm == '' ]] && alarm=${aid2alarm[$alv:$aid:${mn%.*}]}
        [[ $alarm == '' ]] && alarm=${slastmn2alarm[$asl:$ast:$mn]}
        break
    done
    alarm=${alarm//'%'/}

    if [[
        $alarm != '' && ${aid2status[$alv:$aid:$mn]} != @(noalarm|drop)
    ]] then
        print -r -u4 "<errors>"
        print -r -u4 "<alarmid>collection</alarmid>"
        print -r -u4 "<cond>Collection Alarm</cond>"
        rest=${alarm//[ 	]/}
        if [[ $rest != '' && $rest != *CLEAR:* ]] then
            rest="$rest:CLEAR:5:1/1"
        fi
        while [[ $rest != '' ]] do
            prest=$rest
            vorc=${rest%%:*}
            rest=${rest##"$vorc"?(:)}
            case $vorc in
            CLEAR)
                sev=${rest%%:*}
                rest=${rest##"$sev"?(:)}
                minexpr=${rest%%:*}
                rest=${rest##"$minexpr"?(:)}
                min=${minexpr%/*}
                per=${minexpr##"$min"?(/)}
                print -r -u4 "<clear>"
                print -r -u4 "<cond>Collection Alarm Cleared</cond>"
                print -r -u4 "<sev>$sev</sev>"
                print \
                    -r -u4 \
                "<min><count>$min</count><per>$per</per></min>"
                print -r -u4 "</clear>"
                ;;
            *)
                v1= v2= f1= f2=
                if [[ $vorc == +([0-9.%-]) ]] then
                    v1=$vorc v2=$vorc
                    f1=i f2=i
                else
                    case $vorc in
                    "("*")") f1=e f2=e ;;
                    "("*"]") f1=e f2=i ;;
                    "["*")") f1=i f2=e ;;
                    "["*"]") f1=i f2=i ;;
                    ">="*) f1=i v1=${vorc:2} ;;
                    ">"*) f1=e v1=${vorc:1} ;;
                    "<="*) f2=i v2=${vorc:2} ;;
                    "<"*) f2=e v2=${vorc:1} ;;
                    "="*) f1=i f2=i v1=${vorc:1} v2=${vorc:1} ;;
                    *)
                        print -u2 ERROR bad alarm range: $vorc
                        break
                        ;;
                    esac
                    if [[ $v1 == '' && $v2 == '' ]] then
                        vorc=${vorc:1}
                        vorc=${vorc%?}
                        n=${#vorc}
                        for (( i = 1; i < n; i++ )) do
                            [[ ${vorc:$i:1} == - ]] && break
                        done
                        if [[ $i == $n ]] then
                            print -u2 ERROR bad alarm range: $vorc
                            break
                        fi
                        v1=${vorc:0:$i} v2=${vorc:$(( i + 1 ))}
                    fi
                fi
                if [[ $v1 == *%* ]] then
                    (( v1 = maxval * (${v1%'%'*} / 100.0) ))
                fi
                if [[ $v2 == *%* ]] then
                    (( v2 = maxval * (${v2%'%'*} / 100.0) ))
                fi
                sev=${rest%%:*}
                rest=${rest##"$sev"?(:)}
                minexpr=${rest%%:*}
                rest=${rest##"$minexpr"?(:)}
                min=${minexpr%/*}
                per=${minexpr##"$min"?(/)}
                maxexpr=${rest%%:*}
                rest=${rest##"$maxexpr"?(:)}
                max=${maxexpr%/*}
                ival=${maxexpr##"$max"?(/)}
                print -r -u4 "<expr>"
                print -r -u4 "<type>nrange</type>"
                print -r -u4 "<var>collection</var>"
                [[ $v1 != '' ]] && \
                print -r -u4 "<${f1}min><num>$v1</num></${f1}min>"
                [[ $v2 != '' ]] && \
                print -r -u4 "<${f2}max><num>$v2</num></${f2}max>"
                print -r -u4 "<sev>$sev</sev>"
                print \
                    -r -u4 \
                "<min><count>$min</count><per>$per</per></min>"
                print \
                    -r -u4 \
                "<max><count>$max</count><ival>$ival</ival></max>"
                print -r -u4 "</expr>"
                ;;
            esac
            if [[ $prest == $rest ]] then
                print -u2 ERROR no progress in $alarm
                break
            fi
        done
        print -r -u4 "</errors>"
    fi

    print -r -u4 "</cfg>"
}

function queueins {
    typeset file=$1 rfile=$2 ofile=$3
    typeset ts pfile

    ts=$(printf '%(%Y%m%d-%H%M%S)T')
    pfile=cm.$ts.$file.${rfile//./_X_X_}.$sn.$sn.$RANDOM.full.xml
    pfile=$VG_DSYSTEMDIR/incoming/cm/$pfile
    (
        print "<a>full</a>"
        print "<u>$sn</u>"
        print "<f>"
        cat $ofile
        print "</f>"
    ) > $pfile.tmp && mv $pfile.tmp $pfile
}

function queuerem {
    typeset file=$1 rfile=$2
    typeset ts pfile

    ts=$(printf '%(%Y%m%d-%H%M%S)T')
    pfile=cm.$ts.$file.${rfile//./_X_X_}.$sn.$sn.$RANDOM.remove.xml
    pfile=$VG_DSYSTEMDIR/incoming/cm/$pfile
    (
        print "<a>remove</a>"
        print "<u>$sn</u>"
        print "<f>"
        print "</f>"
    ) > $pfile.tmp && mv $pfile.tmp $pfile
}

alarmcount=0
function genalarm { # $1 = type $2 = alarmid $3 = text
    typeset type lv id sev txt msg ti dat

    type=$1
    lv=$2
    id=$3
    sev=$4
    txt=$5
    msg="VG $type $txt"
    ti=$(printf '%(%#)T')
    dat=$(printf '%(%Y.%m.%d.%H.%M.%S)T')
    (
        print "<alarm>"
        print "<v></v><aid>scopehb</aid>"
        print "<ccid></ccid><st></st><sm></sm>"
        print "<vars></vars><di></di>"
        print "<hi></hi>"
        print "<tc></tc><ti>$ti</ti>"
        print "<tp>$type</tp><so></so><pm></pm>"
        print "<lv1>$lv</lv1><id1>$id</id1><lv2></lv2><id2></id2>"
        print "<tm></tm><sev>$sev</sev>"
        print "<txt>$msg</txt><com></com>"
        print "</alarm>"
    ) > $VG_DSYSTEMDIR/incoming/alarm/alarms.${dat}.${alarmcount}HB$$.tmp
    mv \
        $VG_DSYSTEMDIR/incoming/alarm/alarms.${dat}.${alarmcount}HB$$.tmp \
    $VG_DSYSTEMDIR/incoming/alarm/alarms.${dat}.${alarmcount}HB$$.xml
    (( alarmcount++ ))
}

. vg_hdr

nowait=n
while getopts -a vg_scopemgr "$usage" opt; do
    case $opt in
    200) checkhb=y ;;
    201) validate=y ;;
    202) scopestate=y ;;
    203) scopemap=y ;;
    204) reinv=y; riaid=$OPTARG ;;
    205) rebalance=y; rbmincost=$OPTARG ;;
    206) getfiles=y; gfaid=$OPTARG ;;
    300) noauto=y ;;
    301) nopush=y ;;
    302) nowait=y ;;
    400) admindown=y; dsid=$OPTARG ;;
    401) adminup=y; usid=$OPTARG ;;
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) showusage; exit 1 ;;
    esac
done
shift $OPTIND-1

if [[ -f $VGMAINDIR/dpfiles/config.sh ]] then
    . $VGMAINDIR/dpfiles/config.sh
fi
export CASEINSENSITIVE=${CASEINSENSITIVE:-n}
export statinterval=${STATINTERVAL:-300}
export maxscopedelay=${MAXSCOPEDELAY:-$(( $statinterval + 30 ))}
export defaulttmode=${DEFAULTTMODE:-keep}
export standbymode=${STANDBYMODE:-n}
export failoverinv=${FAILOVERINV:-n}
export usescopeinvkeys=${USESCOPEINVKEYS:-n}
float transrate=${TRANSRATE:-1000000}

export SCOPEDIR=$VG_DSCOPESDIR
[[ $HOME == /?* ]] && SCOPEDIR=${SCOPEDIR##"$HOME"?(/)}

sn=$VG_SYSNAME

if [[ $standbymode == y ]] then
    tw -i -H -d $VGMAINDIR/incoming/health -e type==REG rm -f
    for nfile in $VGMAINDIR/incoming/autoinv/*; do
        [[ ! -f $nfile ]] && continue
        rm -f $nfile
    done

    exit 0
fi

nolock=n
[[ $scopestate == y || $scopemap == y ]] && nolock=y

if [[ $nolock == n ]] then
    exec 4>&2
    lockfile=$VG_DSYSTEMDIR/lock.scopemgr
    exitfile=$VG_DSYSTEMDIR/exit.scopemgr
    [[ -f $exitfile ]] && exit 0
    set -o noclobber
    while ! command exec 3> $lockfile; do
        if kill -0 $(< $lockfile); then
            [[ $nowait == y ]] && exit 0
        elif [[ $(fuser $lockfile) != '' ]] then
            [[ $nowait == y ]] && exit 0
        else
            rm -f $lockfile
            [[ $nowait == y ]] && exit 0
        fi
        if [[ -f $exitfile ]] then
            exit 0
        fi
        print -u4 waiting for another instance to exit - pid: $(< ${lockfile})
        sleep 1
    done 2> /dev/null
    print -u3 $$
    set +o noclobber
    exec 4>&-
    trap "rm -f $lockfile" HUP QUIT TERM KILL EXIT
fi

cd $VGMAINDIR/dpfiles/inv || exit 1
mkdir -p scope/state $VGMAINDIR/tmp/scope

IFS='|'

tmpdir=$VGMAINDIR/tmp/scope
rm -rf $tmpdir
mkdir -p $tmpdir

cts=$(printf '%(%#)T')

if [[ $scopestate == y ]] then
    print -r "name|type|hb|count|cost|state"
    [[ ! -f scope/state/scopedata.txt ]] && exit 0
    while read -r sid sst ts cn cost state; do
        print -r "$sid|$sst|$(( cts - ts ))s ago|$cn|$cost|$state"
    done < scope/state/scopedata.txt

    exit 0
fi

if [[ $scopemap == y ]] then
    print -r "asset|asset type|scope type|current scope|failed over scope"
    for i in scope/state/*-info.txt; do
        [[ ! -f $i ]] && continue
        while read -r -u3 alv aid ast sst sid fosid; do
            print -r "$aid|$ast|$sst|$sid|$fosid"
        done 3< $i | sort -t'|'
    done

    exit 0
fi

if [[ $reinv == y ]] then
    typeset -A risids
    for sifile in scope/state/*-info.txt; do
        [[ ! -f $sifile ]] && continue
        while read -r -u3 alv aid ast sst sid fosid; do
            if [[ $aid == $riaid && $sid != '' ]] then
                risids[$sid]=1
            fi
        done 3< $sifile
        (( ${#risids[@]} > 0 )) && break
    done

    if (( ${#risids[@]} == 0 )) then
        print -u2 ERROR cannot find scope for asset $riaid
        exit 0
    fi

    for risid in "${!risids[@]}"; do
        print -u2 MESSAGE queueing $riaid for reinv on scope $risid

        typeset -A sid2sip sid2user sid2pass
        # find and load scope info
        for ifile in scope/$sn-*-inv.txt; do
            [[ ! -f $ifile ]] && continue
            egrep "^node\|o\|$risid\|" $ifile \
            | while read -r -u3 rt slv sid k v; do
                case $k in
                scope_ip)
                    sid2sip[$sid]=$v
                    ;;
                scope_dir)
                    sid2dir[$sid]=$v
                    ;;
                scope_user)
                    sid2user[$sid]=$v
                    ;;
                scope_pass)
                    sid2pass[$sid]=$v
                    ;;
                esac
            done 3<&0-
        done

        user=${sid2user[$risid]}
        ip=${sid2sip[$risid]}
        if [[ $user == '' || $ip == '' ]] then
            print -u2 ERROR cannot find info on scope $risid
            exit 1
        fi
        dir=${sid2dir[$risid]}

        cmd="${dir:-$SCOPEDIR}/etc/schedulejob reinv $riaid"
        timedrun 120 ssh \
            -C -o ConnectTimeout=15 -o ServerAliveInterval=15 \
            -o ServerAliveCountMax=2 -o StrictHostKeyChecking=no \
            $user@$ip "$cmd" \
        | read res
        if [[ $res == ok ]] then
            print -u2 MESSAGE queued $riaid for reinv on scope $risid
        else
            print -u2 ERROR cannot queue $riaid for reinv on scope $risid
        fi
    done

    exit 0
fi

if [[ $getfiles == y ]] then
    typeset -A gfsids
    for sifile in scope/state/*-info.txt; do
        [[ ! -f $sifile ]] && continue
        while read -r -u3 alv aid ast sst sid fosid; do
            if [[ $aid == $gfaid && $sid != '' ]] then
                gfsids[$sid]=1
            fi
        done 3< $sifile
        (( ${#gfsids[@]} > 0 )) && break
    done

    if (( ${#gfsids[@]} == 0 )) then
        print -u2 ERROR cannot find scope for asset $gfaid
        exit 0
    fi

    mkdir -p $VG_DSYSTEMDIR/tmp/getfiles.$gfaid
    if [[ ! -d $VG_DSYSTEMDIR/tmp/getfiles.$gfaid ]] then
        print -u2 ERROR cannot find dir $VG_DSYSTEMDIR/tmp/getfiles.$gfaid
        continue
    fi
    rm -rf $VG_DSYSTEMDIR/tmp/getfiles.$gfaid/*

    for gfsid in "${!gfsids[@]}"; do
        print -u2 MESSAGE copying files for $gfaid from scope $gfsid

        typeset -A sid2sip sid2user sid2pass
        # find and load scope info
        for ifile in scope/$sn-*-inv.txt; do
            [[ ! -f $ifile ]] && continue
            egrep "^node\|o\|$gfsid\|" $ifile \
            | while read -r -u3 rt slv sid k v; do
                case $k in
                scope_ip)
                    sid2sip[$sid]=$v
                    ;;
                scope_dir)
                    sid2dir[$sid]=$v
                    ;;
                scope_user)
                    sid2user[$sid]=$v
                    ;;
                scope_pass)
                    sid2pass[$sid]=$v
                    ;;
                esac
            done 3<&0-
        done

        user=${sid2user[$gfsid]}
        ip=${sid2sip[$gfsid]}
        if [[ $user == '' || $ip == '' ]] then
            print -u2 ERROR cannot find info on scope $gfsid
            exit 1
        fi
        dir=${sid2dir[$gfsid]}

        cmd="${dir:-$SCOPEDIR}/etc/schedulejob getfiles $gfaid"
        timedrun 120 ssh \
            -C -o ConnectTimeout=15 -o ServerAliveInterval=15 \
            -o ServerAliveCountMax=2 -o StrictHostKeyChecking=no \
            $user@$ip "$cmd" \
        | (
            while read line; do
                [[ $line == TARSTARTSNOW ]] && break
            done
            cd $VG_DSYSTEMDIR/tmp/getfiles.$gfaid && tar xf -
        )
        if [[ $? == 0 ]] then
            print -u2 MESSAGE \
                copied files for $gfaid from scope $gfsid \
            in $VG_DSYSTEMDIR/tmp/getfiles.$gfaid/$gfsid
        else
            print -u2 ERROR cannot copy files for $gfaid from scope $gfsid
        fi
    done

    exit 0
fi

print -u2 MESSAGE begin - $(printf '%T')

if [[ $admindown == y || $adminup == y ]] then
    print -u2 MESSAGE begin - performing admin up/down

    typeset -A sid2sst sid2ts sid2cn sid2cost sid2state

    # load scope state
    [[ ! -f scope/state/scopedata.txt ]] && touch scope/state/scopedata.txt
    while read -r sid sst ts cn cost state; do
        sid2sst[$sid]=$sst
        sid2ts[$sid]=$ts
        sid2cn[$sid]=$cn
        sid2cost[$sid]=$cost
        sid2state[$sid]=$state
    done < scope/state/scopedata.txt

    uflag=n
    if [[ $dsid != '' && ${sid2state[$dsid]} != '' ]] then
        print -u2 MESSAGE setting state of scope $dsid to admindown
        sid2state[$dsid]=admindown
        uflag=y
    fi
    if [[ $usid != '' && ${sid2state[$usid]} != '' ]] then
        print -u2 MESSAGE setting state of scope $usid to up
        sid2state[$usid]=up
        uflag=y
    fi

    # save scope state
    if [[ $uflag == y ]] then
        for sid in "${!sid2ts[@]}"; do
            spec="$sid|${sid2sst[$sid]}|${sid2ts[$sid]}|${sid2cn[$sid]}"
            spec+="|${sid2cost[$sid]}|${sid2state[$sid]}"
            print -r "$spec"
        done | sort > scope/state/scopedata.txt.tmp \
        && mv scope/state/scopedata.txt.tmp scope/state/scopedata.txt
    fi

    unset sid2sst sid2ts sid2cn sid2cost sid2state

    print -u2 MESSAGE end - performing admin up/down
fi

if [[ $checkhb == y ]] then
    print -u2 MESSAGE begin - checking scope heartbeats

    typeset -A sid2sst sid2ts sid2cn sid2cost sid2state
    typeset -l sidl

    # load scope state
    [[ ! -f scope/state/scopedata.txt ]] && touch scope/state/scopedata.txt
    while read -r sid sst ts cn cost state; do
        sid2sst[$sid]=$sst
        sid2ts[$sid]=$ts
        sid2cn[$sid]=$cn
        sid2cost[$sid]=$cost
        sid2state[$sid]=$state
    done < scope/state/scopedata.txt

    # update timestamps
    tw -i -H -d $VGMAINDIR/incoming/health \
        -e type==REG -e 'action: printf ("%d|%s\n", mtime, path)' \
    | while read -u3 -r mtime path; do
        [[ $path == *.tmp ]] && continue
        cat $path | while read -r line; do
            [[ $line != "<hb>"*"</hb>" ]] && continue
            sid=${line#'<hb>'}
            sid=${sid%'</hb>'}
            if [[ $CASEINSENSITIVE == y ]] then
                sidl=$sid
                sid=$sidl
            fi
            if [[ ${sid2sst[$sid]} != '' ]] then
                if (( sid2ts[$sid] < mtime )) then
                    sid2ts[$sid]=$mtime
                fi
            fi
        done
        rm $path
    done 3<&0-

    uflag=n
    # update up / down status
    for sid in "${!sid2ts[@]}"; do
        [[ ${sid2ts[$sid]} == '' ]] && continue
        if (( cts - sid2ts[$sid] > maxscopedelay )) then
            if [[ ${sid2state[$sid]} != *down ]] then
                print -u2 MESSAGE scope $sid is down
                sid2state[$sid]=down
                genalarm ALARM o $sid 1 "scope is down"
                uflag=y
            fi
        else
            if [[ ${sid2state[$sid]} == down ]] then
                print -u2 MESSAGE scope $sid is up
                sid2state[$sid]=up
                genalarm CLEAR o $sid 5 "scope is up"
                uflag=y
            fi
        fi
    done

    # save scope state
    for sid in "${!sid2ts[@]}"; do
        spec="$sid|${sid2sst[$sid]}|${sid2ts[$sid]}|${sid2cn[$sid]}"
        spec+="|${sid2cost[$sid]}|${sid2state[$sid]}"
        print -r "$spec"
    done | sort > scope/state/scopedata.txt.tmp \
    && mv scope/state/scopedata.txt.tmp scope/state/scopedata.txt

    unset sid2sst sid2ts sid2cn sid2cost sid2state

    print -u2 MESSAGE end - checking scope heartbeats
fi

print -u2 MESSAGE begin - updating scope data

typeset -A mn2maxvals

typeset -A slast2mn slastmn2rep slastmn2sst slastmn2impl
typeset -A slastmn2unit slastmn2type slastmn2alarm slast2sst mnorder

typeset -A sid2sip sid2dir sid2user sid2pass sid2weights sid2bits sidbits2nets
typeset -A sidbits2weights sid2sst sst2sid sid2ts sid2cn sid2cost sid2state
typeset -A sid2tags

typeset -A alist slist glist

typeset -A cmap
typeset -l cid

tflag=n

# load stat list
sort -u ../stat/statlist.txt | egrep -v '^#|^$' \
| while read -r -u3 mn clabel flabel aggr maxval j; do
    [[ $mn == '' || $maxval == '' ]] && continue
    mn2maxvals[$mn]=$maxval
done 3<&0-

# load scope params
sort -u ../stat/parameter.txt | egrep -v '^#|^$' \
| while read -r -u3 sl ast sst mn rep impl unit type alarm palarm estat; do
    [[ $sl == '' || $ast == '' || $sst == '' || $mn == '' ]] && continue
    [[ ${slast2mn[$sl:$ast]} != '' ]] && slast2mn[$sl:$ast]+="|"
    slast2mn[$sl:$ast]+="$mn"
    slastmn2rep[$sl:$ast:$mn]=$rep
    slastmn2sst[$sl:$ast:$mn]=$sst
    slastmn2impl[$sl:$ast:$mn]=$impl
    slastmn2unit[$sl:$ast:$mn]=$unit
    slastmn2type[$sl:$ast:$mn]=$type
    slastmn2alarm[$sl:$ast:$mn]=$alarm
    if [[ '|'${slast2sst[$sl:$ast]}'|' != *'|'$sst'|'* ]] then
        [[ ${slast2sst[$sl:$ast]} != '' ]] && slast2sst[$sl:$ast]+="|"
        slast2sst[$sl:$ast]+="$sst"
    fi
    mnorder[$sl:$ast:$sst:$mn]=${#mnorder[@]}
done 3<&0-

# find and load scope info
for ifile in scope/$sn-*-inv.txt; do
    [[ ! -f $ifile ]] && continue
    if [[ $ifile -nt scope/state/threshold.txt ]] then
        tflag=y
    fi
    ibase=${ifile##*/}
    egrep '^node\|o\|.*\|scope_sysfunc\|scope$' $ifile \
    | while read -r rt slv sid k v; do
        egrep "^node\|$slv\|$sid\|" $ifile \
        | while read -r -u3 rt slv sid k v; do
            case $k in
            scope_ip)
                sid2sip[$sid]=$v
                ;;
            scope_dir)
                sid2dir[$sid]=$v
                ;;
            scope_user)
                sid2user[$sid]=$v
                ;;
            scope_pass)
                sid2pass[$sid]=$v
                ;;
            scope_weight)
                sid2weights[$sid]=$v
                ;;
            scope_nets)
                sid2bits[$sid]=
                for net in ${v//' '/'|'}; do
                    ip=${net%%/*}
                    rest=${net##"$ip"?(/)}
                    bn=${rest%/*}
                    weight=${rest##"$bn"?(/)}
                    bn=${bn:-24}
                    weight=${weight:-1.0}
                    bm=$(ip2bm $ip)
                    if [[ $bm == '' ]] then
                        print -u2 ERROR empty mask: $sid/${ipuser2sid[$ipuser]}
                        continue
                    fi
                    [[ ${sid2bits[$sid]} != '' ]] && sid2bits[$sid]+="|"
                    bits=${bm:0:$(( bn + 1 ))}
                    sid2bits[$sid]+=$bits
                    sidbits2nets[$sid:$bits]=$net
                    sidbits2weights[$sid:$bits]=$weight

                done
                ;;
            scope_systype)
                sid2sst[$sid]=$v
                [[ ${sst2sid[$v]} != '' ]] && sst2sid[$v]+="|"
                sst2sid[$v]+="$sid"
                sid2ts[$sid]=0
                sid2cn[$sid]=0
                sid2cost[$sid]=0
                sid2state[$sid]=up
                ;;
            scope_tags)
                sid2tags[$sid]=$v
                ;;
            esac
        done 3<&0-
    done
done

typeset -A ipuser2sid
for sid in "${!sid2sip[@]}"; do
    ipuser=${sid2sip[$sid]}:${sid2user[$sid]}
    if [[ ${ipuser2sid[$ipuser]} != '' ]] then
        print -u2 ERROR duplicate scope entries: $sid/${ipuser2sid[$ipuser]}
        sid2sip[$sid]=
        sid2dir[$sid]=
        sid2user[$sid]=
        sid2pass[$sid]=
        sid2weights[$sid]=
        sid2bits[$sid]=
    fi
    ipuser2sid[$ipuser]=$sid
done
unset ipuser2sid

# load customer map
egrep -v '^#|^$' $VGMAINDIR/dpfiles/inv/customer.txt \
| while read -r -u3 cid cname j; do
    cmap[$cid]=$cname
done 3<&0-

# load scope state
[[ ! -f scope/state/scopedata.txt ]] && touch scope/state/scopedata.txt
while read -r sid sst ts cn cost state; do
    if [[ ${sid2sip[$sid]} == '' ]] then
        print -u2 WARNING no scope definition for $sid
        continue
    fi
    sid2sst[$sid]=$sst
    sid2ts[$sid]=$ts
    sid2cn[$sid]=$cn
    sid2cost[$sid]=$cost
    sid2state[$sid]=$state
done < scope/state/scopedata.txt

if [[ $noauto != y ]] then
    print -u2 MESSAGE begin - collecting new autoinv records

    for nfile in $VGMAINDIR/incoming/autoinv/*; do
        [[ ! -f $nfile ]] && continue
        [[ $nfile == */autoinv.*.xml ]] && cat $nfile
        rm $nfile
    done > $tmpdir/autoinv.txt
    sed 's/^.*|cid=//' $tmpdir/autoinv.txt | sort -u | while read -r cid; do
        print -u2 MESSAGE collecting new autoinv records for customer $cid

        if [[ ${cmap[$cid]} == '' ]] then
            print -u2 ERROR autoinv cid $cid not a customer id
            continue
        fi

        ifile=scope/$sn-$cid-inv.txt
        afile=scope/auto-$sn-$cid-inv.txt
        tafile1=$tmpdir/auto-$sn-$cid-inv.txt.tmp1
        tafile2=$tmpdir/auto-$sn-$cid-inv.txt.tmp2
        tafile3=$tmpdir/auto-$sn-$cid-inv.txt.tmp3

        [[ ! -f $ifile ]] && continue

        set -f
        unset pnodes pedges pmaps; typeset -A pnodes pedges pmaps
        egrep "\|cid=$cid\$" $tmpdir/autoinv.txt | while read -r -u3 line; do
            set -A ls -- $line
            case ${ls[0]} in
            node)
                ls[2]=${ls[2]//[!.a-zA-Z0-9_:-]/}
                ls[3]=${ls[3]//[!.a-zA-Z0-9_:-]/_}
                if [[ ${ls[1]} == '' || ${ls[2]} == '' || ${ls[3]} == '' ]] then
                    print -u2 ERROR missing fields in autoinv file for $cid
                    continue
                fi
                ls[5]=${ls[5]#*=}
                ls[5]=${ls[5]//[!.a-zA-Z0-9_:-]/}
                key=${ls[5]}@${ls[1]}@${ls[2]}@${ls[3]}
                val=${ls[4]}
                pnodes[$key]=$val
                pnodes[${ls[5]}@o@${ls[5]}@si_timestamp]=$cts
                ;;
            edge)
                ls[2]=${ls[2]//[!.a-zA-Z0-9_:-]/}
                ls[4]=${ls[4]//[!.a-zA-Z0-9_:-]/}
                ls[5]=${ls[5]//[!.a-zA-Z0-9_:-]/_}
                if [[
                    ${ls[1]} == '' || ${ls[2]} == '' || ${ls[3]} == '' ||
                    ${ls[4]} == ''
                ]] then
                    print -u2 ERROR missing fields in autoinv file for $cid
                    continue
                fi
                ls[7]=${ls[7]#*=}
                ls[7]=${ls[7]//[!.a-zA-Z0-9_:-]/}
                key=${ls[7]}@${ls[1]}@${ls[2]}@${ls[3]}@${ls[4]}@${ls[5]}
                val=${ls[6]}
                pedges[$key]=$val
                pnodes[${ls[7]}@o@${ls[7]}@si_timestamp]=$cts
                ;;
            map)
                ls[2]=${ls[2]//[!.a-zA-Z0-9_:-]/}
                ls[4]=${ls[4]//[!.a-zA-Z0-9_:-]/}
                ls[5]=${ls[5]//[!.a-zA-Z0-9_:-]/_}
                if [[
                    ${ls[1]} == '' || ${ls[2]} == '' || ${ls[3]} == '' ||
                    ${ls[4]} == ''
                ]] then
                    print -u2 ERROR missing fields in autoinv file for $cid
                    continue
                fi
                ls[7]=${ls[7]#*=}
                ls[7]=${ls[7]//[!.a-zA-Z0-9_:-]/}
                key=${ls[7]}@${ls[1]}@${ls[2]}@${ls[3]}@${ls[4]}@${ls[5]}
                val=${ls[6]}
                pmaps[$key]=$val
                pnodes[${ls[7]}@o@${ls[7]}@si_timestamp]=$cts
                ;;
            esac
        done 3<&0-
        set +f
        (
            for k1 in "${!pnodes[@]}"; do
                print -r "|aid=${k1%%@*}|"
            done
            for k1 in "${!pedges[@]}"; do
                print -r "|aid=${k1%%@*}|"
            done
            for k1 in "${!pmaps[@]}"; do
                print -r "|aid=${k1%%@*}|"
            done
        ) | sort -u > $tafile1
        (
            for k1 in "${!pnodes[@]}"; do
                v=${pnodes[$k1]}
                aid=${k1%%@*}; k1=${k1#"$aid"@}
                lv=${k1%%@*}; k1=${k1#"$lv"@}
                id=${k1%%@*}; k1=${k1#"$id"@}
                k=${k1%%@*}
                print -r "node|$lv|$id|$k|$v|aid=$aid"
            done
            for k1 in "${!pedges[@]}"; do
                v=${pedges[$k1]}
                aid=${k1%%@*}; k1=${k1#"$aid"@}
                lv1=${k1%%@*}; k1=${k1#"$lv1"@}
                id1=${k1%%@*}; k1=${k1#"$id1"@}
                lv2=${k1%%@*}; k1=${k1#"$lv2"@}
                id2=${k1%%@*}; k1=${k1#"$id2"@}
                k=${k1%%@*}
                print -r "edge|$lv1|$id1|$lv2|$id2|$k|$v|aid=$aid"
            done
            for k1 in "${!pmaps[@]}"; do
                v=${pmaps[$k1]}
                aid=${k1%%@*}; k1=${k1#"$aid"@}
                lv1=${k1%%@*}; k1=${k1#"$lv1"@}
                id1=${k1%%@*}; k1=${k1#"$id1"@}
                lv2=${k1%%@*}; k1=${k1#"$lv2"@}
                id2=${k1%%@*}; k1=${k1#"$id2"@}
                k=${k1%%@*}
                print -r "map|$lv1|$id1|$lv2|$id2|$k|$v|aid=$aid"
            done
        ) | sort -u > $tafile2
        if [[ -f $ifile ]] then
            egrep '^node\|' $ifile \
            | sed -e 's/^node|[^|]*|//' -e 's/|.*$//' -e 's/^/|aid=/' \
            | sort -u | sed 's/$/|/' > $tafile3
        else
            > $tafile3
        fi
        (
            if [[ -f $afile && -s $tafile3 ]] then
                sed 's/$/|/' $afile | if [[ -s $tafile1 ]] then
                    fgrep -v -f $tafile1 | fgrep -f $tafile3
                else
                    fgrep -f $tafile3
                fi | sed 's/|$//'
            fi
            cat $tafile2
        ) | sort -u > $afile.tmp
        if ! cmp $afile.tmp $afile > /dev/null 2>&1; then
            if [[ -f $afile ]] then
                egrep -v "$dontcareattr" $afile
            fi > $afile.tmp1
            egrep -v "$dontcareattr" $afile.tmp > $afile.tmp2
            if cmp $afile.tmp1 $afile.tmp2 > /dev/null 2>&1; then
                touch -r $afile $afile.tmp
            fi
            rm -f $afile.tmp1 $afile.tmp2
            mv $afile.tmp $afile
            print -u2 MESSAGE queuing autoinv file for customer $cid
            queueins scope auto-$sn-$cid-inv.txt $afile
            tflag=y
        else
            rm $afile.tmp
        fi
    done

    for file in scope/auto-$sn-*; do
        [[ ! -f $file ]] && continue
        if [[ ! -f scope/$sn-${file##*/auto-$sn-} ]] then
            print -u2 MESSAGE deleting auto file ${file##*/}
            queuerem scope ${file##*/}
        fi
    done
    print -u2 MESSAGE end - collecting new autoinv records
fi

if [[ $tflag == y || ../stat/threshold.txt -nt scope/state/threshold.txt ]] then
    print -u2 MESSAGE begin - generating threshold file

    ddir=$VGMAINDIR/data/main/latest/processed/total
    export THRESHOLDFILE=../stat/threshold.txt
    export ACCOUNTFILE=$VGMAINDIR/dpfiles/account.filter
    export LEVELMAPFILE=$ddir/LEVELS-maps.dds
    export INVNODEATTRFILE=$ddir/inv-nodes.dds
    export INVMAPFILE=$ddir/inv-maps-uniq.dds
    if [[ -f $LEVELMAPFILE && -f $INVMAPFILE ]] then
        ddsfilter -osm none -fso vg_inv_threshold.filter.so \
            $ddir/inv-nodes-uniq.dds \
        | sort -u -t'|' -k1,5 > scope/state/threshold.txt.tmp
        if [[ -f scope/state/threshold.txt ]] then
            comm -3 scope/state/threshold.txt scope/state/threshold.txt.tmp \
            | while read -r -u3 cust j2 lv id j3 j4 j5; do
                print -r -- "${cust##+([$' \t'])}"
            done 3<&0- > $tmpdir/threscusts.txt
            [[ ! -s $tmpdir/threscusts.txt ]] && rm $tmpdir/threscusts.txt
        fi
        mv scope/state/threshold.txt.tmp scope/state/threshold.txt
    else
        print -u2 MESSAGE waiting for inventory files
    fi

    print -u2 MESSAGE end - generating threshold file
fi

for ifile in scope/$sn-*-inv.txt; do
    [[ ! -f $ifile ]] && continue
    ibase=${ifile##*/}
    cid=${ibase%-inv.txt}; cid=${cid#"$sn"-}
    if [[ ${cmap[$cid]} == '' ]] then
        print -u2 ERROR $cid not a customer id
        continue
    fi
    sifile=scope/state/$sn-$cid-info.txt
    afile=scope/auto-$sn-$cid-inv.txt

    if [[ ! -f $sifile || $ifile -nt $sifile ]] then
        print -u2 MESSAGE cust $cid - a - scope
        alist[$cid]=1
        slist[$cid]=1
        glist[$cid]=1
    else
        egrep '[^|]$' $sifile | while read -r -u3 j1 j2 j3 j4 j5 fosid; do
            if [[ $fosid != '' && ${sid2state[$fosid]} != *down ]] then
                print -u2 MESSAGE cust $cid - b - scope
                alist[$cid]=1
                slist[$cid]=1
                break
            fi
        done 3<&0-
    fi

    if [[ ! -s $sifile && $(fgrep '|scope_ip=' $ifile) != '' ]] then
        print -u2 MESSAGE cust $cid - c - scope
        alist[$cid]=1
        slist[$cid]=1
    fi

    for sdfile in scope/state/$sn-$cid-sched-*.txt; do
        [[ ! -f $sdfile ]] && continue
        sid=${sdfile##*/}
        sid=${sid%.txt}
        sid=${sid#$sn-$cid-sched-}
        if [[ ${sid2state[$sid]} == *down ]] then
            print -u2 MESSAGE cust $cid - d - scope gen
            alist[$cid]=1
            slist[$cid]=1
            glist[$cid]=1
            break
        fi
        if [[ $afile -nt $sdfile ]] then
            print -u2 MESSAGE cust $cid - e - gen
            alist[$cid]=1
            glist[$cid]=1
        fi
    done

    if [[ -s $tmpdir/threscusts.txt ]] then
        if [[ $(egrep "^$cid\$" $tmpdir/threscusts.txt) != '' ]] then
            print -u2 MESSAGE cust $cid - f - gen
            alist[$cid]=1
            glist[$cid]=1
        fi
    fi

    if [[ $validate == y || $rebalance == y ]] then
        print -u2 MESSAGE cust $cid - g - scope
        alist[$cid]=1
        slist[$cid]=1
    fi
done

for cid in "${!alist[@]}"; do
    ifile=scope/$sn-$cid-inv.txt
    sifile=scope/state/$sn-$cid-info.txt

    unset alvaidsst2sid alvaidsst2fosid
    typeset -A alvaidsst2sid alvaidsst2fosid
    if [[ -f $sifile ]] then
        while read -r alv aid ast sst sid fosid; do
            if [[ ${sid2sip[$sid]} != '' ]] then
                alvaidsst2sid[$alv:$aid:$sst]=$sid
            else
                print -u2 WARNING scope $sid for asset $alv:$aid not found
            fi
            if [[ $fosid != '' && ${sid2sip[$fosid]} != '' ]] then
                alvaidsst2fosid[$alv:$aid:$sst]=$fosid
            elif [[ $fosid != '' ]] then
                print -u2 WARNING foscope $fosid for asset $alv:$aid not found
            fi
        done < $sifile
    fi

    unset alvid amap aattr; typeset -A amap; aattr=()
    egrep '^node\|' scope/$sn-$cid-inv.txt \
    | sort -t'|' -k1,1 -k2,2 -k3,3 \
    | while read -r -u3 rt alv aid k v; do
        if [[ $k == '' ]] then
            print -u2 ERROR empty attribute key $alv:$aid:$k
            continue
        fi
        k=${k//[!.a-zA-Z0-9_:-]/_}
        if [[ $alvid != "$alv:$aid" ]] then
            typeset -n ar=aattr.[$alv:$aid]
            if [[ ${ar.fl} != 1 ]] then
                ar.fl=1
                typeset -A ar.ks
            fi
        fi
        alvid="$alv:$aid"
        amap[$alvid]=1
        ar.ks[$k]=$v
        if [[ $usescopeinvkeys == y ]] then
            if [[ $k == scopeinv_* ]] then
                k=${k//scopeinv_size_/si_sz_}
                k=${k//scopeinv_/si_}
                ar.ks[$k]=$v
            elif [[ $k == si_* ]] then
                k=${k//si_sz_/scopeinv_size_}
                k=${k//si_/scopeinv_}
                ar.ks[$k]=$v
            fi
        fi
    done 3<&0-
    for alvid in "${!amap[@]}"; do
        typeset -n ar=aattr.[$alvid]
        aip=${ar.ks[scope_ip]}
        ast=${ar.ks[scope_systype]}
        asl=${ar.ks[scope_servicelevel]}
        if [[ $aip == '' || $ast == '' || $asl == '' ]] then
            print -u2 MESSAGE skipping incomplete asset $alvid
            amap[$alvid]=
        fi
        if [[ ${slast2sst[$asl:$ast]} == '' ]] then
            [[ $ast != _none_ ]] && \
            print -u2 MESSAGE $alvid: no scope class for service/type: $asl:$ast
            [[ $ast == _none_ ]] && \
            print -u2 MESSAGE $alvid: skipping asset with _none_ type: $asl:$ast
            amap[$alvid]=
        fi
    done

    if [[ ${slist[$cid]} != '' ]] then
        print -u2 MESSAGE begin - assigning scopes for customer $cid

        for alvid in "${!amap[@]}"; do
            [[ ${amap[$alvid]} != 1 ]] && continue
            alv=${alvid%%:*}
            aid=${alvid#*:}
            typeset -n ar=aattr.[$alvid]
            aip=${ar.ks[scope_ip]}
            ast=${ar.ks[scope_systype]}
            asl=${ar.ks[scope_servicelevel]}
            ant=${ar.ks[scope_needtags]}
            for sst in ${slast2sst[$asl:$ast]}; do
                if [[ $sst == _none_ ]] then
                    print -u2 MESSAGE skipping asset with none systype $alv:$aid
                    continue
                fi
                if [[ $rebalance == y ]] then
                    changescope $alv $aid "$aip" "$ast" "$asl" "$ant" "$sst"
                    obestsid=${alvaidsst2sid[$alv:$aid:$sst]}
                    if [[ $obestsid != '' ]] then
                        (( sid2cn[$obestsid]-- ))
                        ((
                            sid2cost[$obestsid] -=
                            ${costmap[$ast]:-1} * ${sid2weights[$obestsid]:-1.0}
                        ))
                    fi
                    alvaidsst2sid[$alv:$aid:$sst]=$bestsid
                    alvaidsst2fosid[$alv:$aid:$sst]=$bestfosid
                fi
                if [[ $validate == y ]] then
                    validatescope $alv $aid "$aip" "$ast" "$asl" "$ant" "$sst"
                    alvaidsst2sid[$alv:$aid:$sst]=$bestsid
                    alvaidsst2fosid[$alv:$aid:$sst]=$bestfosid
                fi
                findscope $alv $aid "$aip" "$ast" "$asl" "$ant" "$sst"
                alvaidsst2sid[$alv:$aid:$sst]=$bestsid
                alvaidsst2fosid[$alv:$aid:$sst]=$bestfosid
                if [[ $bestsid != '' ]] then
                    (( sid2cn[$bestsid]++ ))
                    ((
                        sid2cost[$bestsid] +=
                        ${costmap[$ast]:-1} * ${sid2weights[$bestsid]:-1.0}
                    ))
                    print -u2 MESSAGE scope $bestsid/$sst assigned to $alv:$aid
                else
                    genalarm ALARM $alv $aid 1 \
                    "asset $alv:$aid not covered by any scope"
                fi
                print -r "$alv|$aid|$ast|$sst|$bestsid|$bestfosid"
            done
        done > $sifile.tmp
        if ! cmp $sifile.tmp $sifile > /dev/null 2>&1; then
            glist[$cid]=1
        fi
        mv $sifile.tmp $sifile

        print -u2 MESSAGE end - assigning scopes for customer $cid
    fi

    if [[ ${glist[$cid]} != '' ]] then
        print -u2 MESSAGE begin - generating schedules for customer $cid

        for sdfile in scope/state/$sn-$cid-sched-*.txt; do
            [[ ! -f $sdfile ]] && continue
            rm $sdfile
        done
        unset alvid
        [[ -f scope/auto-$sn-$cid-inv.txt ]] && \
        egrep '^node\|' scope/auto-$sn-$cid-inv.txt \
        | sort -t'|' -k1,1 -k2,2 -k3,3 \
        | while read -r -u3 rt alv aid k v j; do
            if [[ $k == '' ]] then
                print -u2 ERROR empty attribute key $alv:$aid:$k
                continue
            fi
            k=${k//[!.a-zA-Z0-9_:-]/_}
            if [[ $alvid != "$alv:$aid" ]] then
                typeset -n ar=aattr.[$alv:$aid]
                if [[ ${ar.fl} != 1 ]] then
                    ar.fl=1
                    typeset -A ar.ks
                fi
            fi
            alvid="$alv:$aid"
            amap[$alvid]=1
            ar.ks[$k]=$v
            if [[ $usescopeinvkeys == y ]] then
                if [[ $k == scopeinv_* ]] then
                    k=${k//scopeinv_size_/si_sz_}
                    k=${k//scopeinv_/si_}
                    ar.ks[$k]=$v
                elif [[ $k == si_* ]] then
                    k=${k//si_sz_/scopeinv_size_}
                    k=${k//si_/scopeinv_}
                    ar.ks[$k]=$v
                fi
            fi
        done 3<&0-

        unset aid2alarm; typeset -A aid2alarm
        unset aid2status; typeset -A aid2status
        look "$cid|" scope/state/threshold.txt \
        | while read -r -u3 j account lv id mn status alarm; do
            aid2status[$lv:$id:$mn]=$status
            aid2alarm[$lv:$id:$mn]=$alarm
        done 3<&0-

        unset sids; typeset -A sids
        for alvid in "${!amap[@]}"; do
            alv=${alvid%%:*}
            aid=${alvid#*:}
            typeset -n ar=aattr.[$alvid]
            ast=${ar.ks[scope_systype]}
            asl=${ar.ks[scope_servicelevel]}
            for sst in ${slast2sst[$asl:$ast]}; do
                sid=${alvaidsst2sid[$alv:$aid:$sst]}
                [[ $sid == '' ]] && continue
                sids[$sid]=1
            done
        done
        for sid in "${!sids[@]}"; do
            for alvid in "${!amap[@]}"; do
                alv=${alvid%%:*}
                aid=${alvid#*:}
                typeset -n ar=aattr.[$alvid]
                aip=${ar.ks[scope_ip]}
                ast=${ar.ks[scope_systype]}
                asl=${ar.ks[scope_servicelevel]}
                for sst in ${slast2sst[$asl:$ast]}; do
                    [[ $sid != ${alvaidsst2sid[$alv:$aid:$sst]} ]] && continue
                    isprime=y
                    [[
                        ${alvaidsst2fosid[$alv:$aid:$sst]} != '' &&
                        ${alvaidsst2fosid[$alv:$aid:$sst]} != \
                        ${alvaidsst2sid[$alv:$aid:$sst]}
                    ]] && isprime=n
                    genschedule $cid $asl $ast $aip $alv $aid $sid $sst $isprime
                done
            done 4> scope/state/$sn-$cid-sched-$sid.txt &
            sdpjobcntl 4
        done
        wait

        print -u2 MESSAGE end - generating schedules for customer $cid
    fi
    touch $sifile
done

# save scope state
if (( ${#slist[@]} > 0 )) then
    for sid in "${!sid2cn[@]}"; do
        sid2cn[$sid]=0
        sid2cost[$sid]=0
    done
    for sifile in scope/state/$sn-*-info.txt; do
        [[ ! -f $sifile ]] && continue
        cat $sifile
    done | while read -r -u3 alv aid ast sst bestsid bestfosid; do
        if [[ ${sid2sip[$bestsid]} != '' ]] then
            (( sid2cn[$bestsid] += 1 ))
            ((
                sid2cost[$bestsid] +=
                ${costmap[$ast]:-1} * ${sid2weights[$bestsid]:-1.0}
            ))
        fi
    done 3<&0-
    for sid in "${!sid2sip[@]}"; do
        spec="$sid|${sid2sst[$sid]}|${sid2ts[$sid]}|${sid2cn[$sid]}"
        spec+="|${sid2cost[$sid]}|${sid2state[$sid]}"
        print -r "$spec"
    done | sort > scope/state/scopedata.txt.tmp \
    && mv scope/state/scopedata.txt.tmp scope/state/scopedata.txt
fi

# create new schedules
if [[ -f scope/state/pushfailed ]] then
    print -u2 MESSAGE forcing schedule generation due to previous upload failure
    glist[__ALL__]=1
fi

if (( ${#glist[@]} > 0 )) then
    for sdfile in scope/state/$sn-*-sched-*.txt; do
        [[ ! -f $sdfile ]] && continue
        sid=${sdfile#*-}; sid=${sid#*-}; sid=${sid#*-}; sid=${sid%.txt}
        cat $sdfile >> $tmpdir/schedule.$sid.txt
    done
    for sid in "${!sid2sip[@]}"; do
        [[ ${sid2state[$sid]} == up ]] && touch $tmpdir/schedule.$sid.txt
    done
    for fsdfile in $tmpdir/schedule.*.txt; do
        [[ ! -f $fsdfile ]] && continue
        sid=${fsdfile##*schedule.}; sid=${sid%.txt}
        if cmp $fsdfile scope/state/${fsdfile##*/} > /dev/null 2>&1; then
            print -u2 MESSAGE schedule for scope $sid unchanged
            continue
        fi
        lsize=$(ls -Z '%(size)d' $fsdfile)
        user=${sid2user[$sid]}
        ip=${sid2sip[$sid]}
        dir=${sid2dir[$sid]}
        cmd="${dir:-$SCOPEDIR}/etc/schedulejob update $sid $lsize"
        [[ $nopush == y ]] && continue
        [[ ${sid2state[$sid]} == *down ]] && continue
        print -u2 MESSAGE copying schedule to scope $sid
        rm -f scope/state/pushfailed
        SECONDS=0
        (( wt = 120 + ((9.0 * lsize) / transrate) * 18 ))
        timedrun $wt ssh \
            -C -o ConnectTimeout=15 -o ServerAliveInterval=15 \
            -o ServerAliveCountMax=2 -o StrictHostKeyChecking=no \
            $user@$ip "$cmd" \
        < $fsdfile 2> $tmpdir/ssh.err | egrep size= | read rsize
        rsize=${rsize#size=}
        if [[ $rsize == $lsize ]] then
            mv $fsdfile scope/state
            print -u2 MESSAGE copied schedule to scope $sid time: $SECONDS
        else
            print -u2 ERROR cannot copy schedule to scope $sid time: $SECONDS
            sed 's/^/SSH LOG: /' $tmpdir/ssh.err
            touch scope/state/pushfailed
        fi
    done
fi

print -u2 MESSAGE end - updating scope data

# save scope state
if (( ${#alist[@]} > 0 )) then
    for file in scope/state/*; do
        [[ ! -f $file ]] && continue
        case $file in
        */*-info.txt)
            cid=${file##*/}
            cid=${cid%-info.txt}
            cid=${cid#"$sn-"}
            if [[ ${cmap[$cid]} == '' || ! -f scope/$sn-$cid-inv.txt ]] then
                print -u2 MESSAGE deleting info file for cust $cid
                rm $file
            fi
            ;;
        */*-sched-*.txt)
            cid=${file##*/}
            cid=${cid%-sched-*.txt}
            cid=${cid#"$sn-"}
            if [[ ${cmap[$cid]} == '' || ! -f scope/$sn-$cid-inv.txt ]] then
                print -u2 MESSAGE deleting sched file for cust $cid
                rm $file
            fi
            ;;
        */schedule.*.txt)
            sid=${file##*/}
            sid=${sid%.txt}
            sid=${sid#schedule.}
            if [[ ${sid2sip[$sid]} == '' ]] then
                print -u2 MESSAGE deleting schedule file for scope $sid
                rm $file
            fi
            ;;
        esac
    done
fi

print -u2 MESSAGE end - $(printf '%T')
