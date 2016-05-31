
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

if [[ $CASEINSENSITIVE == y ]] then
    trcmd='tr A-Z a-z'
else
    trcmd=cat
fi

egrep -v '^#|^ *$' $1 | $trcmd \
| ddsload -os vg_trans.schema -le '	' -type ascii -dec simple \
    -lnts -lso vg_trans.load.so \
| ddssort -fast -u -ke inid \
> $2.tmp && sdpmv $2.tmp $2
ddssort -fast -ke outid $2 \
> $3.tmp && sdpmv $3.tmp $3
