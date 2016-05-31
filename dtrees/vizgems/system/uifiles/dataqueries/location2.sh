querydata=(
    args=(
        dtools='inv alarm'
        vtools='invmap'
        invkeeplevels='b c'
    )
    dt_inv=(
        poutlevel=l2
        soutlevel=
        runinparallel=y
    )
    dt_alarm=(
        inlevels=o
    )
    vt_invmap=(
        invmapattr=(
            color='#BFBFBF'
            fontcolor='#3F3F3F'
            flags='focus'
        )
        pnodeattr=(
            width=30
            height=10
            info='State/Country: %(name)%'
        )
        pnodenextqid='nextqid/_str_location'
    )
    vt_alarmgrid=(
        rendermode=embed
        alarmgridattr=(
            width=35
            height=16
        )
        alarmattr=(
            flags='filled outline'
        )
        titleattr=(
            label=''
            info=''
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
    querydata.vt_alarmgrid.alarmgridattr.height=8
    pnlabel+="<qimg><path>alarmgrid</path></qimg>"
    snlabel+="<qimg><path>alarmgrid</path></qimg>"
elif [[ ${prefqp.alarmgridmode} == 1 ]] then
    querydata.args.vtools+=" alarmgrid"
    pnlabel+="<qimg><path>alarmgrid</path></qimg>"
    snlabel+="<qimg><path>alarmgrid</path></qimg>"
elif [[ ${prefqp.alarmgridmode} == 2 ]] then
    querydata.args.vtools+=" alarmgrid"
    querydata.vt_alarmgrid.alarmgridattr.flags=""
    querydata.vt_alarmgrid.alarmgridattr.height=24
    pnlabel+="<qimg><path>alarmgrid</path></qimg>"
    snlabel+="<qimg><path>alarmgrid</path></qimg>"
elif [[ ${prefqp.alarmgridmode} == 3 ]] then
    querydata.args.vtools+=" alarmstyle"
    pnlabel+="<qimg><path>alarmstyle</path></qimg>"
    snlabel+="<qimg><path>alarmstyle</path></qimg>"
fi

querydata.vt_invmap.pnodeattr.label="EMBED:<h>$pnlabel</h>"

if [[ $SWIFTLLTILESERVER != '' ]] then
    querydata.vt_invmap.useleaflet=y
fi
