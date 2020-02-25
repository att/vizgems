
ph_data=(
    title="AT&amp;T Visualizer 10.02"
    host=$(uname -n)
    styles=()
    browser=FF pid='' bannermenu='' htmldir='' cgidir='' opencgidir=''
)

function ph_init { # $1=pid $2=bannermenu
    typeset pid=$1 bannermenu=$2
    typeset pdir

    case $HTTP_USER_AGENT in
    *MSIE*)
        ph_data.browser=IE
        ;;
    *iPhone*|*iPad*)
        ph_data.browser=IP
        ;;
    *BlackBerry*)
        ph_data.browser=BB
        ;;
    *Android*)
        ph_data.browser=AD
        ;;
    *Safari*)
        ph_data.browser=AS
        ;;
    *Mozilla*)
        ph_data.browser=FF
        ;;
    *)
        ph_data.browser=FF
        ;;
    esac

    if ! swmuseringroups "vg_att_admin|vg_confedit*"; then
        bannermenu=${bannermenu//@(:c|c:)/}
    fi
    if swmuseringroups "vg_cus_*"; then
        bannermenu=${bannermenu//@(:p|p:)/}
    fi

    ph_data.pid=$pid
    ph_data.bannermenu=$bannermenu
    ph_data.htmldir=$SWIFTWEBPREFIX/proj/vg
    ph_data.cgidir=$SWIFTWEBPREFIX/cgi-bin-vg-members
    ph_data.opencgidir=$SWIFTWEBPREFIX/cgi-bin

    pdir=$SWIFTDATADIR/cache/profiles/$SWMID
    mkdir -p $pdir
    export SUIPREFGENSYSTEMFILE=$SWIFTDATADIR/uifiles/globalprefs.sh
    export SUIPREFGENUSERFILE=$SWIFTDATADIR/uifiles/preferences/$SWMID.sh
    [[ ! -f $SUIPREFGENUSERFILE ]] && touch $SUIPREFGENUSERFILE
    suiprefgen -f -oprefix $pdir/vg.$pid.prefs -prefname $pid
    . $pdir/vg.$pid.prefs.sh
    typeset -n pcp=vg.style.common
    export SHOWHELP=${pcp.showhelp:-1}
    typeset -n pgp=vg.style.general
    [[ ${pgp.insertpageheader} == */* ]] && pgp.insertpageheader=''
    [[ ${pgp.appendpageheader} == */* ]] && pgp.appendpageheader=''
    [[ ${pgp.insertbodyheader} == */* ]] && pgp.insertbodyheader=''
    [[ ${pgp.appendbodyheader} == */* ]] && pgp.appendbodyheader=''
    [[ ${pgp.insertbodyfooter} == */* ]] && pgp.insertbodyfooter=''
    [[ ${pgp.appendbodyfooter} == */* ]] && pgp.appendbodyfooter=''
    [[ ${pgp.insertbanner} == */* ]] && pgp.insertbanner=''
    [[ ${pgp.appendbanner} == */* ]] && pgp.appendbanner=''
    [[ ${pgp.replacebanner} == */* ]] && pgp.replacebanner=''
    [[ ${pgp.replacebannerimage} == */* ]] && pgp.replacebannerimage=''
    [[ ${pgp.replacesdivtab} == */* ]] && pgp.replacesdivtab=''
    [[ ${pgp.replacetitle} != '' ]] && ph_data.title=${pgp.replacetitle}
    typeset -n ppp=vg.style.page
    typeset -n pbp=vg.style.banner
    typeset -n psp=vg.style.sdiv
    typeset -n pmp=vg.style.mdiv
    ph_data.styles.page=(
        body="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        div="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        a="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.link_fgcolor}; background-color:${ppp.link_bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-decoration:underline; display:block
        "
        a__active="
            color:${ppp.link_bgcolor};
            background-color:${ppp.link_fgcolor}
        "
        a__hover="
            color:${ppp.link_hover_fgcolor};
            background-color:${ppp.link_hover_bgcolor}
        "
        select="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.link_fgcolor}; background-color:${ppp.link_bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        option="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.link_fgcolor}; background-color:${ppp.link_bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        input="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.link_fgcolor}; background-color:${ppp.link_bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        img="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.img_bordercolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px
        "
        table="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        caption="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:center
        "
        tr="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        td="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        div_pagemain="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:center
        "
        div_page="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:center
        "
        table_page="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:center;
            margin:auto
        "
        caption_page="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:center
        "
        tr_page="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:center
        "
        td_page="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:center
        "
        td_pageborder="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:center
        "
        img_page="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${ppp.img_bordercolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px
        "
        a_pageil="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.link_fgcolor}; background-color:${ppp.link_bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-decoration:none; display:inline
        "
        a_pageil_active="
            color:${ppp.link_bgcolor}; background-color:${ppp.link_fgcolor}
        "
        a_pageil_hover="
            color:${ppp.link_hover_fgcolor};
            background-color:${ppp.link_hover_bgcolor}
        "
        a_page="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.link_fgcolor}; background-color:${ppp.link_bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-decoration:none; display:block
        "
        a_page_active="
            color:${ppp.link_bgcolor}; background-color:${ppp.link_fgcolor}
        "
        a_page_hover="
            color:${ppp.link_hover_fgcolor};
            background-color:${ppp.link_hover_bgcolor}
        "
        a_pagelabel="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-decoration:none; display:inline
        "
        a_pagelabel_active="
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor}
        "
        a_pagelabel_hover="
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor}
        "
        select_page="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.link_fgcolor}; background-color:${ppp.link_bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        option_page="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.link_fgcolor}; background-color:${ppp.link_bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        input_page="
            font-family:${ppp.fontfamily}; font-size:${ppp.fontsize};
            color:${ppp.link_fgcolor}; background-color:${ppp.link_bgcolor};
            border-spacing:1px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${ppp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
    )
    ph_data.styles.banner=(
        div_banner="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center
        "
        table_banner="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${pbp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center;
            margin:auto
        "
        caption_banner="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${pbp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center
        "
        tr_banner="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${pbp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center
        "
        td_banner="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${pbp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:center
        "
        img_banner="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${pbp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
        "
        a_banneril="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.link_fgcolor}; background-color:${pbp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:inline
        "
        a_banneril_active="
            color:${pbp.link_bgcolor}; background-color:${pbp.link_fgcolor}
        "
        a_banneril_hover="
            color:${pbp.link_hover_fgcolor};
            background-color:${pbp.link_hover_bgcolor}
        "
        a_banner="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.link_fgcolor}; background-color:${pbp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:block
        "
        a_banner_active="
            color:${pbp.link_bgcolor}; background-color:${pbp.link_fgcolor}
        "
        a_banner_hover="
            color:${pbp.link_hover_fgcolor};
            background-color:${pbp.link_hover_bgcolor}
        "
    )
    ph_data.styles.bdiv=(
        div_bdivmain="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${pbp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        div_bdivlist="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${pbp.bgcolor};
            border-spacing:0px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pbp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left;
            position:absolute; visibility:hidden;
            max-height:400px; overflow:auto
        "
        div_bdivrev="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        div_bdiv="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${pbp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        table_bdiv="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${pbp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left;
            margin:auto
        "
        table_bdivform="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        caption_bdiv="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center
        "
        tr_bdivform="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        tr_bdiv="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${pbp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        td_bdivform="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        td_bdiv="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.fgcolor}; background-color:${pbp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        a_bdivil="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.link_fgcolor}; background-color:${pbp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:inline
        "
        a_bdivil_active="
            color:${pbp.link_bgcolor}; background-color:${pbp.link_fgcolor}
        "
        a_bdivil_hover="
            color:${pbp.link_hover_fgcolor};
            background-color:${pbp.link_hover_bgcolor}
        "
        a_bdiv="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.link_fgcolor}; background-color:${pbp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:block
        "
        a_bdiv_active="
            color:${pbp.link_bgcolor}; background-color:${pbp.link_fgcolor}
        "
        a_bdiv_hover="
            color:${pbp.link_hover_fgcolor};
            background-color:${pbp.link_hover_bgcolor}
        "
        a_bdivlabel="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:inline
        "
        a_bdivlabel_active="
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor}
        "
        a_bdivlabel_hover="
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor}
        "
        select_bdiv="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.link_fgcolor}; background-color:${pbp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        option_bdiv="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.link_fgcolor}; background-color:${pbp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        input_bdiv="
            font-family:${pbp.fontfamily}; font-size:${pbp.fontsize};
            color:${pbp.link_fgcolor}; background-color:${pbp.link_bgcolor};
            border-spacing:0px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pbp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
    )
    ph_data.styles.sdiv=(
        div_sdiv="
            font-family:${psp.fontfamily}; font-size:${psp.fontsize};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${psp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center
        "
        table_sdiv="
            font-family:${psp.fontfamily}; font-size:${psp.fontsize};
            color:${psp.fgcolor}; background-color:${psp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${psp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center;
            margin:auto
        "
        table_sdivmenu="
            font-family:${psp.fontfamily}; font-size:${psp.fontsize};
            color:${psp.fgcolor}; background-color:${psp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${psp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center
        "
        table_sdivline="
            font-family:${psp.fontfamily}; font-size:${psp.fontsize};
            color:${psp.fgcolor}; background-color:${psp.bgcolor};
            border-spacing:1px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${psp.bgcolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center
        "
        caption_sdiv="
            font-family:${psp.fontfamily}; font-size:${psp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${psp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center
        "
        tr_sdiv="
            font-family:${psp.fontfamily}; font-size:${psp.fontsize};
            color:${psp.fgcolor}; background-color:${psp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${psp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center
        "
        td_sdiv="
            font-family:${psp.fontfamily}; font-size:${psp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${psp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        a_sdiv="
            font-family:${psp.fontfamily}; font-size:${psp.fontsize};
            color:${psp.link_fgcolor}; background-color:${psp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${psp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:inline
        "
        a_sdiv_active="
            color:${psp.link_bgcolor}; background-color:${psp.link_fgcolor}
        "
        a_sdiv_hover="
            color:${psp.link_hover_fgcolor};
            background-color:${psp.link_hover_bgcolor}
        "
        a_sdivlabel="
            font-family:${psp.fontfamily}; font-size:${psp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${psp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:inline
        "
        a_sdivlabel_active="
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor}
        "
        a_sdivlabel_hover="
            color:${ppp.fgcolor};
            background-color:${ppp.bgcolor}
        "
        select_sdivpage="
            font-family:${psp.fontfamily}; font-size:${psp.fontsize};
            color:${psp.link_fgcolor}; background-color:${psp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${psp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        option_sdivpage="
            font-family:${psp.fontfamily}; font-size:${psp.fontsize};
            color:${psp.link_fgcolor}; background-color:${psp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${psp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
    )
    ph_data.styles.mdiv=(
        div_mdivmain="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.fgcolor}; background-color:${pmp.bgcolor};
            border-spacing:1px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pmp.outlinecolor};
            margin:1px 1px 1px 1px; padding:4px 4px 4px 4px;
            text-align:left;
            position:absolute
        "
        div_mdivmenu="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.fgcolor}; background-color:${pmp.bgcolor};
            border-spacing:1px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pmp.outlinecolor};
            margin:1px 1px 1px 1px; padding:2px 2px 2px 2px;
            text-align:left
        "
        div_mdivcurr="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:1px 1px 1px 1px; padding:2px 2px 2px 2px;
            text-align:left;
            height:6em; min-width:25ex; overflow:auto
        "
        div_mdivwork="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:1px 1px 1px 1px; padding:2px 2px 2px 2px;
            text-align:left;
            height:6em; min-width:25ex; overflow:auto
        "
        div_mdivform="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        div_mdiv="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.fgcolor}; background-color:${pmp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        table_mdivform="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        caption_mdiv="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:center
        "
        tr_mdivform="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        tr_mdiv="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.fgcolor}; background-color:${pmp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        td_mdivform="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:1px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pmp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        td_mdivformnb="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        td_mdiv="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.fgcolor}; background-color:${pmp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        td_mdivborder="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.fgcolor}; background-color:${pmp.bgcolor};
            border-spacing:0px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-align:left
        "
        a_mdivil="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.link_fgcolor}; background-color:${pmp.link_bgcolor};
            border-spacing:0px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:inline
        "
        a_mdivil_active="
            color:${pmp.link_bgcolor}; background-color:${pmp.link_fgcolor}
        "
        a_mdivil_hover="
            color:${pmp.link_hover_fgcolor};
            background-color:${pmp.link_hover_bgcolor}
        "
        a_mdivilset="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.link_fgcolor}; background-color:${pmp.link_bgcolor};
            border-spacing:0px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:inline
        "
        a_mdivilset_active="
            color:${pmp.link_bgcolor}; background-color:${pmp.link_fgcolor}
        "
        a_mdivilset_hover="
            color:${pmp.link_hover_fgcolor};
            background-color:${pmp.link_hover_bgcolor}
        "
        a_mdivilpick="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.link_fgcolor}; background-color:${pmp.link_bgcolor};
            border-spacing:0px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:inline
        "
        a_mdivilpick_active="
            color:${pmp.link_bgcolor}; background-color:${pmp.link_fgcolor}
        "
        a_mdivilpick_hover="
            color:${pmp.link_hover_fgcolor};
            background-color:${pmp.link_hover_bgcolor}
        "
        a_mdivgo="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.link_fgcolor}; background-color:${pmp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:block
        "
        a_mdivgo_active="
            color:${pmp.link_bgcolor}; background-color:${pmp.link_fgcolor}
        "
        a_mdivgo_hover="
            color:${pmp.link_hover_fgcolor};
            background-color:${pmp.link_hover_bgcolor}
        "
        a_mdiv="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.link_fgcolor}; background-color:${pmp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:block
        "
        a_mdiv_active="
            color:${pmp.link_bgcolor}; background-color:${pmp.link_fgcolor}
        "
        a_mdiv_hover="
            color:${pmp.link_hover_fgcolor};
            background-color:${pmp.link_hover_bgcolor}
        "
        a_mdivlabel="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:0px 0px 0px 0px; padding:0px 0px 0px 0px;
            text-decoration:none; display:inline
        "
        a_mdivlabel_active="
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor}
        "
        a_mdivlabel_hover="
            color:${ppp.fgcolor}; background-color:${ppp.bgcolor}
        "
        select_mdiv="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.link_fgcolor}; background-color:${pmp.link_bgcolor};
            border-spacing:0px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pmp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        option_mdiv="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.link_fgcolor}; background-color:${pmp.link_bgcolor};
            border-spacing:0px; border-style:none; border-collapse:collapse;
            border-width:0px; border-color:${pmp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
        input_mdiv="
            font-family:${pmp.fontfamily}; font-size:${pmp.fontsize};
            color:${pmp.link_fgcolor}; background-color:${pmp.link_bgcolor};
            border-spacing:0px; border-style:solid; border-collapse:collapse;
            border-width:1px; border-color:${pmp.outlinecolor};
            margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;
            text-align:left
        "
    )
    export PHTZ=${vg.style.query.timezone}
    [[ $PHTZ == default || $PHTZ == '' ]] && PHTZ=$TZ
}

function ph_emitheader { # $*=style categories to emit
    typeset pat cat k bk type class mod clid query
    typeset ifs mi as ai id vplus
    typeset -n pgp=vg.style.general

    if [[ ${pgp.insertpageheader} != '' ]] then
        $SHELL $SWIFTDATADIR/uifiles/pref_${pgp.insertpageheader} iph
    fi
    print '<meta http-equiv="Content-type" content="text/html;charset=UTF-8">'
    case ${ph_data.browser} in
    IP) print '<meta name="viewport" content="width=device-width">' ;;
    esac
    print "<style type='text/css'>"
    for pat in "" hover active; do
        for cat in "$@"; do
            typeset -n catp=ph_data.styles.$cat
            for k in "${!catp.@}"; do
                bk=${k##*.}
                typeset -n v=$k
                if [[ $pat == '' ]] then
                    [[ $bk == *_@(hover|active) ]] && continue
                else
                    [[ $bk != *_$pat ]] && continue
                fi
                if [[ $bk == *_*_* ]] then
                    type=${bk%%_*}
                    class=${bk#${type}_}
                    mod=${class#*_}
                    class=${class%%_*}
                    if [[ $class == '' ]] then
                        clid="$type:$mod"
                    else
                        clid="$type.$class:$mod"
                    fi
                elif [[ $bk == *_* ]] then
                    type=${bk%%_*}
                    class=${bk#${type}_}
                    clid="$type.$class"
                else
                    type=$bk
                    clid="$type"
                fi
                typeset -n extrav=vg.style.general.append_${cat}_${bk}
                if [[ $extrav != '' ]] then
                    if [[ $extrav == :replace:* ]] then
                        vplus="${extrav#:replace:}"
                    else
                        vplus="$v; $extrav"
                    fi
                else
                    vplus="$v"
                fi
                set -f
                print -r $clid { $vplus }
                set +f
            done
        done
    done
    print "</style>"

    print "<!--BEGIN SKIP-->"
    print "<script src='$SWIFTWEBPREFIX/proj/vg/vg.js'></script>"
    print "<script>"
    print "var vg_browser = '${ph_data.browser}'"
    print "var vg_pid = '${ph_data.pid}'"
    print "var vg_htmldir = '${ph_data.htmldir}'"
    print "var vg_cgidir = '${ph_data.cgidir}'"
    print "var vg_opencgidir = '${ph_data.opencgidir}'"
    print "var vg_showhelpbuttons = '${SHOWHELP}'"
    print "vg_init ()"
    ifs="$IFS"
    IFS='|'
    set -f
    set -A as -- ${ph_data.bannermenu}
    set +f
    IFS="$ifs"
    mi=0
    print "var vg_bannermenus = new Array ()"
    for (( ai = 0; ai < ${#as[@]}; ai += 5 )) do
        if [[
            ${as[$(( ai + 0 ))]} == link && ${as[$(( ai + 1 ))]} == logout &&
            $SWMACCESS != 1
        ]] then
            continue
        fi
        print "vg_bannermenus[$mi] = {"
        print "  type: '${as[$(( ai + 0 ))]}',"
        print "  key: '${as[$(( ai + 1 ))]}',"
        print "  args: '${as[$(( ai + 2 ))]}',"
        print "  label: '${as[$(( ai + 3 ))]}',"
        print "  help: '${as[$(( ai + 4 ))]}'"
        print "}"
        (( mi++ ))
    done
    print "vg_bannermenun = $mi"
    id=0
    print "var vg_favorites = new Array ()"
    [[ -f $SWIFTDATADIR/uifiles/favorites/$SWMID.txt ]] && \
    egrep -v '^#' $SWIFTDATADIR/uifiles/favorites/$SWMID.txt \
    | while read -r line; do
        label=${line%%'|+|'*}; rest=${line##"$label"?('|+|')}
        query=${rest%%'|+|'*}; rest=${rest##"$query"?('|+|')}
        if [[ $query != *'&'pid=* && $query != pid=* ]] then
            query+='&pid=default'
        fi
        if [[ $query != *'&'date=* && $query != date=* ]] then
            query+='&date=latest'
        fi
        if [[ $query == */vg_dserverhelper.cgi* ]] then
            if [[ $SWIFTWEBPREFIX != '' && $query != $SWIFTWEBPREFIX* ]] then
                query="$SWIFTWEBPREFIX$query"
            elif [[ $SWIFTWEBPREFIX == '' && $query != /cgi-bin-* ]] then
                query=/cgi-bin-${query#*/cgi-bin-}
            fi
        fi
        print "vg_favorites[$id] = { n:'$(printf '%#H' "$label")', q:'$query' }"
        (( id++ ))
    done
    print "</script>"
    print "<!--END SKIP-->"
    if [[ ${pgp.appendpageheader} != '' ]] then
        $SHELL $SWIFTDATADIR/uifiles/pref_${pgp.appendpageheader} aph
    fi
}

function ph_emitbodyheader { # $1=pid $2=class $3=width
    typeset pid=$1 class=$2 width=$3
    typeset img i
    typeset -n pgp=vg.style.general

    shift 3

    if [[ ${pgp.insertbodyheader} != '' ]] then
        $SHELL $SWIFTDATADIR/uifiles/pref_${pgp.insertbodyheader} ibh
    fi
    if [[ ${pgp.replacebanner} != '' ]] then
        $SHELL $SWIFTDATADIR/uifiles/pref_${pgp.replacebanner} rb
    else
        if [[ ${pgp.insertbanner} != '' ]] then
            $SHELL $SWIFTDATADIR/uifiles/pref_${pgp.insertbanner} ib
        fi
        print "<div class=$class><table class=$class>"
        if [[ ${pgp.replacebannerimage} != '' ]] then
            $SHELL $SWIFTDATADIR/uifiles/pref_${pgp.replacebannerimage} ibh
        else
            print "<tr class=$class><td class=$class>"
            if [[ $width == *.@(gif|jpg) ]] then
                img=$width
            else
                case ${ph_data.browser} in
                IP|BB)
                    img=vg_banner2.gif
                    ;;
                *)
                    if (( width >= 650 )) then
                        img=vg_banner1.gif
                    else
                        img=vg_banner2.gif
                    fi
                    ;;
                esac
                if [[ $HTTP_COOKIE == *SWIFT_CODE* ]] then
                    img=vg_bd${img#vg_}
                fi
                img=$SWIFTWEBPREFIX/proj/vg/$img
            fi
            print -n "<img"
            print -n " src='$img'"
            print -n " class=$class style='vertical-align:bottom'"
            print "/>"
            print "</td></tr>"
        fi
        print "<tr class=$class><td class=$class><div"
        print "  id=bdiv class=bdivmain"
        print "></div></td></tr>"
        for i in "$@"; do
            print "<tr class=$class><td class=$class>"
            print -r "$i"
            print "</td></tr>"
        done
        print "</table></div>"
        print "<!--BEGIN SKIP-->"
        print "<script>vg_bdivshow ()</script>"
        print "<!--END SKIP-->"
        if [[ ${pgp.appendbanner} != '' ]] then
            $SHELL $SWIFTDATADIR/uifiles/pref_${pgp.appendbanner} ab
        fi
    fi
    if [[ ${pgp.appendbodyheader} != '' ]] then
        $SHELL $SWIFTDATADIR/uifiles/pref_${pgp.appendbodyheader} abh
    fi
    print "<!--EMAIL TABLE-->"
}

typeset -A ph_tzmap
ph_tzmap["default"]="default time zone"
ph_tzmap["AKST9AKDT"]="Alaskan Time"
ph_tzmap["AST4ADT"]="Atlantic Time"
ph_tzmap["ACST-9:30ACDT"]="Australian Central Time"
ph_tzmap["AEST-10AEDT"]="Australian Eastern Time"
ph_tzmap["AWST-8"]="Australian Western Time"
ph_tzmap["BT-3"]="Baghdad Time"
ph_tzmap["BST-1"]="British Summer Time"
ph_tzmap["CET-1CEST"]="Central European Time"
ph_tzmap["CST6CDT"]="Central Time"
ph_tzmap["CHADT-13:45CHAST"]="Chatham Islands Time"
ph_tzmap["CCT-8"]="China Coast Time"
ph_tzmap["UTC"]="Universal Time (UTC)"
ph_tzmap["EET-2EEST"]="Eastern European Time"
ph_tzmap["EST5EDT"]="Eastern Time"
ph_tzmap["GMT"]="Greenwich Mean Time"
ph_tzmap["GST-10"]="Guam Time"
ph_tzmap["HST10"]="Hawaiian Time"
ph_tzmap["IST-5:30"]="India Time"
ph_tzmap["IDLE-12"]="Date Line East"
ph_tzmap["IDLW12"]="Date Line West"
ph_tzmap["IST-1"]="Irish Summer Time"
ph_tzmap["JST-9"]="Japan Time"
ph_tzmap["MET-1"]="Middle European Time"
ph_tzmap["MSK-3MSD"]="Moscow Time"
ph_tzmap["MST7MDT"]="Mountain Time"
ph_tzmap["NZST-12NZDT"]="New Zealand Time"
ph_tzmap["NST3:30NDT"]="Newfoundland Time"
ph_tzmap["PST8PDT"]="Pacific Time"
ph_tzmap["SWT0"]="Swedish Winter Time"
ph_tzmap["UT"]="Universal Time"
ph_tzmap["WAT1"]="West African Time"
ph_tzmap["WET0WEST"]="Western European Time"

function ph_emitbodyfooter { # $1=class $2=width
    typeset class=$1 width=$2
    typeset st img u1 u2
    typeset -n pgp=vg.style.general

    if [[ ${pgp.insertbodyfooter} != '' ]] then
        $SHELL $SWIFTDATADIR/uifiles/pref_${pgp.insertbodyfooter} ibf
    fi
    u1="https://www.businessdirect.att.com/exclude/tcapt.jhtml?mode=tandc&a=c"
    u2="http://www.att.com/privacy/"
    st="style='font-size:80%'"
    print "<br><br><div class=page>"
    print "<div class=page $st>"
    print "${ph_data.title}"
    print "</div><br>"
    print "<div class=page $st>"
    print "All times in ${ph_tzmap[$PHTZ]}"
    print "</div><br>"
    print "<div class=page $st>"
    print "Page generated by server $(uname -n) on $(TZ=$PHTZ printf '%T')"
    print "</div>"
    print "<br><div class=page $st>"
    if [[ $HTTP_COOKIE == *SWIFT_CODE* ]] then
        print "<a class=page href='$u1' target='_blank'>"
        print "  Terms and Conditions"
        print "</a>"
        print "&nbsp;|&nbsp;<a class=page href='$u2' target='_blank'>"
        print "  Privacy Policy"
        print "</a>"
        print "<br>"
    fi
    print "</div>"
    print "</div>"
    if [[ ${pgp.appendbodyfooter} != '' ]] then
        $SHELL $SWIFTDATADIR/uifiles/pref_${pgp.appendbodyfooter} abf
    else
        print "<div class=page $st>"
        print "&copy; 2006-$(printf '%(%Y)T') AT&amp;T. All rights reserved."
        print "</div>"
    fi
}

phs_data=(
    pid=''
    bannermenu=''
)

function phs_init { # $1=pid $2=menu
    phs_data.pid=$1
    phs_data.bannermenu=$2
    ph_init "${phs_data.pid}" "${phs_data.bannermenu}"

    return 0
}

function phs_emit_header_begin { # $1 = title
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"

    ph_emitheader page banner bdiv
    print "<title>$1</title>"

    return 0
}

function phs_emit_header_end {
    print "</head>"

    return 0
}

function phs_emit_body_begin {
    typeset vt st img

    print "<body>"
    ph_emitbodyheader ${phs_data.pid} banner 1600

    return 0
}

function phs_emit_body_end {
    ph_emitbodyfooter
    print "</body>"
    print "</html>"

    return 0
}
