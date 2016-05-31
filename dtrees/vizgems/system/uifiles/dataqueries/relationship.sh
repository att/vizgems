querydata=(
    args=(
        dtools='inv alarm stat'
        vtools='invgraph alarmtab'
    )
    dt_inv=(
        poutlevel=o
        soutlevel=g
        runinparallel=y
    )
    dt_alarm=(
        inlevels=o
    )
    dt_stat=(
        inlevels=o
        filters=(
            onempty=(
                stat_key='cpu_used._total'
            )
        )
    )
    vt_invgraph=(
        graphlevel=s
        graphattr=(
            label="%(c.nodename)%\n%(l.nodename)% - SiteID: %(s.nodename)%"
            info="Customer: %(c.nodename)%"
        )
        graphnextqid='_str_element'
        sgraphattr=(
            label="%(c.nodename)%"
            info="Customer: %(c.nodename)%"
        )
        sgraphnextqid='_str_element'
        pclusterlevel=g
        pclusterattr=(
            label="%(groupname)%"
            info="%(groupname)%"
        )
        pclusternextqid='nextqid/_str_relationship'
        sclusterattr=(
            label="%(groupname)%"
            info="%(groupname)%"
        )
        sclusternextqid='nextqid/_str_relationship'
        pnodenextqid='nextqid/_str_edetailed'
        snodenextqid='nextqid/_str_relationship'
    )
    vt_alarmgrid=(
        rendermode=embed
        titleattr=(
            label=""
            info=""
        )
        alarmgridattr=(
            flags=map_filtered_deferred
            width=125
            height=32
        )
    )
    vt_alarmstyle=(
        rendermode=embed
        alarmstyleattr=(
            flags=map_filtered_deferred
        )
    )
    vt_alarmtab=(
        pnodeattr=(
            label='%(c.name)%'
        )
        pnodenextqid='nextqid/_str_edetailed'
    )
    vt_statchart=(
        rendermode=embed
        grouporder='asset c1 ci'
        metricorder='1|cpu_used._total|ping_loss|url_time'
        statchartattr=(
            width=125
            height=30
        )
        titleattr=(
            label=''
            info=''
        )
        metricattr=(
            flags='label_inchart quartiles:#FFFFFF|#DDDDDD'
            label='%(stat_baselabel)%'
        )
        xaxisattr=(
            flags='axis'
        )
        yaxisattr=(
            flags='axis'
        )
    )
)

pnlabel="<label>"
pnlabel+="<fontname>${prefpp.fontfamily}</fontname>"
pnlabel+="<fontsize>$(fs2gd ${prefpp.fontsize})</fontsize>"
pnlabel+="<text>%(nodename)%</text>"
pnlabel+="</label>"
snlabel=$pnlabel

if [[ ${prefqp.nodeicons} == 1 ]] then
    pnlabel+="<rimg>"
    pnlabel+="<path>$SWIFTDATADIR/uifiles/images/n:o:%(icon)%.gif</path>"
    pnlabel+="</rimg>"
    snlabel+="<rimg>"
    snlabel+="<path>$SWIFTDATADIR/uifiles/images/n:g:%(name)%.gif</path>"
    snlabel+="</rimg>"
fi

if [[ ${prefqp.alarmgridmode} == 0 ]] then
    querydata.args.vtools+=" alarmgrid"
    querydata.vt_alarmgrid.alarmgridattr.flags="hide_filtered hide_deferred"
    querydata.vt_alarmgrid.alarmgridattr.height=16
    pnlabel+="<qimg><path>alarmgrid</path></qimg>"
    snlabel+="<qimg><path>alarmgrid</path></qimg>"
elif [[ ${prefqp.alarmgridmode} == 1 ]] then
    querydata.args.vtools+=" alarmgrid"
    pnlabel+="<qimg><path>alarmgrid</path></qimg>"
    snlabel+="<qimg><path>alarmgrid</path></qimg>"
elif [[ ${prefqp.alarmgridmode} == 2 ]] then
    querydata.args.vtools+=" alarmgrid"
    querydata.vt_alarmgrid.alarmgridattr.flags=""
    querydata.vt_alarmgrid.alarmgridattr.height=48
    pnlabel+="<qimg><path>alarmgrid</path></qimg>"
    snlabel+="<qimg><path>alarmgrid</path></qimg>"
elif [[ ${prefqp.alarmgridmode} == 3 ]] then
    querydata.args.vtools+=" alarmstyle"
    pnlabel+="<qimg><path>alarmstyle</path></qimg>"
    snlabel+="<qimg><path>alarmstyle</path></qimg>"
fi

if [[ ${prefqp.nodecharts} == 1 ]] then
    querydata.args.vtools+=" statchart"
    querydata.vt_statchart.metricattr.fontsize=${
        fssmaller ${prefpp.fontsize};
    }
    pnlabel+="<qimg><path>statchart</path></qimg>"
fi

querydata.vt_invgraph.pnodeattr.label="EMBED:<v>$pnlabel</v>"
querydata.vt_invgraph.snodeattr.label="EMBED:<v>$snlabel</v>"
