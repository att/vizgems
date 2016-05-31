
ts=$(printf '%(%Y%m%d-%H%M%S)T')
sn=$VG_SYSNAME
pfile=cm.$ts.account..$sn.$sn.$RANDOM.edit.xml
pfile=$VG_DSYSTEMDIR/incoming/cm/$pfile
(
    print "<a>special</a>"
    print "<u>${SWMID:-$sn}</u>"
    print "<f>"
    print -r "$1;$2;$3;$4;$5;$6"
    print "</f>"
) > $pfile.tmp && mv $pfile.tmp $pfile

if [[ $1 == del ]] then
    rfile=$2.sh
    pfile=cm.$ts.preferences.${rfile//./_X_X_}.$sn.$sn.$RANDOM.remove.xml
    pfile=$VG_DSYSTEMDIR/incoming/cm/$pfile
    (
        print "<a>remove</a>"
        print "<u>$sn</u>"
        print "<f>"
        print "</f>"
    ) > $pfile.tmp && mv $pfile.tmp $pfile
fi
