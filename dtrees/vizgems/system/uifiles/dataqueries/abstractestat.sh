querydata=(
    args=(
        dtools='inv alarm stat'
        vtools='invgraph statchart'
    )
    dt_inv=(
        poutlevel=g
        soutlevel=g
        runinparallel=y
    )
    dt_alarm=(
        inlevels=o
    )
    dt_stat=(
        inlevels=o
        groupmode=1
        essentialstats=y
    )
    vt_invgraph=(
        graphlevel=s
        graphattr=(
            label="%(c.nodename)%\n%(l.nodename)% - SiteID: %(s.nodename)%"
            info="Customer: %(c.nodename)%"
        )
        graphnextqid='_str_relationship'
        sgraphattr=(
            label="%(c.nodename)%"
            info="Customer: %(c.nodename)%"
        )
        sgraphnextqid='_str_relationship'
        pclusterattr=(
            label="%(groupname)%"
            info="%(groupname)%"
        )
        pclusternextqid='_str_relationship'
        sclusterattr=(
            label="%(groupname)%"
            info="%(groupname)%"
        )
        sclusternextqid='_str_relationship'
        pnodenextqid='nextqid/_str_relationship'
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
    vt_statchart=(
        statchartnextqid='_str_stattab'
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
    pnlabel+="<path>$SWIFTDATADIR/uifiles/images/n:g:%(name)%.gif</path>"
    pnlabel+="</rimg>"
    snlabel=$pnlabel
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

querydata.vt_invgraph.pnodeattr.label="EMBED:<v>$pnlabel</v>"
querydata.vt_invgraph.snodeattr.label="EMBED:<v>$snlabel</v>"
