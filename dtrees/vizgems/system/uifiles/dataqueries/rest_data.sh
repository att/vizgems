querydata=(
    args=(
        dtools='inv alarm stat alarmstat'
        vtools='invgraph statchart alarmstatchart alarmtab autoinvtab'
    )
    dt_inv=(
        poutlevel=o
        soutlevel=o
        runinparallel=y
    )
    dt_alarm=(
        inlevels=o
    )
    dt_stat=(
        inlevels=o
    )
    dt_alarmstat=(
        inlevels=o
    )
    vt_invgraph=(
        pnodenextqid='nextqid/_str_detailed'
        snodenextqid='nextqid/_str_detailed'
    )
    vt_alarmgrid=(
        rendermode=embed
        alarmgridattr=(
            width=125
            height=32
        )
        titleattr=(
            label=""
            info=""
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
    vt_alarmstatchart=(
        statchartnextqid='_str_alarmstattab'
    )
    vt_alarmtab=(
        pnodeattr=(
            label='%(c.name)%'
        )
        pnodenextqid='nextqid/_str_detailed'
    )
    vt_autoinvtab=(
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
