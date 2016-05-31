#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

function m2js { # $1 = cmd $2 = args
    print "  delete tmpo"
    print "  var tmpo = new Object ()"
    typeset -A keys
    typeset havech
    typeset -n x=$2

    for i in "${!x.@}"; do
        k=${i#$2.}
        havech=n
        if [[ $k == *.* ]] then
            k=${k%%.*}
            havech=y
        fi
        if [[ ${keys[$k]} != y ]] then
            keys[$k]=$havech
        fi
    done
    for k in "${!keys[@]}"; do
        typeset -n x=$2.$k
        if [[ ${keys[$k]} == y ]] then
            print "  tmpo._$k = new Array ("
            for ((j = 0; ; j++ )) do
                typeset -n y=$2.$k._$j
                if [[ ${y} == '' ]] then
                    break
                fi
                if (( j != 0 )) then
                    print ", \c"
                fi
                print "\"${y}\""
                typeset +n y
            done
            print "  )"
        else
            print "  tmpo._$k = \"$x\""
        fi
        typeset +n x
    done
}

group=default

suireadcgikv

if [[ $qs_mode == helper ]] then
    print "Content-type: text/xml\n"
    print "<script>"
    while sleep 2; do
        done=n
        for i in $SWMROOT/$qs_dir/msg.*; do
            if [[ -f $i ]] then
                suireadcmd 0 cmd args < $i
                rm $i
                case $cmd in
                load)
                    m2js cmd args
                    print "  parent.frames[0].suipad_insert (tmpo)"
                    ;;
                unload)
                    m2js cmd args
                    print "  parent.frames[0].suipad_remove (tmpo)"
                    ;;
                reload)
                    m2js cmd args
                    print "  parent.frames[0].suipad_reloadmany (tmpo)"
                    ;;
                focus)
                    m2js cmd args
                    print "  parent.frames[0].suipad_focus (tmpo)"
                    ;;
                select)
                    m2js cmd args
                    print "  parent.frames[0].suipad_selection (tmpo)"
                    ;;
                esac | sed 's/%/__P__/g' | sed \
                    -e 's/ /%20/g' -e 's/+/%2B/g' -e 's/;/%3B/g' \
                    -e 's/&/%26/g' -e 's/|/%7C/g' -e 's/</%3C/g' \
                    -e 's/>/%5C/g' \
                | sed 's/__P__/%25/g'
                unset ${!args.@} args
                done=y
                break
            fi
        done
        [[ $done == y ]] && break
        if (( SECONDS > 15 )) then
            print "i = 1"
            SECONDS=0
        fi
    done
    print "</script>"
    exit 0
fi

if [[ $qs_prefprefix == '' ]] then
    print -u2 "ERROR configuration not specified"
    exit 1
fi
. $SWIFTDATADIR$qs_prefprefix.sh
for dir in ${PATH//:/ }; do
    if [[ -f $dir/${instance}_prefshelper ]] then
        . $dir/${instance}_prefshelper
        break
    fi
done

typeset -n x=$instance.pad.$group

print "Content-type: text/html\n"
print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
print "<html>"
print "<head>"
print "<style type='text/css'>"
print "BODY {"
print "  font-family: ${x.viewfontname}; font-size: ${x.viewfontsize};"
print "  background: ${x.viewcolor%%\ *}; color: ${x.viewcolor##*\ };"
print "}"
for var in "${!x.class_@}"; do
    tc=${var##$instance.pad.$group.class_}
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
print "  </style>"
if [[ ${x.showtitle} == 1 ]] then
    print "<title>$SWIFTINSTANCENAME</title>"
fi
print "<script src=/swift/suilss.js></script>"
print "</head>"
print "<body>"
print "<div id=suilss_div></div>"
print "<script>"
print "  suilss_init ('${instance}_lss.cgi?mode=helper&dir=$qs_dir')"
print "</script>"
print "</body>"
print "</html>"

typeset +n x
