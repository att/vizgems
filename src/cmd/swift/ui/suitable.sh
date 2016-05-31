
function getsortkeys {
    sortkeys=" ${1//KEY/ } "
}

function putsortkeys {
    typeset s

    s=${1# }
    s=${s% }
    print -r -- ${s// /KEY}
}

function addsortkey {
    integer k
    typeset i s ifs

    ifs="$IFS"
    IFS=' '
    ((k = $1 * 2 + 2))
    if [[ $sortkeys != *\ $k,* ]] then
        print -r " $k,$k${sorttypes[$k]}$sortkeys"
    else
        s=
        for i in $sortkeys; do
            case $i in
            ${k},${k}*r) s=" ${i%r}$s" ;;
            ${k},${k}*) s=" ${i}r$s" ;;
            *) s="$s $i" ;;
            esac
        done
        s="$s "
        print -r -- "$s"
    fi
    IFS="$ifs"
}

function delsortkey {
    integer k
    typeset i s ifs

    ifs="$IFS"
    IFS=' '
    ((k = $1 * 2 + 2))
    if [[ $sortkeys == *\ $k,* ]] then
        s=
        for i in $sortkeys; do
            case $i in
            ${k},${k}*) ;;
            *) s="$s $i" ;;
            esac
        done
        s="$s "
        print -r -- "$s"
    fi
    IFS="$ifs"
}

function gensorturl {
    case $1 in
    -*) print -r -- "$sorturl&sortkey=$(putsortkeys $(delsortkey ${1#-}))" ;;
    *) print -r -- "$sorturl&sortkey=$(putsortkeys $(addsortkey $1))" ;;
    esac
}

function gensortpos {
    integer k count
    typeset i ifs

    ifs="$IFS"
    IFS=' '
    ((k = $1 * 2 + 2))
    count=1
    for i in $sortkeys; do
        case $i in
        ${k},${k}*r) print -- ${count}r ;;
        ${k},${k}*) print -- ${count} ;;
        esac
        ((count++))
    done
    IFS="$ifs"
}

function gensortoptions {
    typeset s

    s=${sortkeys% }
    print -- "${s// / -k }"
}

function getsorttypes {
    integer count
    typeset i

    shift
    count=1
    for i in "$@"; do
        if [[ $i == __SKIP__ ]] then
            continue
        elif [[ $i == number ]] then
            k=nr
        else
            k=
        fi
        sorttypes[$((count * 2 + 2))]=$k
        (( count++ ))
    done
}

function setvar {
    typeset -n x=$instance.table.$group

    eval x.$3=\"$4\"

    typeset +n x
}

function header {
    typeset -n x=$instance.table.$group
    typeset ifs

    ifs="$IFS"
    IFS=' '
    if [[ $SUITABLEISEMBEDED == '' ]] then
        print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
        print "<html>"
        print "<head>"
    fi
    print "<style type='text/css'>"
    print "BODY {"
    print "  font-family: ${x.viewfontname}; font-size: ${x.viewfontsize};"
    print "  background: ${x.viewcolor%%\ *}; color: ${x.viewcolor##*\ };"
    print "}"
    for var in "${!x.class_@}"; do
        tc=${var##$instance.table.$group.class_}
        tc=${tc%_style}
        if [[ $tc == *_* ]] then
            type=${tc%%_*}
            class=${tc#${type}_}
            eval print "$type.$class { \${x.class_${type}_${class}_style} }"
        else
            type=$tc
            eval print "$type { \${x.class_${type}_style} }"
        fi
    done
    print "</style>"
    if [[ $SUITABLEISEMBEDED == '' ]] then
        if [[ ${x.showtitle} == 1 ]] then
            if [[ ${x.title} == __RUNFUNC__ ]] then
                x.title=$(${instance}_${group}_table_gentitle)
            fi
            print "<title>${x.title}</title>"
        fi
    fi
    cat $hdrfile
    if [[ $SUITABLEISEMBEDED == '' ]] then
        print "</head>"
        print "<body>"
        if [[ ${x.showheader} == 1 ]] then
            if [[ ${x.header} == __RUNFUNC__ ]] then
                x.header=$(${instance}_${group}_table_genheader)
            fi
            print "${x.header}"
        fi
    fi
    typeset +n x
    IFS="$ifs"
}

function footer {
    typeset -n x=$instance.table.$group

    if [[ $SUITABLEISEMBEDED == '' ]] then
        if [[ ${x.showfooter} == 1 ]] then
            if [[ ${x.footer} == __RUNFUNC__ ]] then
                x.footer=$(${instance}_${group}_table_genfooter)
            fi
            print "${x.footer}"
        fi
        print "</body>"
        print "</html>"
    fi
    typeset +n x
}

function legendbegin {
    typeset -n x=$instance.table.$group

    if [[ ${x.showlegends} != 1 ]] then
        return
    fi
    print "<table>"

    typeset +n x
}

function legendend {
    typeset -n x=$instance.table.$group

    if [[ ${x.showlegends} != 1 ]] then
        return
    fi
    print "</table><br>"

    typeset +n x
}

function legendentry {
    typeset -n x=$instance.table.$group
    typeset class

    if [[ ${x.showlegends} != 1 ]] then
        return
    fi
    class=$1
    shift
    print -r "<tr><td class=$class>$@</td></tr>"

    typeset +n x
}

function tablebegin {
    print "<table border=1>"
}

function tableend {
    print "</table><br>"
}

function tableentry {
    typeset class field text pos skey sinfo
    integer count
    set -A list

    class=$1
    shift
    count=0
    print "<tr class=$class>"
    for field in $fields; do
        class=$1
        text=$2
        if [[ $text == '' ]] then
            text="&nbsp;"
        fi
        (( $# < 2 )) && break
        shift 2
        (( count++ ))
        if [[ $text == __SKIP__ ]] then
            continue
        fi
        if [[ ${fieldmap[$field]} == -1 ]] then
            continue
        fi
        case $class in
        th*)
            if [[ $sorturl != '' ]] then
                pos=$(gensortpos $count)
                skey="<a class=a${class#th} href=$(gensorturl $count)>$text</a>"
                if [[ $pos != '' ]] then
                    sinfo=$(gensorturl -$count)
                    sinfo=" s:$pos,<a class=a${class#th} href=$sinfo>x</a>"
                else
                    sinfo=
                fi
                list[${fieldmap[$field]}]="  <th class=$class>$skey$sinfo</th>"
            else
                list[${fieldmap[$field]}]="  <th class=$class>$text</th>"
            fi
            ;;
        td*)
            list[${fieldmap[$field]}]="  <td class=$class>$text</td>"
            ;;
        esac
    done
    for item in "${list[@]}"; do
        print -r -- $item
    done
    print "</tr>"
}

set -f

sortkeys=" "
typeset -A sorttypes

while (( $# > 0 )) do
    case $1 in
    -instance)
        instance=$2
        shift 2
        ;;
    -group)
        group=$2
        shift 2
        ;;
    -conf)
        conf=$2
        shift 2
        ;;
    -sortkey)
        if [[ $2 != '' ]] then
            getsortkeys $2
            set -A sortoptions -- $(gensortoptions)
        fi
        shift 2
        ;;
    -sorturl)
        sorturl=$2
        shift 2
        ;;
    *)
        break
        ;;
    esac
done

if [[ $instance == '' ]] then
    print -u2 "ERROR instance not specified"
    exit 1
fi

if [[ $group == '' ]] then
    print -u2 "ERROR group not specified"
    exit 1
fi

if [[ $conf == '' ]] then
    print -u2 "ERROR configuration not specified"
    exit 1
fi
. $conf.sh
export SWIFTPREFPREFIX=${conf#$SWIFTDATADIR}
for dir in ${PATH//:/ }; do
    if [[ -f $dir/${instance}_prefshelper ]] then
        . $dir/${instance}_prefshelper
        break
    fi
done

if [[ $sorturl != '' ]] then
    url="/cgi-bin-members/suitablehelper.cgi"
    url="${url}?instance=$instance"
    url="${url}&group=$group"
    url="${url}&conf=${conf#$SWIFTDATADIR}"
    url="${url}&$sorturl"
    sorturl="$url"
fi

fields=$(eval print "\${$instance.table.$group.fields}")
fieldorder=$(eval print "\${$instance.table.$group.fieldorder}")
fields=${fields//' '/'|'}
fieldorder="${fieldorder//' '/'|'}"

typeset -A fieldmap

function setfields {
    if [[ $fieldorder == '' ]] then
        fieldorder=$fields
    fi
    integer count=0
    for field in $fields; do
        fieldmap[$field]=-1
    done
    for field in $fieldorder; do
        if [[ ${fieldmap[$field]} != -1 ]] then
            continue
        fi
        fieldmap[$field]=$count
        (( count++ ))
    done
}

hdrfile=${TMPDIR:-/tmp}/suitable.hdr.$$
trap 'rm -f $hdrfile' HUP QUIT TERM KILL EXIT
> $hdrfile

ifs="$IFS"
IFS='|'

setfields

mode=''
entries=0
case $sortkeys in
" ")
    cat
    ;;
*)
    integer si=0
    st=
    while read -u3 -r line; do
        if [[ "$line" == tr+([!|])\|td* ]] then
            print -r -- "$si|${line//\<*([!<>])\>/}|__SUITABLE__$line"
            st=a
        else
            if [[ $st == a ]] then
                (( si++ ))
            fi
            print -r -- "$si|__SUITABLE__$line"
            (( si++ ))
            st=b
        fi
    done 3<&0 \
    | sort -t '|' -k 1,1n ${sortoptions[@]} | sed 's/^.*__SUITABLE__//'
    ;;
esac | while read -u3 -r line; do
    case "$line" in
    hdr\|*)
        if [[ $mode == table || $mode == legend ]] then
            print -u2 ERROR hdr directive cannot appear after other directives
            exit 1
        fi
        if [[ $line == hdr\|set\|* ]] then
            setvar $line
        else
            print -r "${line#hdr\|}" >> $hdrfile
        fi
        mode=header
        ;;
    fields\|*)
        if [[ $mode == table ]] then
            print -u2 ERROR type directive cannot appear after other directives
            exit 1
        fi
        fields=${line#fields?}
        fieldorder=
        setfields
        ;;
    type\|*)
        if [[ $mode == table ]] then
            print -u2 ERROR type directive cannot appear after other directives
            exit 1
        fi
        getsorttypes $line
        ;;
    lg*\|*)
        if [[ $mode != table && $mode != legend ]] then
            header
        fi
        if [[ $mode == 'table' ]] then
            tableend
        fi
        if [[ $mode != legend ]] then
            legendbegin
        fi
        legendentry $line
        mode=legend
        ;;
    tr*\|*)
        if (( ++entries % 50 == 0 )) then
            tableend
            tablebegin
            if [[ $hline != '' ]] then
                tableentry $hline
            fi
        fi
        if [[ $mode != table && $mode != legend ]] then
            header
        fi
        if [[ $mode == 'legend' ]] then
            legendend
        fi
        if [[ $mode != table ]] then
            tablebegin
        fi
        if [[ "$line" == tr+([0-9a-zA-Z])\|th* ]] then
            hline="$line"
        fi
        tableentry $line
        mode=table
        ;;
    esac
done 3<&0
if [[ $mode != table && $mode != legend ]] then
    header
fi
if [[ $mode == 'table' ]] then
    tableend
fi
if [[ $mode == 'legend' ]] then
    legendend
fi
footer
IFS="$ifs"
