
function header {
    print -r -- "digraph $2 {"
    print -r -- "$3"
}

function footer {
    print -r -- "label = \"$label\""
    print -r -- "}"
}

function node {
    typeset attr nname i

    shift
    eval attr=\$$#
    for (( i = 1; i < $#; i++ )) do
        eval nname=\$$i
        if [[ $nname == __SKIP__ ]] then
            continue
        fi
        print -r -- "\"$nname\" [ $attr ]"
    done
}

function edge {
    typeset attr nname1 nname2 i

    shift
    eval attr=\$$#
    for (( i = 2; i < $#; i++ )) do
        eval nname1=\$$(($i - 1))
        eval nname2=\$$i
        if [[ $nname1 == __SKIP__ || $nname2 == __SKIP__ ]] then
            continue
        fi
        print -r -- "\"$nname1\" -> \"$nname2\" [ $attr ]"
    done
}

function verbatim {
    typeset attr

    shift
    eval attr=\$$#
    print -r -- "$attr"
}

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

(
    ifs="$IFS"
    IFS='|'
    label=''
    while read -r line; do
        case "$line" in
        hdr\|*)
            ;;
        lg[0-9]*\|*)
            if [[ $label == '' ]] then
                label=${line#*\|}
            else
                label="${label}\\\\n${line#*\|}"
            fi
            ;;
        G\|*)
            header $line
            ;;
        N\|*)
            node $line
            ;;
        E\|*)
            edge $line
            ;;
        V\|*)
            verbatim $line
            ;;
        esac
    done
    footer
    IFS="$ifs"
) | sutgraph -lt dot -Txdot
