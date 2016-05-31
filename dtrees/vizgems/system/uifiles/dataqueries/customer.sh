querydata=(
    args=(
        dtools='inv alarm'
        vtools='invgrid'
        invkeeplevels='b l l2'
    )
    dt_inv=(
        poutlevel=c
        soutlevel=
        runinparallel=y
    )
    dt_alarm=(
        inlevels=o
    )
    vt_invgrid=(
        titleattr=(
            label=""
            info=""
        )
        pnodeattr=(
            width=150
            height=15
        )
        pnodenextqid='nextqid/_str_abstract'
    )
    vt_alarmgrid=(
        rendermode=embed
        titleattr=(
            label=""
            info=""
        )
        alarmgridattr=(
            width=150
            height=32
        )
    )
    vt_alarmstyle=(
        rendermode=embed
        alarmstyleattr=(
            flags=map_filtered_deferred
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
    pnlabel="<rimg>"
    pnlabel+="<alt>$snlabel</alt>"
    pnlabel+="<path>$SWIFTDATADIR/uifiles/images/%(_object_)%.gif</path>"
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
    querydata.vt_alarmgrid.alarmgridattr.height=63
    pnlabel+="<qimg><path>alarmgrid</path></qimg>"
    snlabel+="<qimg><path>alarmgrid</path></qimg>"
elif [[ ${prefqp.alarmgridmode} == 3 ]] then
    querydata.args.vtools+=" alarmstyle"
    pnlabel+="<qimg><path>alarmstyle</path></qimg>"
    snlabel+="<qimg><path>alarmstyle</path></qimg>"
fi

querydata.vt_invgrid.pnodeattr.label="EMBED:<v>$pnlabel</v>"
querydata.vt_invgrid.snodeattr.label="EMBED:<v>$snlabel</v>"
