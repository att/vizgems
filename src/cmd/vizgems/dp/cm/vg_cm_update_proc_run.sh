function vg_cm_update_proc_run {
    typeset -n gv=$1
    typeset datadir=$2
    typeset linkdir=$3
    shift 3
    typeset dates="$@"

    typeset dir maxjobs date year month day rest indir outdir
    typeset sn rsn file ifile rfile xrfile ofile pfile
    typeset ts ext maxd spec ifs op ln extras extrai

    if [[ -f $VGMAINDIR/dpfiles/config.sh ]] then
        . $VGMAINDIR/dpfiles/config.sh
    fi
    export CASEINSENSITIVE=${CASEINSENSITIVE:-n}
    export DEFAULTLEVEL=${DEFAULTLEVEL:-o}
    export DEFAULTTMODE=${DEFAULTTMODE:-keep}
    export MAXALARMKEEPMIN=${MAXALARMKEEPMIN:-120}
    export PROPALARMS=${PROPALARMS:-n}
    export PROPEMAILS=${PROPEMAILS:-n}
    export PROPSTATS=${PROPSTATS:-n}
    export STATINTERVAL=${STATINTERVAL:-300}
    export PROFILESAMPLES=${PROFILESAMPLES:-4}
    export GCEPHASEON=${GCEPHASEON:-n}
    export PCEPHASEON=${PCEPHASEON:-n}
    export NOTIMENORM=${NOTIMENORM:-0}
    export AGGRDELAY=${AGGRDELAY:-'1 hour'}
    export CONCATDELAY=${CONCATDELAY:-'120 min'}
    export ALWAYSCOMPRESSDP=${ALWAYSCOMPRESSDP:-n}
    export STANDBYMODE=${STANDBYMODE:-n}
    export ALARMNOAGGR=${ALARMNOAGGR:-n}
    export STATNOAGGR=${STATNOAGGR:-n}

    . $VGMAINDIR/etc/confmgr.info
    for dir in ${PATH//:/ }; do
        if [[ -f $dir/../lib/vg/vg_cmfilelist ]] then
            configfile=$dir/../lib/vg/vg_cmfilelist
            break
        fi
    done
    . $configfile
    if [[ $? != 0 ]] then
        print -u2 ERROR missing or bad configuration file
        exit 1
    fi

    if [[ ${gvars.cmfileremovedays} != '' ]] then
        maxd="${gvars.cmfileremovedays} day ago"
    fi

    sn=$VG_SYSNAME
    maxjobs=${gv.raw2ddsjobs}
    for date in $dates; do
        sdpruncheck ${gv.exitfile} || break
        sdpspacecheck $datadir ${gv.mainspace} || break
        year=${date%%/*}
        rest=${date#*/}
        month=${rest%%/*}
        rest=${rest#*/}
        day=${rest%%/*}
        if ! sdpensuredualdirs $date $datadir $linkdir; then
            continue
        fi
        indir=$datadir/$date/input
        outdir=$datadir/$date/processed
        (
            exec 3< ${gv.lockfile}
            (
                print begin cm processing $date $(date)
                cd $outdir
                rm -f * 2> /dev/null
                mkdir -p save
                for ifile in ../input/cm.*.xml; do
                    [[ ! -f $ifile ]] && continue
                    spec=${ifile##*/}
                    ifs="$IFS"
                    IFS='.'
                    set -f
                    set -A cs -- $spec
                    set +f
                    IFS="$ifs"
                    file=${cs[2]}
                    rfile=${cs[3]//_X_X_/.}
                    xrfile=${cs[3]}
                    rsn=${cs[4]}
                    op=${cs[7]}
                    if [[
                        $rsn == $sn && $STANDBYMODE == n && $MASTERCONF != y
                    ]] then
                        if [[ $PROPCONFS == y ]] then
                            ts=$(printf '%(%Y%m%d-%H%M%S)T')
                            pfile=$VGMAINDIR/outgoing/cm/${ifile##*/}
                            cp $ifile $pfile.tmp && mv $pfile.tmp $pfile
                        fi
                        rm $ifile
                        continue
                    fi
                    typeset -n fdata=files.$file
                    if [[ ${fdata.locationmode} == dir ]] then
                        ofile=${fdata.location}/$rfile
                    else
                        ofile=${fdata.location}
                    fi
                    ts=$(printf '%(%Y%m%d-%H%M%S)T')
                    ext=$ts.$$
                    pflag=n
                    case $op in
                    *edit)
                        if [[ ${fdata.fileedit} != vg_cmdefedit ]] then
                            CMNOIDCHECK=y \
                            $SHELL ${fdata.fileedit} $configfile $file $ifile \
                            5> ${ifile##*/}.user || continue
                            read user < ${ifile##*/}.user
                            (
                                print "VGMSG|$ts|$user|Begin|$file|$rfile"
                                print "VGMSG|$ts|$user|Edit|"
                                print "VGMSG|$ts|$user|End"
                            ) 1>&2
                        else
                            CMNOIDCHECK=y \
                            $SHELL ${fdata.fileedit} $configfile $file $ifile \
                            3> ${ifile##*/}.del 4> ${ifile##*/}.ins \
                            5> ${ifile##*/}.user || continue
                            (
                                if [[ -s ${ifile##*/}.del ]] then
                                    fgrep -v -x -f ${ifile##*/}.del $ofile
                                elif [[ -f $ofile ]] then
                                    cat $ofile
                                fi
                                cat ${ifile##*/}.ins
                            ) | sort -u > $ofile.tmp || continue
                            [[ -f $ofile ]] && cp $ofile save/${ofile##*/}.$ext
                            if ! cmp $ofile.tmp $ofile > /dev/null 2>&1; then
                                mv $ofile.tmp $ofile
                            else
                                op=noop
                                rm $ofile.tmp
                            fi
                            read user < ${ifile##*/}.user
                            (
                                print "VGMSG|$ts|$user|Begin|$file|$rfile"
                                sed \
                                    "s/^/VGMSG|$ts|$user|Remove|/" \
                                ${ifile##*/}.del
                                sed \
                                    "s/^/VGMSG|$ts|$user|Insert|/" \
                                ${ifile##*/}.ins
                                print "VGMSG|$ts|$user|End"
                            ) 1>&2
                        fi
                        ;;
                    full)
                        # cheat - dont read file as xml
                        while read -r line; do
                            case $line in
                            '<a>'*'</a>')
                                ;;
                            '<u>'*'</u>')
                                user=${line#'<u>'}
                                user=${user%'</u>'}
                                ;;
                            '<f>'|'</f>')
                                ;;
                            *)
                                print -r "$line"
                                ;;
                            esac
                        done < $ifile > $ofile.tmp || continue
                        [[ -f $ofile ]] && cp $ofile save/${ofile##*/}.$ext
                        if ! cmp $ofile.tmp $ofile > /dev/null 2>&1; then
                            mv $ofile.tmp $ofile
                        else
                            rm $ofile.tmp
                        fi
                        (
                            print "VGMSG|$ts|$user|Begin|$file|$rfile"
                            ln=$(wc -l < $ofile)
                            print "VGMSG|$ts|$user|Replace|${ln##+(' ')} lines"
                            print "VGMSG|$ts|$user|End"
                        ) 1>&2
                        ;;
                    remove)
                        # cheat - dont read file as xml
                        while read -r line; do
                            case $line in
                            '<a>'*'</a>')
                                ;;
                            '<u>'*'</u>')
                                user=${line#'<u>'}
                                user=${user%'</u>'}
                                ;;
                            esac
                        done < $ifile
                        [[ -f $ofile ]] && cp $ofile save/${ofile##*/}.$ext
                        rm -f $ofile
                        (
                            print "VGMSG|$ts|$user|Begin|$file|$rfile"
                            print "VGMSG|$ts|$user|Remove"
                            print "VGMSG|$ts|$user|End"
                        ) 1>&2
                        ;;
                    esac
                    [[ $file == view ]] && touch $VGMAINDIR/dpfiles/inv/doview
                    [[ $file == scope ]] && touch $VGMAINDIR/dpfiles/inv/doscope
                    if [[ $file == account ]] then
                        touch $VG_DWWWDIR/etc/dodbm
                        extras=(
                            passwd=(
                                file=passwd rfile='' ofile=$VGCM_PASSWDFILE
                            )
                            group=(
                                file=group rfile='' ofile=$VGCM_GROUPFILE
                            )
                        )
                    fi
                    if [[ $maxd != '' ]] then
                        tw \
                            -i -H -d ./save \
                            -e "type==REG && mtime < \"$maxd\"" \
                        rm
                        maxd= # do this just once
                    fi
                    if [[
                        $PROPCONFS == y && $STANDBYMODE == n &&
                        $MASTERCONF == y
                    ]] then
                        if [[ $op == noop ]] then
                            print "VGMSG|$ts|$user|Same|$file|$rfile"
                            rm $ifile
                            continue
                        fi
                        ts=$(printf '%(%Y%m%d-%H%M%S)T')
                        pfile=cm.$ts.$file.$xrfile.$sn.$user.$RANDOM.full.xml
                        pfile=$VGMAINDIR/outgoing/cm/$pfile
                        (
                            case $op in
                            remove)
                                print "<a>remove</a>"
                                print "<u>$sn</u>"
                                print "<f>"
                                print "</f>"
                                ;;
                            *)
                                print "<a>full</a>"
                                print "<u>$sn</u>"
                                print "<f>"
                                cat $ofile
                                print "</f>"
                                ;;
                            esac
                        ) > $pfile.tmp && mv $pfile.tmp $pfile
                        if [[ ${extras} != '' ]] then
                            for extrai in "${!extras.@}"; do
                                [[ $extrai != *.file ]] && continue
                                typeset -n extrar=${extrai%.file}
                                pfile=$VGMAINDIR/outgoing/cm/cm.$ts
                                pfile+=.${extrar.file}.${extrar.rfile//./_X_X_}
                                pfile+=.$sn.$user.$RANDOM.full.xml
                                (
                                    print "<a>full</a>"
                                    print "<u>$sn</u>"
                                    print "<f>"
                                    cat ${extrar.ofile}
                                    print "</f>"
                                ) > $pfile.tmp && mv $pfile.tmp $pfile
                            done
                        fi
                    fi
                    rm $ifile
                done
                print end cm processing $date $(date)
            ) 2>&1
        )
    done

    typeset +n gv
}
