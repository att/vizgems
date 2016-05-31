
argv0=${0##*/}
if [[ $(whence -p $argv0) == "" ]] then
    print -u2 ERROR environment not set up
    exit 1
fi

instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
group=${rest%%_*}
rest=${rest#$group}
rest=${rest#_}
if [[ $instance == '' || $group == '' || $rest == '' ]] then
    print -u2 ERROR program name not in correct form
    exit 1
fi

function completetransfer {
    typeset statfile tmpname inname date outname

    tw -i -H -d $tmp1dir -e 'name=="*.failed"' 2> /dev/null \
    | while read statfile; do
        print -u2 ERROR failure file $statfile
        tmpname=${statfile##*/}
        tmpname=${tmpname%.failed}
        inname=${tmpname#INNAME_}
        inname=${inname%_DATE_*}
        date=${tmpname#*_DATE_}
        date=${date%_OUTNAME_*}
        outname=${tmpname##*_OUTNAME_}
        outname=${outname:-$inname}
        [[ ${gvars.filerecord} == y ]] && touch $recdir/$inname
        if [[ ${gvars.fileremove} == y ]] then
            rm $rmflags -f $(< $statfile)
        fi
        rm -f $statfile $tmp1dir/$tmpname
    done
    tw -i -H -d $tmp1dir -e 'name=="*.done"' 2> /dev/null \
    | while read statfile; do
        print -u2 success file $statfile
        tmpname=${statfile##*/}
        tmpname=${tmpname%.done}
        inname=${tmpname#INNAME_}
        inname=${inname%_DATE_*}
        date=${tmpname#*_DATE_}
        date=${date%_OUTNAME_*}
        date=${date//.//}
        outname=${tmpname##*_OUTNAME_}
        outname=${outname:-$inname}
        (
            if mv $tmp1dir/$tmpname $tmp2dir/$tmpname; then
                [[ ${gvars.filerecord} == y ]] && touch $recdir/$inname
                if ! mv $tmp2dir/$tmpname $datadir/$date/input/$outname; then
                    print -u2 ERROR cannot move $outname to $datadir/$date/input
                fi
            else
                print -u2 ERROR cannot move $tmpname to $tmp2dir
            fi
            if [[ ${gvars.fileremove} == y ]] then
                rm $rmflags -f $(< $statfile)
            fi
            rm -f $statfile $tmp1dir/$tmpname
        ) &
        sdpjobcntl ${gvars.filesinparallel}
    done
}

function recovertransfer {
    typeset tmpname inname date outname
    tw -i -H -d $tmp2dir -e 'name=="INNAME_*_DATE_*_OUTNAME_*"' 2> /dev/null \
    | while read file; do
        tmpname=${file##*/}
        inname=${tmpname#INNAME_}
        inname=${inname%_DATE_*}
        date=${tmpname#*_DATE_}
        date=${date%_OUTNAME_*}
        date=${date//.//}
        outname=${tmpname##*_OUTNAME_}
        outname=${outname:-$inname}
        print -u2 MESSAGE recovering $outname to $datadir/$date/input
        [[ ${gvars.filerecord} == y ]] && touch $recdir/$inname
        if sdpensuredualdirs $date $datadir $linkdir; then
            if ! mv $tmp2dir/$tmpname $datadir/$date/input/$outname; then
                print -u2 ERROR cannot move $outname to $datadir/$date/input
            fi
        fi
        rm -f $file
    done
}

function cleanexit {
    exec >&-
    exec 1>&4

    if [[ ! -s $logfile ]] then
        rm -f $logfile
    fi
    exit
}

function showusage {
    print -u2 "$argv [optional flags]"
    print -u2 "    [-mindate <date>]  (default: ${gvars.mindate})"
    print -u2 "    [-maxdate <date>]  (default: ${gvars.maxdate})"
}

autoload sdpensuredualdirs sdpensuredirs
autoload sdpruncheck sdpspacecheck
autoload ${instance}_init ${instance}_${group}_inc_init
autoload ${instance}_${group}_inc_fileinfo

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft completetransfer recovertransfer
    typeset -ft sdpensuredualdirs sdpensuredirs
    typeset -ft sdpruncheck sdpspacecheck
    typeset -ft ${instance}_init ${instance}_${group}_inc_init
    typeset -ft ${instance}_${group}_inc_fileinfo
    set -x
fi

gvars=(
    maindir=UNSET
    permdir=UNSET
    feeddir=UNSET
    datadir=

    mindate=UNSET
    maxdate=UNSET

    mainspace=UNSET
    permspace=UNSET

    filetype=UNSET
    filepatincl=UNSET
    filepatexcl=UNSET
    filepatremove=UNSET
    filecheckuse=UNSET
    filecheckhost=
    filesperround=UNSET
    filesinparallel=UNSET
    fileremove=UNSET
    filesync=y
    filecopier=cp
    filecollector=
    filerecord=y
)

typeset -A datemap complmap

${instance}_init gvars $group
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke instance customization function
    exit 1
fi
${instance}_${group}_inc_init gvars
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke group customization function
    exit 1
fi
if [[ -f ${gvars.maindir}/etc/${instance}_${group}_inc_vars ]] then
    . ${gvars.maindir}/etc/${instance}_${group}_inc_vars gvars
    if [[ $? != 0 ]] then
        print -u2 ERROR cannot invoke local customization script
        exit 1
    fi
fi
if [[ ${gvars.datadir} == "" ]] then
    gvars.datadir=${gvars.feeddir}
fi
for i in "${!gvars.@}"; do
    if [[ $(eval print \${$i}) == UNSET ]] then
        print -u2 ERROR variable $i not set
        exit 1
    fi
done

if [[ ${gvars.filetype} == DIR ]] then
    cpflags=-r
    rmflags=-r
else
    cpflags=
    rmflags=
fi

while (( $# > 0 )) do
    if [[ $1 == '-mindate' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        gvars.mindate=$2
        shift; shift
    elif [[ $1 == '-maxdate' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        gvars.maxdate=$2
        shift; shift
    else
        showusage
        exit
    fi
done

if ! sdpensuredirs ${gvars.maindir}; then
    exit 1
fi
if [[ ${gvars.permdir} != '' ]] && ! sdpensuredirs ${gvars.permdir}; then
    exit 1
fi

incdir=${gvars.maindir}/incoming/${gvars.feeddir}
if [[ ${gvars.permdir} != "" ]] then
    datadir=${gvars.maindir}/data/${gvars.datadir}
    linkdir=${gvars.permdir}/data/${gvars.datadir}
else
    datadir=${gvars.maindir}/data/${gvars.datadir}
    linkdir=
fi
recdir=${gvars.maindir}/record/inc.$group
logdir=${gvars.maindir}/logs
tmpdir=${gvars.maindir}/tmp/misc
export TMPDIR=$tmpdir
tmp1dir=${gvars.maindir}/tmp/inc.${group}.1
tmp2dir=${gvars.maindir}/tmp/inc.${group}.2

if ! sdpensuredirs $incdir; then
    exit 1
fi
if ! sdpensuredirs -c $datadir $linkdir $recdir $logdir; then
    exit 1
fi
if ! sdpensuredirs -c $tmpdir $tmp1dir $tmp2dir; then
    exit 1
fi

logfile=$logdir/inc.$group.$(date -f '%Y%m%d-%H%M%S').log
gvars.exitfile=${gvars.maindir}/exit.inc.$group
gvars.lockfile=${gvars.maindir}/lock.inc.$group

set -o noclobber
if ! command exec 3> ${gvars.lockfile}; then
    if kill -0 $(< ${gvars.lockfile}); then
        exit 0
    elif [[ $(fuser ${gvars.lockfile}) != '' ]] then
        exit 0
    else
        rm ${gvars.lockfile}
        exit 0
    fi
fi 2> /dev/null
print -u3 $$
set +o noclobber

trap 'rm -f ${gvars.lockfile}' HUP QUIT TERM KILL EXIT

exec 4>&1
exec >> $logfile
exec 2>&1

recovertransfer
completetransfer
rm -rf $tmp1dir/*
rm -rf $tmp2dir/*

(( gvars.filesinparallel++ ))

sdpruncheck ${gvars.exitfile} || cleanexit
sdpspacecheck $datadir ${gvars.mainspace} || cleanexit

if [[ ${gvars.filecollector} != "" ]] then
    ${gvars.filecollector} gvars || cleanexit
fi

filesthisround=0
tw -i -H -d $incdir -e "sort: name" -e "
    type==${gvars.filetype} && name==\"${gvars.filepatincl}\" &&
    name!=\"${gvars.filepatexcl}\"
" 2> /dev/null | while read inpath; do
    inname=${inpath##*/}
    if [[ ${gvars.filerecord} == y ]] then
        if [[ -f $recdir/$inname ]] then
            if [[ ${gvars.fileremove} == y ]] then
                rm $rmflags -f $inpath
            fi
            continue
        fi
    fi
    if [[ $inname == ${gvars.filepatremove} ]] then
        rm -f $inpath
        continue
    fi
    ovars=(date=ERROR outname=ERROR)
    ${instance}_${group}_inc_fileinfo gvars ovars $inname
    if [[ ${ovars.date} == ERROR || ${ovars.outname} == ERROR ]] then
        print -u2 ERROR unknown file pattern $inname
        continue
    fi
    if [[ ${ovars.outname} == $inname ]] then
        ovars.outname=
    fi
    if [[
        ${ovars.date} < ${gvars.mindate} || ${ovars.date} > ${gvars.maxdate}
    ]] then
        continue
    fi
    if [[ ${complmap[${ovars.date}]} == "" ]] then
        if [[
            -f $datadir/${ovars.date//.//}/complete.stamp ||
            -f $datadir/${ovars.date//.//}/complete.stamp1
        ]] then
            complmap[${ovars.date}]=y
        else
            complmap[${ovars.date}]=n
        fi
    fi
    if [[ ${complmap[${ovars.date}]} == y ]] then
        continue
    fi
    if [[ ${datemap[${ovars.date}]} == "" ]] then
        if ! sdpensuredualdirs ${ovars.date//.//} $datadir $linkdir; then
            continue
        fi
        datemap[${ovars.date}]=y
    fi
    (
        exec 3< ${gvars.lockfile}
        if [[ ${gvars.filecheckuse} == y ]] then
            if [[ ${gvars.filecheckhost} == "" ]] then
                if [[ $(fuser $inpath 2> /dev/null) != '' ]] then
                    exit
                fi
            else
                if [[
                    $(
                        rsh ${gvars.filecheckhost} fuser $inpath 2> /dev/null
                    ) != ''
                ]] then
                    exit
                fi
            fi
        fi
        print -u2 begin inc processing file $inname
        tmpname=INNAME_${inname}_DATE_${ovars.date}_OUTNAME_${ovars.outname}
        if ${gvars.filecopier} $cpflags $inpath $tmp1dir/$tmpname; then
            print $inpath > $tmp1dir/$tmpname.done
        else
            print $inpath > $tmp1dir/$tmpname.failed
        fi
        print -u2 end inc processing file $inname
    ) &
    sdpjobcntl ${gvars.filesinparallel}
    (( filesthisround++ ))
    if (( $filesthisround == ${gvars.filesperround} )) then
        # there is a race condition in this section
        # it's not really a problem
        sdpjobcntl 2
        if [[ ${gvars.filesync} == y ]] then
            sync
        fi
        completetransfer
        sdpjobcntl 2
        sdpruncheck ${gvars.exitfile} || break
        sdpspacecheck $datadir ${gvars.mainspace} || break
        filesthisround=0
        unset complmap
        typeset -A complmap
    fi
done
wait; wait
if (( $filesthisround > 0 )) then
    if [[ ${gvars.filesync} == y ]] then
        sync
    fi
fi
completetransfer
wait; wait

cleanexit
