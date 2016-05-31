
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

function p2p {
    print $(( $1 / 72.0 ))
}

function fssmaller {
    if (( ${1%pt} > 6 )) then
        print -- $(( ${1%pt} - 2 ))pt
    else
        print -- $1
    fi
}

function fs2gd {
    print ${1%pt}
}

bannermenu="link|home||Home|return to the home page|link|back||Back|go back one page|list|favorites||Favorites|run a favorite query|list|tools|x:m:u:p:c|Tools|tools|list|help|userguide:dqpage|Help|documentation"

dqmode=${DQMODE:-default}

# security checks

if [[ $REMOTEQUERY == y ]] then
    group='vg_hevon*'
else
    case $VG_SYSMODE in
    ems) group=vg_hevonems ;;
    viz) group=vg_hevonweb ;;
    esac
fi
if ! swmuseringroups "vg_att_admin|$group"; then
    print -u2 "SWIFT ERROR vg_dq_run security violation user: $SWMID"
    print "<html><body><b><font color=red>"
    print "sorry, you are not authorized to access this page"
    print "</font></b></body></html>"
    exit
fi

rm -f *.attr *.dds *.filter *.cmap *.gif *.embed* *.*list *.html *.css
rm -f *.toc *.*info secret.txt this.txt

# load system settings

. vg_hdr
. vg_prefshelper
. $SWIFTDATADIR/dpfiles/config.sh
export ALWAYSCOMPRESSUI=${ALWAYSCOMPRESSUI:-n}
if [[ $ALWAYSCOMPRESSUI == y ]] then
    export UIZFLAG='-ozm rtb'
fi
export STRICTEDGEUI=${STRICTEDGEUI:-n}

# load query parameters

suireadkv 0 vars

if [[ ${vars.debug} != '' ]] then
    if swmuseringroups "vg_att_*"; then
        export DEBUG=y
        [[ ${vars.debug} == 1 ]] && export SWIFTWARNLEVEL=0
        exec 2> debug.log
        print -u2 "debug log"
        ulimit -Sc unlimited
    fi
fi

if [[ $HTTP_COOKIE == *attVGvars* ]] then
    varlist=${HTTP_COOKIE##*attVGvars=}
    varlist=${varlist%%\;*}
    if [[ $varlist != '' ]] then
        varlist=${varlist//'+'/' '}
        varlist=${varlist//@([\'\\])/'\'\1}
        eval "varlist=\$'${varlist//'%'@(??)/'\x'\1"'\$'"}'"
    fi
    while [[ $varlist != '' ]] do
        kv=${varlist%%';'*}
        varlist=${varlist##"$kv"?(";")}
        [[ $kv != ?*=* ]] && continue
        k=${kv%%"="*}
        v=${kv#*"="}
        k=${k//'.'/_}
        [[ $k != +([a-zA-Z0-9_]) ]] && continue
        typeset -n varr=vars.$k
        varr=$v
        typeset +n varr
    done
fi

qid=${vars.qid}
vars.name=${vars.name##*/}

[[ ${vars.winw} == '' ]] && vars.winw=1000
[[ ${vars.winh} == '' ]] && vars.winh=600
export SCREENWIDTH=${vars.winw}

# load query preferences

pid=${vars.pid}
if [[ $pid == '' ]] then
    print -u2 SWIFT ERROR configuration not specified
    exit 1
fi
vg=()
ph_init $pid "$bannermenu"
typeset -n prefpp=vg.style.page
typeset -n prefbp=vg.style.banner
typeset -n prefsp=vg.style.sdiv
typeset -n prefmp=vg.style.mdiv
typeset -n prefqp=vg.style.query

. vg_dq_main

# prepare query state

secret=$SWMID.$RANDOM.$SECONDS.$RANDOM.$$
print $secret > secret.txt
export IMAGEPREFIX=$SWIFTWEBPREFIX/cgi-bin-vg-members/vg_dq_imghelper.cgi
IMAGEPREFIX+="?pid=$pid&secret=$secret&name=${vars.name}/"

if [[ -f $SWMROOT/logs/user_log ]] then
    printf 'T=%(%Y/%m/%d-%H:%M:%S)T A=%q U=%q S=%q Q=%q\n' \
        "" ${REMOTE_ADDR:-local} "$SWMID" "$DQSRC" "${vars}" \
    >> $SWMROOT/logs/user_log
fi

# set up query

if [[ -f $SWIFTDATADIR/uifiles/dataqueries/setup.sh ]] then
    . $SWIFTDATADIR/uifiles/dataqueries/setup.sh
fi

querydone=n
found=n
egrep -v '^#|^$' $SWIFTDATADIR/uifiles/dataqueries/queries.txt \
| while read -r line; do
    ls=${line%%'|'*}; rest=${line##"$ls"?('|')}
    us=${rest%%'|'*}; rest=${rest##"$us"?('|')}
    gs=${rest%%'|'*}; rest=${rest##"$gs"?('|')}
    qs=${rest%%'|'*}; rest=${rest##"$qs"?('|')}
    ns=${rest%%'|'*}; rest=${rest##"$ns"?('|')}
    ds=${rest%%'|'*}; rest=${rest##"$ds"?('|')}
    hs=${rest%%'|'*}; rest=${rest##"$hs"?('|')}
    if [[ $qs == "$qid" ]] then
        if [[ $us != '' && " $SWMID " != @(*\ (${us//:/'|'})\ *) ]] then
            continue
        fi
        if [[ $gs != '' ]] && ! swmuseringroups "${gs//:/'|'}"; then
            continue
        fi
        found=y
        break
    fi
done
if [[ $found != y ]] then
    print -u2 SWIFT ERROR cannot find query $qid for user/group in queries.txt
    exit 1
fi
if [[ ! -f $SWIFTDATADIR/uifiles/dataqueries/$qid.sh ]] then
    print -u2 SWIFT ERROR cannot find query $qid script
    exit 1
fi
. $SWIFTDATADIR/uifiles/dataqueries/$qid.sh
if [[ $querydone == y ]] then
    exit 0
fi
dq_main_init

# run query

dq_main_run

# emit results

dq_main_emit_header_begin
dq_main_emit_header_end
dq_main_emit_body_begin
dq_main_emit_body_end
