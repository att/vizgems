function vg_dserverhelper_run {
    typeset -n gv=$1
    typeset name=$2
    typeset -n params=$3

    typeset prefprefix outfile outdir view0 view0file vid cname

    if [[ ${params.query} != '' ]] then
        prefprefix=${params.prefprefix}
        case ${params.query} in
        dataquery)
            outfile=rep.out
            cname=/data/main/$SWMID/${name##*/}
            if ! createcache gv "$cname" params; then
                gv.outfile=${gv.outdir}/$outfile
                return 0
            else
                cd ${gv.outdir} || exit 1
                (
                    $SHELL vg_dq_run < vars.lst > $outfile
                ) 9>&1
                gv.outfile=$PWD/$outfile
            fi
            ;;
        filterquery)
            outfile=rep.out
            cname=/data/main/$SWMID/${name##*/}
            if ! createcache gv "$cname" params; then
                gv.outfile=${gv.outdir}/$outfile
                return 0
            else
                cd ${gv.outdir} || exit 1
                (
                    $SHELL vg_filter_run < vars.lst > $outfile
                ) 9>&1
                gv.outfile=$PWD/$outfile
            fi
            ;;
        wusagequery)
            outfile=rep.out
            cname=/data/main/$SWMID/${name##*/}
            if ! createcache gv "$cname" params; then
                gv.outfile=${gv.outdir}/$outfile
                return 0
            else
                cd ${gv.outdir} || exit 1
                (
                    $SHELL vg_wusage_run < vars.lst > $outfile
                ) 9>&1
                gv.outfile=$PWD/$outfile
            fi
            ;;
        *)
            ;;
        esac
    else
        return 0
    fi
    return 0
}
