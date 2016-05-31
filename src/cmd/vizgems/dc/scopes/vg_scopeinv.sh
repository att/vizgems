
usage=$'
[-1p1?
@(#)$Id: vg_scopeinv (AT&T Labs Research) 2006-09-15 $
]
'$USAGE_LICENSE$'
[+NAME?vg_scopeinv - collect inventory information from target system]
[+DESCRIPTION?\bvg_scopeinv\b collects inventory information using either
SSH (UNIX) or WMI (MS Windows).
]
[999:v?increases the verbosity level. May be specified multiple times.]
[+SEE ALSO?\bVizGEMS\b(1)]
'

while getopts -a vg_scopeinv "$usage" opt; do
    case $opt in
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) OPTIND=0 getopts -a vg_scopeinv "$usage" opt '-?'; exit 1 ;;
    esac
done
shift $OPTIND-1

set -o pipefail

. vg_units

while read -r line; do
    cid=${line%%'|'*}
    rest=${line#"$cid"'|'}
    aid=${rest%%'|'*}
    rest=${rest#"$aid"'|'}
    ip=${rest%%'|'*}
    rest=${rest#"$ip"'|'}
    user=${rest%%'|'*}
    rest=${rest#"$user"'|'}
    pass=${rest%%'|'*}
    rest=${rest#"$pass"'|'}
    cs=${rest%%'|'*}
    rest=${rest#"$cs"'|'}
    targettype=${rest%%'|'*}
    rest=${rest#"$targettype"'|'}
    scopetype=${rest%%'|'*}
    rest=${rest##"$scopetype"?('|')}
    servicelevel=${rest%%'|'*}
    rest=${rest##"$servicelevel"?('|')}

    [[ $scopetype == win32.* && $targettype != win32.* ]] && continue
    [[ $scopetype == linux.* && $targettype == win32.* ]] && continue

    if [[ $pass == SWENC*SWENC*SWENC || $cs == SWENC*SWENC*SWENC ]] then
        builtin -f swiftsh swiftshenc swiftshdec
        builtin -f codex codex
        if [[ $pass == SWENC*SWENC*SWENC ]] then
            phrase=${pass#SWENC*SWENC}; phrase=${phrase%SWENC}
            swiftshdec phrase code
            pass=${pass#SWENC}; pass=${pass%%SWENC*}
            print -r "$pass" \
            | codex --passphrase="$code" "<uu-base64-string<aes" \
            | read -r pass
        fi
        if [[ $cs == SWENC*SWENC*SWENC ]] then
            phrase=${cs#SWENC*SWENC}; phrase=${phrase%SWENC}
            swiftshdec phrase code
            cs=${cs#SWENC}; cs=${cs%%SWENC*}
            print -r "$cs" \
            | codex --passphrase="$code" "<uu-base64-string<aes" \
            | read -r cs
        fi
    fi

    export CID=$cid AID=$aid IP=$ip USER=$user PASS=$pass CS=$cs
    export TARGETTYPE=$targettype SCOPETYPE=$scopetype
    export SERVICELEVEL=$servicelevel

    print -u2 MESSAGE begin inventory for $aid
    done=n
    tt=${targettype//[!a-zA-Z0-9]/_}
    while [[ $tt != '' ]] do
        if [[ -x $VG_SSCOPESDIR/current/scopeinv/vg_scopeinv_$tt ]] then
            print -u2 MESSAGE inventory for $aid - tool $tt
            $SHELL $VG_SSCOPESDIR/current/scopeinv/vg_scopeinv_$tt $rest
            done=y
            break
        fi
        if [[ $tt == *_* ]] then
            tt=${tt%_*}
        else
            tt=
            [[ $done == n ]] && print "node|o|$aid|si_dummy$RANDOM|"
        fi
    done | sed "s!\$!|aid=$aid|cid=$cid!"
    print -u2 MESSAGE end inventory for $aid
done
