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

errstate=0
function err { # $1 = log line, $2 = web text
    typeset lerr=$1 werr=$2
    [[ $# == 1 ]] && werr=$1

    if [[ $lerr != '' ]] then
        print -u2 "INVEDIT ERROR $lerr"
    fi
    if [[ $werr != '' ]] then
        [[ $errstate == 0 ]] && print "Content-type: text/html\n"
        errstate=1
        print "<b><font color=red>$werr</font></b>"
    fi
}

function getcid { # $1 = ilv, $2 = iid, $3,4,5 = cust id, name, nodename
    typeset ilv=$1 iid=$2
    typeset -n cidr=$3 cnamer=$4 cnodenamer=$5
    typeset t lv id name value
    typeset IFS ifs

    cidr=''
    ifs="$IFS"
    IFS='|'
    SHOWATTRS=1 SHOWRECS=1 vg_dq_invhelper -lv "$custlevel" "$ilv" "$iid" \
    | while read t lv id name value; do
        if [[ $t == D ]] then
            [[ $name == name ]] && cnamer=$value
            [[ $name == nodename ]] && cnodenamer=$value
            continue
        fi
        [[ $t != B || $lv != $custlevel ]] && continue
        if [[ $cidr != '' ]] then
            err "Multiple customer ids found for parameter $ilv:$iid"
            return 1
        fi
        cidr=$id
    done
    IFS="$ifs"
    if [[ $cidr == '' ]] then
        err "No customer id found for parameter $ilv:$iid"
        return 1
    fi
    return 0
}

function exactlyone { # $1 = ilv, $2 = iid, $3,4,5 = asset id, name, nodename
    typeset ilv=$1 iid=$2
    typeset -n rlevelr=$3 ridr=$4 rnamer=$5
    typeset t lv id name value
    typeset IFS ifs

    rlevelr='' ridr=''
    ifs="$IFS"
    IFS='|'
    SHOWATTRS=1 SHOWRECS=1 vg_dq_invhelper \
        -invdir $PWD -lv "$assetlevel" "$ilv" "$iid" \
    | while read t lv id name value; do
        if [[ $t == D ]] then
            [[ $name == name ]] && rnamer=$value
            continue
        fi
        [[ $t != B || $lv != $assetlevel ]] && continue
        if [[ $ridr != '' ]] then
            err "Multiple asset ids found for parameter $ilv:$iid"
            return 1
        fi
        rlevelr=$lv ridr=$id
    done
    IFS="$ifs"
    if [[ $ridr == '' ]] then
        err "No asset id found for parameter $ilv:$iid"
        return 1
    fi
    return 0
}

function showinv { # $1 = action
    if ! $showfunc "$1" < $custid-inv.txt; then
        err "Show op failed $@" "Show operation failed"
        return 1
    fi
}

function editinv { # $1 = version, $2 = action
    typeset label

    if ! $editfunc "$2" label < $custid-inv.txt > $custid-inv.tmp; then
        err "Edit op failed $2" "Edit operation failed"
        return 1
    fi
    mv $custid-inv.tmp $custid-inv.txt
    rm -f $custid.$v
    ln $custid-inv.txt $custid.$v
    {
        cat versions.lst
        print -r -- "$1|$label"
    } > versions.tmp && mv versions.tmp versions.lst
}

function pickinv { # $1 = version
    if [[ ! -f $custid.$1 ]] then
        err "Cannot find inventory file $custid.$1" "Cannot find inventory"
        return 1
    fi
    cp $custid.$1 $custid-inv.tmp
    mv $custid-inv.tmp $custid-inv.txt
}

function buildinv {
    touch *
    if ! $SHELL vg_inv one $custid-inv.txt \
        $custid-inv-nodes.dds $custid-inv-edges.dds $custid-inv-maps.dds \
    2> /dev/null || \
    ! $SHELL vg_inv all LEVELS-nodes.dds LEVELS-maps.dds \
        inv-nodes inv-edges inv-maps inv-nodes.dds inv-nodes-uniq.dds \
        inv-edges.dds inv-edges-uniq.dds inv-maps.dds inv-maps-uniq.dds \
        inv-cc-nd2cc.dds inv-cc-cc2nd.dds 2> /dev/null
    then
        err "Cannot build inventory for $custid" "Inventory build failed"
        return 1
    fi
}

function queueinv { # $1 = file class $2 = file name $3 = data file
    typeset file=$1 rfile=$2 ofile=$3
    typeset ts pfile

    if ! $queuefunc "queue" $custid-orig $custid-inv.txt; then
        err "Queue op failed $2" "Queue operation failed"
        return 1
    fi
    rfile=${rfile//./_X_X_}
    ts=$(printf '%(%Y%m%d-%H%M%S)T')
    pfile=cm.$ts.$file.$rfile.$VG_SYSNAME.$VG_SYSNAME.$RANDOM.full.xml
    pfile=$VG_DSYSTEMDIR/incoming/cm/$pfile
    (
        print "<a>full</a>"
        print "<u>$sn</u>"
        print "<f>"
        cat $ofile
        print "</f>"
    ) > $pfile.tmp && mv $pfile.tmp $pfile
}

function gotoquery {
    typeset u

    u="/cgi-bin-vg-members/vg_dserverhelper.cgi"
    u+="?pid=$pid&update=-1"
    u+="&query=dataquery&qid=$qid&level_$custlevel=$custid"
    u+="&invedit_dir=data.$(printf '%#H' "$custlevel.$custid")"
    u+="&action__invdir=data.$(printf '%#H' "$custlevel.$custid")"
    u+="&action__filter=clouduseruiactionfilter"
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head></head>"
    print "<body>"
    print "<script>"
    print "location.href = '$u'"
    print "</script>"
    print "</body>"
    print "</html>"
}

function gotourl {
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head></head>"
    print "<body>"
    print "<script>"
    print "location.href = '$1'"
    print "</script>"
    print "</body>"
    print "</html>"
}

. vg_hdr
. vg_prefshelper
. $SWIFTDATADIR/dpfiles/config.sh

suireadcgikv

inveditconfig=()

if ! swmuseringroups "vg_att_admin|vg_clouduserui"; then
    print -u2 "SWIFT ERROR action $qs_action security violation, user: $SWMID"
    exit 1
fi

pid=${qs_pid:-default}
if [[ $pid == '' ]] then
    print -u2 SWIFT ERROR configuration not specified
    exit 1
fi

if [[ $qs_tool == '' ]] then
    err 'No edit tool specified'
    exit 0
fi
if [[ $qs_tool == *[*?/]* ]] then
    err 'Illegal edit tool specified'
    exit 0
fi
set -f
if [[ ! -x $SWMROOT/proj/$instance/bin/${qs_tool}config ]] then
    err 'Cannot load edit tool data'
    exit 0
fi
. $SWMROOT/proj/$instance/bin/${qs_tool}config
set +f
if [[ $? != 0 ]] then
    err 'Error loading edit tool data'
    exit 0
fi
typeset -n cr=inveditconfig
custlevel=${cr.custlevel}
if [[ $custlevel != +([a-zA-Z0-9]) ]] then
    err 'Bad customer level: $custlevel'
    exit 0
fi
bizlevel=${cr.bizlevel}
if [[ $bizlevel != +([a-zA-Z0-9]) ]] then
    err 'Bad business level: $bizlevel'
    exit 0
fi
bizid=${cr.bizid}
if [[ $bizid != +([a-zA-Z0-9]) ]] then
    err 'Bad business id: $bizid'
    exit 0
fi
assetlevel=${cr.assetlevel}
if [[ $assetlevel != +([a-zA-Z0-9]) ]] then
    err 'Bad asset level: $assetlevel'
    exit 0
fi
showfunc=${cr.showfunc}
if [[ $showfunc == '' || $(typeset +f "$showfunc") == '' ]] then
    err 'No show function'
    exit 0
fi
editfunc=${cr.editfunc}
if [[ $editfunc == '' || $(typeset +f "$editfunc") == '' ]] then
    err 'No edit function'
    exit 0
fi
queuefunc=${cr.queuefunc}
if [[ $queuefunc == '' || $(typeset +f "$queuefunc") == '' ]] then
    err 'No queue function'
    exit 0
fi
qid=${cr.qid}
if [[ $qid != +([a-zA-Z0-9.]) ]] then
    err 'Bad query id: $qid'
    exit 0
fi

custid= custname= custnodename=

rlevel= rid= rname=

case $qs_action in
setup)
    if ! getcid "$qs_clevel" "$qs_cid" custid custname custnodename; then
        exit 0
    fi
    cdir=$SWIFTDATADIR/cache/invedit/$SWMID/data.$custlevel.$custid
    mkdir -p $cdir
    if ! cd $cdir; then
        err "Cannot use $cdir"
        exit 0
    fi

    if [[ -f ${custid}-inv.txt ]] then
        print "Content-type: text/html\n"
        gotoquery
        exit 0
    fi

    print -r "$HTTP_REFERER" > referer.url
    {
        print "custlevel=$custlevel"
        print "custid=$custid"
        print "custname=$custname"
        print "custnodename=$custnodename"
    } > edit.info
    > versions.lst
    for i in LEVELS-nodes.dds LEVELS-maps.dds; do
        if ! cp -p $SWIFTDATADIR/data/main/latest/processed/total/$i .; then
            err "Copy of $i failed" "Copy of LEVEL files failed"
            exit 0
        fi
    done
    if [[ ! -f $SWIFTDATADIR/dpfiles/inv/view/$custid-inv.txt ]] then
        touch $custid-inv.txt
    elif ! cp $SWIFTDATADIR/dpfiles/inv/view/$custid-inv.txt $cdir; then
        err "Copy of $custid-inv.txt failed" "Copy of customer file failed"
        exit 0
    fi
    rm -f $custid-orig
    cp $custid-inv.txt $custid-orig
    v=$(printf '%(%Y.%m.%d.%H.%M.%S.%N)T')
    if ! editinv $v cat; then
        err "Initial copy failed" "Cannot create official version"
        exit 0
    fi
    if ! buildinv; then
        exit 0
    fi
    print "Content-type: text/html\n"
    gotoquery
    ;;
setversion)
    cdir=$SWIFTDATADIR/cache/invedit/$SWMID/${qs_action__invdir//[/]/}
    if ! cd $cdir; then
        err "Cannot use $cdir"
        exit 0
    fi
    while read line; do
        typeset -n eikr=${line%%=*}
        eikr=${line#*=}
    done < edit.info
    if ! pickinv $qs_version; then
        err "Cannot find version $qs_version" "Error switching versions"
        exit 0
    fi
    if ! buildinv; then
        exit 0
    fi
    print "Content-type: text/html\n"
    gotoquery
    ;;
save|quit)
    cdir=$SWIFTDATADIR/cache/invedit/$SWMID/${qs_action__invdir//[/]/}
    if ! cd $cdir; then
        err "Cannot use $cdir"
        exit 0
    fi
    while read line; do
        typeset -n eikr=${line%%=*}
        eikr=${line#*=}
    done < edit.info
    if [[ $qs_action == save ]] then
        queueinv view ${custid}-inv.txt ${custid}-inv.txt
    fi
    referer=$(< referer.url)
    rm -rf $cdir.old
    mv $cdir $cdir.old
    print "Content-type: text/html\n"
    gotourl "$referer"
    ;;
@(insert|remove|modify|link|unlink)-*)
    cdir=$SWIFTDATADIR/cache/invedit/$SWMID/${qs_action__invdir//[/]/}
    if ! cd $cdir; then
        err "Cannot use $cdir"
        exit 0
    fi
    while read line; do
        typeset -n eikr=${line%%=*}
        eikr=${line#*=}
    done < edit.info
    if [[ $qs_rid != '' ]] then
        if ! exactlyone "$qs_rlevel" "$qs_rid" rlevel rid rname; then
            exit 0
        fi
    fi
    case $qs_step in
    show)
        print "Content-type: text/html\n"
        phs_init "$pid" ""
        phs_emit_header_begin "Account and User Management Actions"
        phs_emit_header_end
        phs_emit_body_begin
        print "<div class=page><b>User Management Tool</b></div>"
        if ! showinv "$qs_action"; then
           err "Error preparing for $qs_action $qs_rlevel:$qs_rid"
           exit 0
        fi
        print "</div>"
        phs_emit_body_end
        ;;
    do)
        v=$(printf '%(%Y.%m.%d.%H.%M.%S.%N)T')
        if ! editinv $v "$qs_action"; then
            err "Initial copy failed" "Cannot create official version"
            exit 0
        fi
        if ! buildinv; then
            exit 0
        fi
    print "Content-type: text/html\n"
    gotoquery
        ;;
    esac
    ;;
esac
