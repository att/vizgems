
set -A FILES
MLEVEL="0"
LMODE="async"

function usage {
    print "usage: dotty [-V] [-lm (sync|async)] [-el (0|1)] <filename>"
}

function processoptions {
    while [[ $# > 0 ]] do
        case $1 in
        -V)
            print "dotty version 96c (09-24-96)"
            shift
            ;;
        -f)
            shift
            loadfile=$1
            shift
            ;;
        -lt)
            shift
            layouttool=$1
            shift
            ;;
        -lm)
            shift
            LMODE=$1
            if [[ $LMODE != 'sync' && $LMODE != 'async' ]] then
                usage
                exit 1
            fi
            shift
            ;;
        -el)
            shift
            MLEVEL=$1
            if [[ $MLEVEL != '0' && $MLEVEL != '1' ]] then
                usage
                exit 1
            fi
            shift
            ;;
        -mm)
            monitormode=y
            shift
            ;;
        -birdeye)
            viewmode=be
            shift
            ;;
        -)
            FILES[${#FILES[@]}]="'"$1"'"
            shift
            ;;
        -*)
            usage
            exit 1
            ;;
        *)
            FILES[${#FILES[@]}]="'"$1"'"
            shift
            ;;
        esac
    done
}

monitormode=n
viewmode=normal
if [[ $DOTTYOPTIONS != '' ]] then
    processoptions $DOTTYOPTIONS
fi
processoptions "$@"

if [[ $DOTTYPATH != '' ]] then
    export LEFTYPATH="$DOTTYPATH:$LEFTYPATH"
fi

CMDS=""

CMDS="dotty.protogt.layoutmode = '$LMODE';"

CMDS=$(print $CMDS dotty.mlevel = $MLEVEL";")

if [[ $loadfile != '' ]] then
    CMDS=$(print $CMDS load \("'"$loadfile"'"\)";")
fi

if [[ $layouttool != '' ]] then
    CMDS=$(print $CMDS dotty.protogt.lserver = "'$layouttool';")
fi

if [[ $FILES == '' ]] then
    FILES=null
fi
FUNC="dotty.createviewandgraph"
for i in "${FILES[@]}"; do
    CMDS=$(print $CMDS $FUNC \($i, "'"file"'", null, null\)";")
done

leftypath=$(whence -p lefty)
if [[ $leftypath == '' ]] then
    print -u2 "dotty: cannot locate the lefty program"
    print -u2 "       make sure that your path includes"
    print -u2 "       the directory containing dotty and lefty"
    exit 1
fi

$leftypath -e "
load ('dotty.lefty');
checkpath = function () {
    if (tablesize (dotty) > 0)
        remove ('checkpath');
    else {
        echo ('dotty: cannot locate the dotty scripts');
        echo ('       make sure that the environment variable LEFTYPATH');
        echo ('       is set to the directory containing dotty.lefty');
        exit ();
    }
};
setmonitorfile = function (mm) {
    if (mm == 'y') {
        monitorfile = dotty.monitorfilemm;
        monitor ('on', 0);
    } else
        monitorfile = dotty.monitorfile;
};
setviewmode = function (vm) {
    if (vm == 'be')
        dotty.protovt.normal = dotty.protovt.birdseye;
};
checkpath ();
setviewmode ('$viewmode');
dotty.init ();
setmonitorfile ('$monitormode');
$CMDS
txtview ('off');
"
