#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $KSHREC != 1 && ${KSH_VERSION##*' '} < $SHELLVERSION ]] && \
KSHREC=1 exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 SWIFT ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

typeset errs=''
typeset sep=''

function checkid {
    if [[ $1 == '' ]] then
        errs+="${sep}missing user id"
        sep=', '
    elif (( ${#1} < 3 || ${#1} > 12 )) then
        errs+="${sep}the user id must have 3-12 characters"
        sep=', '
    fi
    if [[ $1 != +([a-zA-Z0-9_-]) ]] then
        errs+="${sep}the user id may only contain characters: a-z,A-Z,0-9,_,-"
        sep=', '
    fi
}

function checkpass { # $1=pass1 $2=pass2
    if [[ $1 != $2 ]] then
        errs+="${sep}the two passwords do not match"
        sep=', '
    fi
    if (( ${#1} < 4 || ${#1} > 20 )) then
        errs+="${sep}user passwords must have between 4 - 20 characters"
        sep=', '
    fi
}

function checkgroups { # $1=mgroups $2=fgroups $3=ogroups
    typeset mgs=$1 fgs=$2 ogs=$3
    typeset g

    for g in $mgs; do
        if [[ " $mgroups " != *' '$g' '* ]] then
            errs+="${sep}unknown group $g"
            sep=', '
        fi
    done
    for g in $fgs; do
        if [[ " $fgroups " != *' '$g' '* ]] then
            errs+="${sep}unknown group $g"
            sep=', '
        fi
    done
    for g in $ogs; do
        if [[ " $mgroups " == *' '$g' '* ]] then
            errs+="${sep}cannot specific main group $g"
            sep=', '
        fi
        if [[ " $fgroups " == *' '$g' '* ]] then
            errs+="${sep}cannot specific feature group $g"
            sep=', '
        fi
        if [[ $g != +([a-z0-9_]) ]] then
            errs+="${sep}group $g contains characters other than a-z 0-9 and _"
            sep=', '
        fi
    done
    if [[ $mgs != *' 'vg_att* && $mgs != *' 'vg_cus* ]] then
        errs+="${sep}must assign a user to exactly one att or cust group"
        sep=', '
    fi
    if [[ $mgs == *' 'vg_att* && $mgs == *' 'vg_cus* ]] then
        errs+="${sep}cannot assign a user to both att and cust groups"
        sep=', '
    fi
    if [[ $mgs == *' 'vg_att_admin' '* && $mgs == *' 'vg_att_user' '* ]] then
        errs+="${sep}cannot assign a user to both att_admin and att_user groups"
        sep=', '
    fi
    if [[ $mgs == *' 'vg_cus_admin' '* && $mgs == *' 'vg_cus_user' '* ]] then
        errs+="${sep}cannot assign a user to both cus_admin and cus_user groups"
        sep=', '
    fi

    if [[ $mgs == *' 'vg_cus_* ]] then
        if [[ $fgs == *' 'vg_*opsview* ]] then
            errs+="${sep}cannot assign a cust user to any ops group"
            sep=', '
        fi
        if [[ $fgs == *' 'vg_file* ]] then
            errs+="${sep}cannot assign a cust user to any file group"
            sep=', '
        fi
        if [[ $fgs == *' 'vg_webusage* ]] then
            errs+="${sep}cannot assign a cust user to the web usage group"
            sep=', '
        fi
    fi
}

function checkassets { # $1=mgroups
    typeset mgs=$1
    typeset lvi lv pn

    for lvi in "${!lvs[@]}"; do
        lv=${lvs[$lvi]%%'|'*}; pn=${lvs[$lvi]#*'|'}
        typeset -n lvp=qs_level_$lv
        if [[ ${lvp[@]} == '' ]] then
            errs+="${sep}no asset list for $pn"
            sep=', '
        fi
        if [[ ${lvp[@]} == *__ALL__* && $mgs == *' 'vg_cus* ]] then
            errs+="${sep}customer users cannot see all $pn"
            sep=', '
        fi
    done
}

. vg_hdr

suireadcgikv

mgroups="vg_att_admin vg_att_user vg_cus_admin vg_cus_user"

fgroups="vg_hevonems vg_hevonweb vg_hevopsview vg_hevalarmclear"
fgroups+=" vg_restdataquery vg_restdataupload"
fgroups+=" vg_restconfquery vg_restconfupdate"
fgroups+=" vg_confeditfilter vg_confeditemail vg_confeditthreshold"
fgroups+=" vg_confeditprofile vg_confpoweruser vg_confview"
fgroups+=" vg_fileedit vg_fileview"
fgroups+=" vg_createticket vg_viewticket vg_getassetinfo"
fgroups+=" vg_passchange vg_docview vg_webusage"
ogroups=""

egrep -v '^#' $SWIFTDATADIR/uifiles/dataqueries/levelorder.txt \
| while read -r line; do
    lv=${line%%'|'*}; rest=${line##"$lv"?('|')}
    sn=${rest%%'|'*}; rest=${rest##"$sn"?('|')}
    nl=${rest%%'|'*}; rest=${rest##"$nl"?('|')}
    fn=${rest%%'|'*}; rest=${rest##"$fn"?('|')}
    pn=${rest%%'|'*}; rest=${rest##"$pn"?('|')}
    af=${rest%%'|'*}; rest=${rest##"$af"?('|')}
    attf=${rest%%'|'*}; rest=${rest##"$attf"?('|')}
    cusf=${rest%%'|'*}; rest=${rest##"$cusf"?('|')}
    [[ $line != ?*'|'?*'|'?*'|'?*'|'?* ]] && continue
    [[ $af != yes ]] && continue
    lvs[${#lvs[@]}]="$lv|$pn"
done

print "Content-type: text/xml"
print "Expires: Mon, 19 Oct 1998 00:00:00 GMT\n"

if ! swmuseringroups vg_att_admin; then
    if [[ $qs_mode != passchange || $qs_id != $SWMID ]] then
        print -u2 "SWIFT ERROR accounthelper security violation user: $SWMID"
        exit
    fi
fi

case $qs_mode in
listuser)
    if [[ $qs_uid != '' ]] then
        print "<response>"
        $SHELL vg_accountinfo $qs_uid | while read -r line; do
            k=${line%%=*}; v=${line#*'='}
            if [[ $k == group ]] then
                if [[ " $mgroups " == *' '$v' '* ]] then
                    k=mgroup
                elif [[ " $fgroups " == *' '$v' '* ]] then
                    k=fgroup
                else
                    k=ogroup
                fi
            fi
            if [[ $k == vg_level_* ]] then
                k=${k#vg_}
            fi
            print -r "<r>$(printf '%#H' "$k")|$(printf '%#H' "$v")</r>"
        done
        conffiles=
        $SWIFTDATADIR/etc/confmgr -printbyuser | while read -r line; do
            [[ $line != "$qs_uid|"* ]] && continue
            [[ $line == "$qs_uid|"@(favorites|preferences)'|'* ]] && continue
            file=${line#*'|'}
            file=${file%%'|'*}
            conffiles+=" $file"
        done
        if [[ $conffiles != '' ]] then
            print -r "<r>hasconf|${conffiles#' '}</r>"
        fi
        print "</response>"
    fi
    ;;
create)
    checkid "$qs_id"
    checkpass "$qs_pass1" "$qs_pass2"
    checkgroups " ${qs_mgroup[*]} " " ${qs_fgroup[*]} " " ${qs_ogroup[*]} "
    checkassets " ${qs_mgroup[*]} "

    if [[ $errs != '' ]] then
        print "<response>"
        print "<r>ERROR|$(printf '%#H' "$errs")</r>"
        print "</response>"
        return
    fi

    print "<response>"
    allgroups="$(print -r ${qs_mgroup[@]} ${qs_fgroup[@]} ${qs_ogroup[@]} vg all)"
    allgroups=${allgroups//' '/:}
    info="vg_groups=$allgroups"
    info+="|vg_name=$qs_name|vg_comments=$qs_info"
    for lvi in "${!lvs[@]}"; do
        lv=${lvs[$lvi]%%'|'*}; pn=${lvs[$lvi]#*'|'}
        typeset -n lvp=qs_level_$lv
        lvs=$(print "${lvp[@]}")
        lvs=${lvs//' '/:}
        info+="|vg_level_$lv=$lvs"
    done
    info+="|vg_cmid=$qs_cmid"
    $SHELL vg_accountupdate \
        add \
    "$qs_id" "$qs_pass1" "$allgroups" "$qs_name" "$info"
    case $? in
    10)
        print "<r>ERROR|user $qs_id exists</r>"
        ;;
    0)
        print "<r>OK|OK</r>"
        ;;
    *)
        print "<r>ERROR|operation failed</r>"
        ;;
    esac
    print "</response>"
    ;;
modify)
    checkid "$qs_id"
    [[ $qs_pass1 != '' ]] && checkpass "$qs_pass1" "$qs_pass2"
    checkgroups " ${qs_mgroup[*]} " " ${qs_fgroup[*]} " " ${qs_ogroup[*]} "
    checkassets " ${qs_mgroup[*]} "

    if [[ $errs != '' ]] then
        print "<response>"
        print "<r>ERROR|$(printf '%#H' "$errs")</r>"
        print "</response>"
        return
    fi

    print "<response>"
    allgroups="$(print -r ${qs_mgroup[@]} ${qs_fgroup[@]} ${qs_ogroup[@]} vg all)"
    allgroups=${allgroups//' '/:}
    info="vg_groups=$allgroups"
    info+="|vg_name=$qs_name|vg_comments=$qs_info"
    for lvi in "${!lvs[@]}"; do
        lv=${lvs[$lvi]%%'|'*}; pn=${lvs[$lvi]#*'|'}
        typeset -n lvp=qs_level_$lv
        lvs=$(print "${lvp[@]}")
        lvs=${lvs//' '/:}
        info+="|vg_level_$lv=$lvs"
    done
    info+="|vg_cmid=$qs_cmid"
    $SHELL vg_accountupdate \
        mod \
    "$qs_id" "$qs_pass1" "$allgroups" "$qs_name" "$info"
    case $? in
    10)
        print "<r>ERROR|user $qs_id does not exist</r>"
        ;;
    0)
        print "<r>OK|OK</r>"
        ;;
    *)
        print "<r>ERROR|operation failed</r>"
        ;;
    esac
    print "</response>"
    ;;
passchange)
    checkpass "$qs_pass1" "$qs_pass2"

    if [[ $errs != '' ]] then
        print "<response>"
        print "<r>ERROR|$(printf '%#H' "$errs")</r>"
        print "</response>"
        return
    fi

    print "<response>"
    $SHELL vg_accountupdate mod "$SWMID" "$qs_pass1" "" "" "" ""
    case $? in
    10)
        print "<r>ERROR|user $qs_id does not exist</r>"
        ;;
    0)
        print "<r>OK|OK</r>"
        ;;
    *)
        print "<r>ERROR|operation failed</r>"
        ;;
    esac
    print "</response>"
    ;;
delete)
    checkid "$qs_id"

    if [[ $errs != '' ]] then
        print "<response>"
        print "<r>ERROR|$(printf '%#H' "$errs")</r>"
        print "</response>"
        return
    fi

    print "<response>"
    $SHELL vg_accountupdate del "$qs_id"
    case $? in
    10)
        print "<r>ERROR|user $qs_id does not exist</r>"
        ;;
    0)
        print "<r>OK|OK</r>"
        ;;
    *)
        print "<r>ERROR|operation failed</r>"
        ;;
    esac
    print "</response>"
    ;;
esac
