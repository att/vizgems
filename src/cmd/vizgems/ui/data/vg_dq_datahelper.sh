
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

dqmode=${default}

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

tool=$1 file=$2 page=$3

. vg_dq_main
. vg_dq_vt_${tool}

dq_vt_${tool}_emitpage $file $page
