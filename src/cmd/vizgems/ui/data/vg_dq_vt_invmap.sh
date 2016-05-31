
dq_vt_invmap_data=(
    deps='' rerun=''
    attrs=() embeds=''
    usell=n
)

function dq_vt_invmap_init {
    typeset -n vt=$1
    typeset -A prefs
    typeset n n1 n2 k v id ev
    typeset -n qprefs=$1.args

    for k in "${!qprefs.@}"; do
        n=${k##*$1.args?(.)}
        [[ $n == '' || $n == *.* ]] && continue
        typeset -n vr=$k
        prefs[$n]=$vr
    done
    for n in "${!vars.invmap_@}"; do
        typeset -n vr=$n
        k=${n##*invmap_}
        prefs[$k]=$vr
    done

    for k in "${!prefs[@]}"; do
        v=${prefs[$k]}
        if [[ $v == *_URL_BASE_* ]] then
            typeset -n mdt=dq_main_data
            v=${v//_URL_BASE_/${mdt.baseurl}}
        fi
        if [[ $v == *_INV_FILTERS_* ]] then
            typeset -n idt=dq_dt_inv_data
            v=${v//_INV_FILTERS_/${idt.filterstr}}
        fi
        if [[ $v == *_INV_KEEPFILTERS_* ]] then
            typeset -n idt=dq_dt_inv_data
            v=${v//_INV_KEEPFILTERS_/${idt.keepfilterstr}}
        fi
        if [[ $v == *_STAT_FILTERS_* ]] then
            typeset -n sdt=dq_dt_stat_data
            v=${v//_STAT_FILTERS_/${sdt.filterstr}}
        fi
        if [[ $v == *_ALARM_FILTERS_* ]] then
            typeset -n adt=dq_dt_alarm_data
            v=${v//_ALARM_FILTERS_/${adt.filterstr}}
        fi
        if [[ $k == *_* ]] then
            n1=${k%%_*}
            n2=${k#*_}
            typeset -n attr=dq_vt_invmap_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_invmap_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_invmap_data.embeds
                embeds+=" $id"
                dq_vt_invmap_data.deps+=" $id"
            done
        fi
    done
    dq_vt_invmap_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_invmap_data.divmode=${prefs[divmode]:-max}
    dq_vt_invmap_data.attrs.pagemode=${prefs[pagemode]:-all}
    dq_vt_invmap_data.attrs.runid=invmap

    [[ ${qprefs.useleaflet} == y ]] && dq_vt_invmap_data.usell=y

    return 0
}

function dq_vt_invmap_run {
    typeset -n vt=$1
    typeset attri embed

    export INVMAPATTRFILE=invmap.attr INVMAPEMBEDLISTFILE=invmap.embedlist
    export INVMAPGEOMDIR=$SWIFTDATADIR/uifiles/geom
    rm -f invmap.*

    (
        print "maxpixel=${dq_main_data.maxpixel}"
        for attri in "${!dq_vt_invmap_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=$attr"
        done
    ) > $INVMAPATTRFILE

    (
        for embed in ${dq_vt_invmap_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $INVMAPEMBEDLISTFILE

    if [[ ${dq_vt_invmap_data.usell} == y ]] then
        ddsfilter -osm none -fso vg_dq_vt_invmapll.filter.so inv.dds \
        > invmap.list
    else
        ddsfilter -osm none -fso vg_dq_vt_invmap.filter.so inv.dds \
        > invmap.list
    fi
    return $?
}

function dq_vt_invmap_emitheader {
    typeset file line loc

    while read file; do
        [[ $file != *.css ]] && continue
        if [[ ${dq_vt_invmap_data.usell} == y ]] then
            while read -r line; do
                case $line in
                *SWIFTLLCSS*)
                    loc=$SWIFTWEBPREFIX/proj/vg/leaflet/leaflet.css
                    line=${line//SWIFTLLCSS/$loc}
                    ;;
                *SWIFTLLJS*)
                    loc=$SWIFTWEBPREFIX/proj/vg/leaflet/leaflet.js
                    line=${line//SWIFTLLJS/$loc}
                    ;;
                esac
                print -r "$line"
            done < $file
        else
            cat $file
        fi
    done < invmap.list

    return 0
}

function dq_vt_invmap_emitbody {
    typeset nodataflag=y id file pagen page pagei paget s flag prefix url
    typeset order flags prefix line line2
    typeset -A ids urls flags

    [[ ${dq_vt_invmap_data.rendermode} == embed ]] && return 0

    while read file; do
        [[ $file != *.toc ]] && continue
        cat $file
    done < invmap.list > invmap.toc
    while read file; do
        if [[ $file == *.lllist ]] then
            print "<script>"
            print "function tmp() {"
            print "  var el, i, classes = new Array ("
            print "    'mdiv', 'bdiv_tools_data', 'bdiv_favorites_data',"
            print "    'bdiv_help_data'"
            print "  )"
            print "  for (i = 0; i < classes.length; i++)"
            print "    if ((el = document.getElementById (classes[i])))"
            print "      el.style.zIndex = '2000'"
            print "}"
            print "tmp();"
            print "</script>"
            while read prefix flag url; do
                urls[$prefix]=$url
                flags[$prefix]=$flag
                order[${#order[@]}]=$prefix
                nodataflag=n
            done < $file
            if [[ -f ${file%.lllist}.html ]] then
                while read -r line; do
                    case $line in
                    *SWIFTLLTILESERVER*)
                        line=${line//SWIFTLLTILESERVER/$SWIFTLLTILESERVER}
                        line=${line//SWIFTLLTILEATTR/$SWIFTLLTILEATTR}
                        ;;
                    *SWIFTLLTILEATTR*)
                        line=${line//SWIFTLLTILESERVER/$SWIFTLLTILESERVER}
                        line=${line//SWIFTLLTILEATTR/$SWIFTLLTILEATTR}
                        ;;
                    *SWLLP*.*PLLWS*)
                        prefix=${line##*SWLLP}; prefix=${prefix%%PLLWS*}
                        line2=${line%%SWLLP*}
                        s=
                        if [[ -f $prefix.imginfo ]] then
                            set -A s $(< $prefix.imginfo)
                            if ((
                                s[0] > 0 && s[0] < 10000 &&
                                s[1] > 0 && s[1] < 10000
                            )) then
                                s[0]="width=${s[0]}" s[1]="height=${s[1]}"
                            fi
                        fi
                        s[${#s[@]}]='class=page'
                        if [[ -f $prefix.cmap ]] then
                            line2+="<img ${s[@]}"
                            line2+=" src=\"$IMAGEPREFIX$prefix.gif\""
                            line2+=" usemap=\"#LL$prefix\">"
                            line2+="<map name=\"LL$prefix\">"
                            line2+="$(< $prefix.cmap)</map>"
                        else
                            line2+="<img ${s[@]}"
                            line2+=" src=\"$IMAGEPREFIX$prefix.gif\">"
                        fi
                        line2+=${line##*PLLWS}
                        line=$line2
                        ;;
                    esac
                    print -r "$line"
                done < ${file%.lllist}.html
            fi
            for prefix in "${order[@]}"; do
                [[ ${flags[$prefix]} != 0 ]] && continue
                if [[ -s $prefix.toc ]] then
                    page=$(< $prefix.toc)
                    pagei=${page%%'|'*}
                    [[ $pagei != '' ]] && print "<!--page $pagei-->"
                fi
                s=
                if [[ -f $prefix.imginfo ]] then
                    set -A s $(< $prefix.imginfo)
                    if ((
                        s[0] > 0 && s[0] < 10000 && s[1] > 0 && s[1] < 10000
                    )) then
                        s[0]="width=${s[0]}" s[1]="height=${s[1]}"
                    fi
                fi
                s[${#s[@]}]='class=page'
                if [[ -f $prefix.cmap ]] then
                    print -n "<img"
                    print -n " ${s[@]} src='$IMAGEPREFIX$prefix.gif'"
                    print -n " usemap='#$prefix'"
                    print ">"
                    print "<map name='$prefix'>"
                    cat $prefix.cmap
                    print "</map>"
                else
                    print "<img ${s[@]} src='$IMAGEPREFIX$prefix.gif'>"
                fi
            done
            break
        fi
        [[ $file != *.gif ]] && continue
        if [[ -s ${file%.gif}.toc ]] then
            page=$(< ${file%.gif}.toc)
            pagei=${page%%'|'*}
            [[ $pagei != '' ]] && print "<!--page $pagei-->"
        fi
        s=
        if [[ -f ${file%.gif}.imginfo ]] then
            set -A s $(< ${file%.gif}.imginfo)
            if (( s[0] > 0 && s[0] < 10000 && s[1] > 0 && s[1] < 10000 )) then
                s[0]="width=${s[0]}" s[1]="height=${s[1]}"
            fi
        fi
        s[${#s[@]}]='class=page'
        if [[ -f ${file%.gif}.cmap ]] then
            print "<img ${s[@]} src='$IMAGEPREFIX$file' usemap='#${file%.gif}'>"
            print "<map name='${file%.gif}'>"
            cat ${file%.gif}.cmap
            print "</map>"
        else
            print "<img ${s[@]} src='$IMAGEPREFIX$file'>"
        fi
        nodataflag=n
    done < invmap.list > invmap.html
    [[ nodataflag == n ]] && print "<!--page 123456789-->" >> invmap.html

    dq_main_emit_sdiv_begin \
        invmap $SCREENWIDTH 200 "${dq_vt_invmap_data.divmode}" \
    "Inv Map View"
    if [[ ${dq_vt_invmap_data.attrs.pagemode} == all ]] then
        pagen=
    else
        pagen=$(tail -1 invmap.toc); pagen=${pagen%%'|'*}
        [[ $pagen == 1 ]] && pagen=
    fi
    if [[ $pagen == '' ]] then
        cat invmap.html
    else
        typeset -n pi=dq_vt_invmap_data.attrs.pageindex
        pi=${pi:-1}
        dq_main_emit_sdivpage_begin invmap invmap.html $pagen $pi
        while read page; do
            pagei=${page%%'|'*}
            paget=${page##*'|'}
            paget="$pagei/$pagen - ${paget:0:20}..."
            dq_main_emit_sdivpage_option "$pagei" "$paget"
        done < invmap.toc
        dq_main_emit_sdivpage_middle invmap invmap.html $pagen $pi
        dq_vt_invmap_emitpage invmap.html $pi
        dq_main_emit_sdivpage_end invmap invmap.html $pagen $pi
    fi
    [[ $HIDEINFO == '' && -f invmap.info ]] && cat invmap.info
    if [[ $HIDEINFO == '' ]] then
        for id in ${dq_vt_invmap_data.embeds}; do
            [[ ${ids[$id]} == '' && -f $id.info ]] && cat $id.info
            ids[$id]=1
        done
    fi
    if [[ $nodataflag == y ]] then
        print "The query returned no inventory records."
    fi
    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_invmap_emitpage { # $1 = file $2 = page
    typeset htmlfile=$1 pageindex=$2
    typeset tocfile=${htmlfile%.html}.toc pagen

    if [[ $pageindex == all ]] then
        cat $htmlfile
    else
        pagen=$(tail -1 $tocfile)
        pagen=${pagen%%'|'*}
        exec 5< $htmlfile
        5<##'<!--page 1-->'
        5<#"<!--page $pageindex-->"
        read -u5
        5<##'<!--page*-->'
        5<#'<!--page 123456789-->'
        5<##EOF
        exec 5<&-
    fi
}

function dq_vt_invmap_setup {
    dq_main_data.query.vtools.invmap=(args=())
    typeset -n imr=dq_main_data.query.vtools.invmap.args
    typeset -n qdimr=querydata.vt_invmap
    typeset -n lvs=dq_main_data.levels
    typeset pnlv=${querydata.dt_inv.poutlevel}
    typeset snlv=${querydata.dt_inv.soutlevel}
    typeset k
    typeset nurlfmt=${dq_main_data.nodeurlfmt}
    typeset eurlfmt=${dq_main_data.edgeurlfmt}

    imr.rendermode=${qdimr.rendermode:-final}
    imr.pagemode=${prefqp.invmappage:-all}
    if [[ $SWIFTLLTILESERVER != '' ]] then
        imr.useleaflet=${qdimr.useleaflet:-n}
    else
        imr.useleaflet=n
    fi

    imr.invmapattr_flags=""
    imr.invmapattr_bgcolor=${prefpp.bgcolor}
    imr.invmapattr_width=$SCREENWIDTH
    imr.invmapattr_height=$SCREENHEIGHT
    imr.invmapattr_color=${prefpp.fgcolor}
    imr.invmapattr_linewidth=2
    imr.invmapattr_fontname=${prefpp.fontfamily}
    imr.invmapattr_fontsize=$(fs2gd ${prefpp.fontsize})
    imr.invmapattr_fontcolor=${prefpp.fgcolor}
    for k in "${!qdimr.invmapattr.@}"; do
        typeset -n srcr=$k dstr=imr.invmapattr_${k##*.}
        dstr=$srcr
    done
    imr.invmapurl=""

    imr.titleattr_color=${prefpp.fgcolor}
    imr.titleattr_fontname=${prefpp.fontfamily}
    imr.titleattr_fontsize=$(fs2gd ${prefpp.fontsize})
    imr.titleattr_label=""
    imr.titleattr_info=""
    for k in "${!qdimr.titleattr.@}"; do
        print -u2 $k
        typeset -n srcr=$k dstr=imr.titleattr_${k##*.}
        dstr=$srcr
    done
    imr.titleurl=""
    if [[ ${qdimr.titlenextqid} != '' ]] then
        imr.titleurl=${nurlfmt//___NEXTQID___/${qdimr.titlenextqid}}
    fi

    imr.pnodeattr_bgcolor=${prefpp.bgcolor}
    imr.pnodeattr_color=${prefpp.fgcolor}
    imr.pnodeattr_linewidth=1
    imr.pnodeattr_fontname=${prefpp.fontfamily}
    imr.pnodeattr_fontsize=$(fs2gd ${prefpp.fontsize})
    if [[ ${imr.useleaflet} == y ]] then
        imr.pnodeattr_coord="%(llcoord)%"
    else
        imr.pnodeattr_coord="%(coord)%"
    fi
    imr.snodeattr_width=50
    imr.snodeattr_height=8
    imr.pnodeattr_label="${lvs[$pnlv].fn}: %(nodename)%"
    imr.pnodeattr_info="${lvs[$pnlv].fn}: %(name)%"
    for k in "${!qdimr.pnodeattr.@}"; do
        typeset -n srcr=$k dstr=imr.pnodeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdimr.pnodenextqid} != '' ]] then
        imr.pnodeurl=${nurlfmt//___NEXTQID___/${qdimr.pnodenextqid}}
    fi

    if [[ $snlv != '' ]] then
        imr.snodeattr_bgcolor=${prefpp.bgcolor}
        imr.snodeattr_color=${prefpp.fgcolor}
        imr.snodeattr_linewidth=1
        imr.snodeattr_fontname=${prefpp.fontfamily}
        imr.snodeattr_fontsize=$(fs2gd ${prefpp.fontsize})
        if [[ ${imr.useleaflet} == y ]] then
            imr.snodeattr_coord="%(llcoord)%"
        else
            imr.snodeattr_coord="%(coord)%"
        fi
        imr.snodeattr_width=50
        imr.snodeattr_height=8
        imr.snodeattr_label="${lvs[$snlv].fn}: %(nodename)%"
        imr.snodeattr_info="${lvs[$snlv].fn}: %(name)%"
        for k in "${!qdimr.snodeattr.@}"; do
            typeset -n srcr=$k dstr=imr.snodeattr_${k##*.}
            dstr=$srcr
        done
        if [[ ${qdimr.snodenextqid} != '' ]] then
            imr.snodeurl=${nurlfmt//___NEXTQID___/${qdimr.snodenextqid}}
        fi

        imr.ppedgeattr_color=${prefpp.fgcolor}
        imr.ppedgeattr_linewidth=1
        imr.ppedgeattr_fontname=${prefpp.fontfamily}
        imr.ppedgeattr_fontsize=$(fs2gd ${prefpp.fontsize})
        imr.ppedgeattr_label=""
        imr.ppedgeattr_info=""
        for k in "${!qdimr.ppedgeattr.@}"; do
            typeset -n srcr=$k dstr=imr.ppedgeattr_${k##*.}
            dstr=$srcr
        done
        if [[ ${qdimr.ppedgenextqid} != '' ]] then
            imr.ppedgeurl=${eurlfmt//___NEXTQID___/${qdimr.ppedgenextqid}}
        fi

        imr.ppedgeattr_color=${prefpp.fgcolor}
        imr.ppedgeattr_linewidth=1
        imr.ppedgeattr_fontname=${prefpp.fontfamily}
        imr.ppedgeattr_fontsize=$(fs2gd ${prefpp.fontsize})
        imr.ppedgeattr_label=""
        imr.ppedgeattr_info=""
        for k in "${!qdimr.ppedgeattr.@}"; do
            typeset -n srcr=$k dstr=imr.ppedgeattr_${k##*.}
            dstr=$srcr
        done
        if [[ ${qdimr.ppedgenextqid} != '' ]] then
            imr.ppedgeurl=${eurlfmt//___NEXTQID___/${qdimr.ppedgenextqid}}
        fi

        imr.psedgeattr_color=${prefpp.fgcolor}
        imr.psedgeattr_linewidth=1
        imr.psedgeattr_fontname=${prefpp.fontfamily}
        imr.psedgeattr_fontsize=$(fs2gd ${prefpp.fontsize})
        imr.psedgeattr_label=""
        imr.psedgeattr_info=""
        for k in "${!qdimr.psedgeattr.@}"; do
            typeset -n srcr=$k dstr=imr.psedgeattr_${k##*.}
            dstr=$srcr
        done
        if [[ ${qdimr.psedgenextqid} != '' ]] then
            imr.psedgeurl=${eurlfmt//___NEXTQID___/${qdimr.psedgenextqid}}
        fi

        imr.spedgeattr_color=${prefpp.fgcolor}
        imr.spedgeattr_linewidth=1
        imr.spedgeattr_fontname=${prefpp.fontfamily}
        imr.spedgeattr_fontsize=$(fs2gd ${prefpp.fontsize})
        imr.spedgeattr_label=""
        imr.spedgeattr_info=""
        for k in "${!qdimr.spedgeattr.@}"; do
            typeset -n srcr=$k dstr=imr.spedgeattr_${k##*.}
            dstr=$srcr
        done
        if [[ ${qdimr.spedgenextqid} != '' ]] then
            imr.spedgeurl=${eurlfmt//___NEXTQID___/${qdimr.spedgenextqid}}
        fi

        imr.ssedgeattr_color=${prefpp.fgcolor}
        imr.ssedgeattr_linewidth=1
        imr.ssedgeattr_fontname=${prefpp.fontfamily}
        imr.ssedgeattr_fontsize=$(fs2gd ${prefpp.fontsize})
        imr.ssedgeattr_label=""
        imr.ssedgeattr_info=""
        for k in "${!qdimr.ssedgeattr.@}"; do
            typeset -n srcr=$k dstr=imr.ssedgeattr_${k##*.}
            dstr=$srcr
        done
        if [[ ${qdimr.ssedgenextqid} != '' ]] then
            imr.ssedgeurl=${eurlfmt//___NEXTQID___/${qdimr.ssedgenextqid}}
        fi
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_invmap_init dq_vt_invmap_run dq_vt_invmap_setup
    typeset -ft dq_vt_invmap_emitbody dq_vt_invmap_emitheader
    typeset -ft dq_vt_invmap_emitpage
fi
